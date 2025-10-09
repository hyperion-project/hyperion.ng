// STL includes
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string>
#include <map>
#include <iostream>

//Qt
#include <QThread>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>
#include <QSize>
#include <QMap>

// Local includes
#include <grabber/drm/DRMFrameGrabber.h>

// Constants
namespace {
	const bool verbose = false;

	// drm discovery service
	const char DISCOVERY_DIRECTORY[] = "/dev/dri";
	const char DISCOVERY_FILEPATTERN[] = "card?";
} //End of constants

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define ALIGN(v, a) (((v) + (a)-1) & ~((a)-1))

static PixelFormat GetPixelFormatForDrmFormat(uint32_t format)
{
	switch (format)
	{
		case DRM_FORMAT_XRGB8888:	return PixelFormat::BGR32;
		case DRM_FORMAT_ARGB8888:	return PixelFormat::BGR32;
		case DRM_FORMAT_XBGR8888:	return PixelFormat::RGB32;
		case DRM_FORMAT_NV12:		return PixelFormat::NV12;
		case DRM_FORMAT_YUV420:		return PixelFormat::I420;
		default:					return PixelFormat::NO_CHANGE;
	}
}

// Code from: https://gitlab.freedesktop.org/drm/igt-gpu-tools/-/blob/master/lib/igt_vc4.c#L339
static size_t vc4_sand_tiled_offset(size_t column_width, size_t column_size, size_t x, size_t y, size_t bpp)
{
	size_t offset = 0;
	size_t cols_x;
	size_t pix_x;

	/* Offset to the beginning of the relevant column. */
	cols_x = x / column_width;
	offset += cols_x * column_size;

	/* Offset to the relevant pixel. */
	pix_x = x % column_width;
	offset += (column_width * y + pix_x) * bpp / 8;

	return offset;
}

DRMFrameGrabber::DRMFrameGrabber(const QString & device, int cropLeft, int cropRight, int cropTop, int cropBottom)
	: Grabber("DRM", cropLeft, cropRight, cropTop, cropBottom)
	, _device(device)
	, _deviceFd (-1)
{
	_useImageResampler = true;
}

DRMFrameGrabber::~DRMFrameGrabber()
{
	if (_deviceFd >= 0)
	{
		freeResources();
		closeDevice();
	}
}

void DRMFrameGrabber::freeResources()
{
	for(auto & connector : connectors)
	{
		for(auto & property : connector.second->props)
		{
			drmModeFreeProperty(property.second.spec);
		}
		drmModeFreeConnector(connector.second->ptr);
		delete connector.second;
	}
	connectors.clear();

	for(auto & encoder : encoders)
	{
		for(auto & property : encoder.second->props)
		{
			drmModeFreeProperty(property.second.spec);
		}
		drmModeFreeEncoder(encoder.second->ptr);
		delete encoder.second;
	}
	encoders.clear();

	if (crtc)
	{
		drmModeFreeCrtc(crtc);
		crtc = nullptr;
	}

	for(auto & plane : planes)
	{
		drmModeFreePlane(plane.second);
	}
	planes.clear();

	for(auto & framebuffer : framebuffers)
	{
		drmModeFreeFB2(framebuffer.second);
	}
	framebuffers.clear();
}

bool DRMFrameGrabber::setupScreen()
{
	bool rc (false);

	if (_deviceFd >= 0)
	{
		freeResources();
		closeDevice();
	}

	rc = getScreenInfo();
	setEnabled(rc);

	return rc;
}

bool DRMFrameGrabber::setWidthHeight(int width, int height)
{
	bool rc (false);

	if (Grabber::setWidthHeight(width, height))
	{
		rc = setupScreen();
	}

	return rc;
}

int DRMFrameGrabber::grabFrame(Image<ColorRgb> & image)
{
	bool rc (true);

	if (_isEnabled && !_isDeviceInError)
	{
		if ( getScreenInfo() )
		{
			for(auto & framebuffer : framebuffers)
			{
				_pixelFormat = GetPixelFormatForDrmFormat(framebuffer.second->pixel_format);
				uint64_t modifier = framebuffer.second->modifier;
				int fb_dmafd = 0;

				DebugIf(verbose, _log, "Framebuffer ID: %d - Width: %d - Height: %d  - DRM Format: %c%c%c%c - PixelFormat: %s"
					, framebuffer.first // framebuffer ID
					, framebuffer.second->width // width
					, framebuffer.second->height // height
					, framebuffer.second->pixel_format         & 0xff
					, (framebuffer.second->pixel_format >> 8)  & 0xff
					, (framebuffer.second->pixel_format >> 16) & 0xff
					, (framebuffer.second->pixel_format >> 24) & 0xff
					, QSTRING_CSTR(pixelFormatToString(_pixelFormat))
				);

				if (_pixelFormat != PixelFormat::NO_CHANGE && modifier == DRM_FORMAT_MOD_LINEAR)
				{
					int w = framebuffer.second->width;
					int h = framebuffer.second->height;
					Grabber::setWidthHeight(w, h);

					int size = 0, lineLength = 0;

					if (_pixelFormat == PixelFormat::I420 || _pixelFormat == PixelFormat::NV12)
					{
						size = (w * h * 3) / 2;
						lineLength = w;
					}
					else if (_pixelFormat == PixelFormat::RGB32 || _pixelFormat == PixelFormat::BGR32)
					{
						size = w * h * 4;
						lineLength = w * 4;
					}

					int ret = drmPrimeHandleToFD(_deviceFd, framebuffer.second->handles[0], O_RDONLY, &fb_dmafd);
					if (ret < 0)
					{
						continue;
					}

					uint8_t *mmapFrameBuffer;
					mmapFrameBuffer = (uint8_t*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fb_dmafd, 0);
					if (mmapFrameBuffer != MAP_FAILED)
					{
						_imageResampler.processImage(mmapFrameBuffer, w, h, lineLength, _pixelFormat, image);
						munmap(mmapFrameBuffer, size);
						close(fb_dmafd);
						break;
					}
					else
					{
						Error(_log, "Format: %c%c%c%c failed. Error: %s"
							, framebuffer.second->pixel_format         & 0xff
							, (framebuffer.second->pixel_format >> 8)  & 0xff
							, (framebuffer.second->pixel_format >> 16) & 0xff
							, (framebuffer.second->pixel_format >> 24) & 0xff
							, strerror(errno)
						);
						break;
					}

					close(fb_dmafd);
				}
				else if (_pixelFormat == PixelFormat::NV12 && modifier >> 56ULL == DRM_FORMAT_MOD_VENDOR_BROADCOM)
				{
					switch (fourcc_mod_broadcom_mod(modifier))
					{
						case DRM_FORMAT_MOD_BROADCOM_SAND32:
						case DRM_FORMAT_MOD_BROADCOM_SAND64:
						case DRM_FORMAT_MOD_BROADCOM_SAND128:
						case DRM_FORMAT_MOD_BROADCOM_SAND256:
						{
							uint64_t modifier_base = fourcc_mod_broadcom_mod(modifier);
							uint32_t column_height = fourcc_mod_broadcom_param(modifier);
							uint32_t column_width_bytes;

							switch (modifier_base)
							{
								case DRM_FORMAT_MOD_BROADCOM_SAND32:
								{
									column_width_bytes = 32;
									break;
								}
								case DRM_FORMAT_MOD_BROADCOM_SAND64:
								{
									column_width_bytes = 64;
									break;
								}
								case DRM_FORMAT_MOD_BROADCOM_SAND128:
								{
									column_width_bytes = 128;
									break;
								}
								case DRM_FORMAT_MOD_BROADCOM_SAND256:
								{
									column_width_bytes = 256;
									break;
								}
							}

							int ret = drmPrimeHandleToFD(_deviceFd, framebuffer.second->handles[0], O_RDONLY, &fb_dmafd);
							if (ret < 0)
							{
								continue;
							}

							int w = framebuffer.second->width;
							int h = framebuffer.second->height;
							Grabber::setWidthHeight(w, h);

							int num_planes = 0;
							size_t plane_bpp[4] = {0, 0, 0, 0};
							int plane_width[4] = {0, 0, 0, 0};
							int plane_height[4] = {0, 0, 0, 0};
							int plane_stride[4] = {0, 0, 0, 0};
							uint32_t size = 0;

							// TODO add NV21 and P030 format (DRM_FORMAT_NV21/DRM_FORMAT_P030)
							if (_pixelFormat == PixelFormat::NV12)
							{
								num_planes = 1; // 2; // Need help for the UV information from the second plane
								plane_bpp[0] = 8;
								plane_bpp[1] = 16;
								plane_width[0] = w;
								plane_width[1] = DIV_ROUND_UP(w, 2);
								plane_height[0] = h;
								plane_height[1] = DIV_ROUND_UP(h, 2);
								size = (w * h * 3) / 2;

								for (int plane = 0; plane < num_planes; plane++)
								{
									uint32_t min_stride = plane_width[plane] * (plane_bpp[plane] / 8);
									plane_stride[plane] = ALIGN(min_stride, column_width_bytes);
								}
							}

							uint8_t *src_buf = (uint8_t*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fb_dmafd, 0);
							if (src_buf != MAP_FAILED)
							{
								uint8_t *dst_buf = (uint8_t*)malloc(size);
								uint32_t column_size = column_width_bytes * column_height;

								for (int plane = 0; plane < num_planes; plane++)
								{
									for (int i = 0; i < plane_height[plane]; i++)
									{
										for (int j = 0; j < plane_width[plane]; j++)
										{
											size_t src_offset = framebuffer.second->offsets[plane];
											size_t dst_offset = framebuffer.second->offsets[plane];

											src_offset += vc4_sand_tiled_offset(column_width_bytes * plane_width[plane] / w, column_size, j, i, plane_bpp[plane]);
											dst_offset += plane_stride[plane] * i + j * plane_bpp[plane] / 8;

											switch (plane_bpp[plane])
											{
												case 8:
													*(uint8_t *)(dst_buf + dst_offset) = *(uint8_t *)(src_buf + src_offset);
													break;
												case 16:
													*(uint16_t *)(dst_buf + dst_offset) = *(uint16_t *)(src_buf + src_offset);
													break;
											}
										}
									}
								}

								_imageResampler.processImage(dst_buf, w, h, w, _pixelFormat, image);

								munmap(src_buf, size);
								free(dst_buf);
								close(fb_dmafd);
								break;
							}

							close(fb_dmafd);
						}

						case DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED:
						{
							Debug(_log, "Broadcom modifier 'DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED' currently not supported");
							break;
						}

						default:
							Debug(_log, "Unknown Broadcom modifier");
					}
				}
				else
					Debug(_log, "Currently unsupported format: %c%c%c%c"
						, framebuffer.second->pixel_format         & 0xff
						, (framebuffer.second->pixel_format >> 8)  & 0xff
						, (framebuffer.second->pixel_format >> 16) & 0xff
						, (framebuffer.second->pixel_format >> 24) & 0xff
					);
			}
		}
	}

	freeResources();
	closeDevice();
	return rc;
}

bool DRMFrameGrabber::openDevice()
{
	bool rc = true;

	_deviceFd = ::open(QSTRING_CSTR(_device), O_RDWR | O_CLOEXEC);
	if (_deviceFd < 0)
	{
		QString errorReason = QString("Error opening %1, [%2] %3").arg(_deviceFd).arg(errno).arg(std::strerror(errno));
		this->setInError(errorReason);
		rc = false;
	}

	return rc;
}

bool DRMFrameGrabber::closeDevice()
{
	bool rc = false;

	if (_deviceFd >= 0)
	{
		if ( ::close(_deviceFd) == 0)
		{
			rc = true;
		}

		_deviceFd = -1;
	}

	return rc;
}

 QSize DRMFrameGrabber::getScreenSize() const
 {
	return getScreenSize(_device);
 }

 QSize DRMFrameGrabber::getScreenSize(const QString& device) const
 {
	int width (0);
	int height(0);

	int drmfd = ::open(QSTRING_CSTR(device), O_RDWR);
	if (drmfd != -1)
	{
		width = 1920;
		height = 1080;
		::close(drmfd);
	}

	return QSize(width, height);
 }

bool DRMFrameGrabber::getScreenInfo()
{
	bool rc (false);

	if ( openDevice() )
	{
		// TODO: move to discover()
		drmSetClientCap(_deviceFd, DRM_CLIENT_CAP_ATOMIC, 1);
		drmSetClientCap(_deviceFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

		// enumerate resources
		drmModeResPtr resources = drmModeGetResources(_deviceFd);
		if (!resources)
		{
			this->setInError(QString("Unable to get DRM resources on %1").arg(_device));
			freeResources();
			closeDevice();
			return false;
		}

		for(int i = 0; i < resources->count_connectors; i++)
		{
			auto connector = new Connector();
			connector->ptr = drmModeGetConnector(_deviceFd, resources->connectors[i]);
			connectors.insert(std::pair<uint32_t, Connector*>(resources->connectors[i], connector));
		}

		for(int i = 0; i < resources->count_encoders; i++)
		{
			auto encoder = new Encoder();
			encoder->ptr = drmModeGetEncoder(_deviceFd, resources->encoders[i]);
			encoders.insert(std::pair<uint32_t, Encoder*>(resources->encoders[i], encoder));
		}

		for(int i = 0; i < resources->count_crtcs; i++)
		{
			crtc = drmModeGetCrtc(_deviceFd, resources->crtcs[i]);
			if (crtc->mode_valid)
			{
				break;
			}

			drmModeFreeCrtc(crtc);
			crtc = nullptr;
		}

		drmModePlaneResPtr planeResources = drmModeGetPlaneResources(_deviceFd);
		for(unsigned int i = 0; i < planeResources->count_planes; i++)
		{
			auto properties = drmModeObjectGetProperties(_deviceFd, planeResources->planes[i], DRM_MODE_OBJECT_PLANE);
			if (!properties) continue;

			bool foundPrimary (false);
			for(int j = 0; i < properties->count_props; i++)
			{
				auto prop = drmModeGetProperty(_deviceFd, properties->props[j]);
				if (strcmp(prop->name, "type") == 0 && properties->prop_values[j] == DRM_PLANE_TYPE_PRIMARY)
				{
					auto plane = drmModeGetPlane(_deviceFd, planeResources->planes[i]);
					if (plane->crtc_id == crtc->crtc_id)
					{
						planes.insert(std::pair<uint32_t, drmModePlanePtr>(planeResources->planes[i], plane));
						foundPrimary = true;
						drmModeFreeProperty(prop);
						break;
					}
					drmModeFreePlane(plane);
				}
				drmModeFreeProperty(prop);
			}

			drmModeFreeObjectProperties(properties);
			if (foundPrimary)
				break;
		}

		// get all properties
		for(auto & connector : connectors)
		{
			auto properties = drmModeObjectGetProperties(_deviceFd, connector.first, DRM_MODE_OBJECT_CONNECTOR);
			if (!properties) continue;
			for(unsigned int i = 0; i < properties->count_props; i++)
			{
				auto prop = drmModeGetProperty(_deviceFd, properties->props[i]);
				connector.second->props.insert(std::pair<std::string, DrmProperty>(std::string(prop->name),
					{ .spec=prop, .value=properties->prop_values[i] }));
			}
		}

		for(auto & encoder : encoders)
		{
			auto properties = drmModeObjectGetProperties(_deviceFd, encoder.first, DRM_MODE_OBJECT_ENCODER);
			if (!properties) continue;
			for(unsigned int i = 0; i < properties->count_props; i++)
			{
				auto prop = drmModeGetProperty(_deviceFd, properties->props[i]);
				encoder.second->props.insert(std::pair<std::string, DrmProperty>(std::string(prop->name),
					{ .spec=prop, .value=properties->prop_values[i] }));
			}
		}

		for(auto & plane : planes)
		{
			for (unsigned int j = 0; j < plane.second->count_formats; ++j)
			{
				drmModeFB2Ptr fb = drmModeGetFB2(_deviceFd, plane.second->fb_id);
				if (fb)
					framebuffers.insert(std::pair<uint32_t, drmModeFB2Ptr>(plane.second->fb_id, fb));
				else
					continue;
			}
		}
		rc = !framebuffers.empty();
	}
	else
		rc = false;

	return rc;
}

QJsonObject DRMFrameGrabber::discover(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QJsonObject inputsDiscovered;

	//Find framebuffer devices 0-9
	QDir deviceDirectory (DISCOVERY_DIRECTORY);
	QStringList deviceFilter(DISCOVERY_FILEPATTERN);
	deviceDirectory.setNameFilters(deviceFilter);
	deviceDirectory.setSorting(QDir::Name);
	QFileInfoList deviceFiles = deviceDirectory.entryInfoList(QDir::System);

	int drmIdx (0);
	QJsonArray video_inputs;

	QFileInfoList::const_iterator deviceFileIterator;
	for (deviceFileIterator = deviceFiles.constBegin(); deviceFileIterator != deviceFiles.constEnd(); ++deviceFileIterator)
	{
		QString const fileName = (*deviceFileIterator).fileName();
		drmIdx = fileName.right(1).toInt();
		QString device = (*deviceFileIterator).absoluteFilePath();
		DebugIf(verbose, _log, "DRM device [%s] found", QSTRING_CSTR(device));

		QSize screenSize = getScreenSize(device);
		if ( !screenSize.isEmpty() )
		{
			QJsonArray fps = { "1", "5", "10", "15", "20", "25", "30", "40", "50", "60" };

			QJsonObject in;

			QString displayName;
			displayName = QString("Output %1").arg(drmIdx);

			in["name"] = displayName;
			in["inputIdx"] = drmIdx;

			QJsonArray formats;
			QJsonObject format;

			QJsonArray resolutionArray;

			QJsonObject resolution;

			resolution["width"] = screenSize.width();
			resolution["height"] = screenSize.height();
			resolution["fps"] = fps;

			resolutionArray.append(resolution);

			format["resolutions"] = resolutionArray;
			formats.append(format);

			in["formats"] = formats;
			video_inputs.append(in);
		}

		if (!video_inputs.isEmpty())
		{
			inputsDiscovered["device"] = "drm";
			inputsDiscovered["device_name"] = "DRM";
			inputsDiscovered["type"] = "screen";
			inputsDiscovered["video_inputs"] = video_inputs;

			QJsonObject defaults, video_inputs_default, resolution_default;
			resolution_default["fps"] = _fps;
			video_inputs_default["resolution"] = resolution_default;
			video_inputs_default["inputIdx"] = 0;
			defaults["video_input"] = video_inputs_default;
			inputsDiscovered["default"] = defaults;
		}
	}

	if (inputsDiscovered.isEmpty())
	{
		DebugIf(verbose, _log, "No displays found to capture from!");
	}

	DebugIf(verbose, _log, "device: [%s]", QString(QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return inputsDiscovered;
}

