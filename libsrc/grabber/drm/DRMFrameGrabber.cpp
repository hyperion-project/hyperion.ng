#include <grabber/drm/DRMFrameGrabber.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string>
#include <map>
#include <iostream>

#include <QThread>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>
#include <QSize>
#include <QMap>

// Constants
namespace {
	const bool verbose = false;
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

DRMFrameGrabber::DRMFrameGrabber(int deviceIdx, int cropLeft, int cropRight, int cropTop, int cropBottom)
	: Grabber("GRABBER-DRM", cropLeft, cropRight, cropTop, cropBottom)
	, _deviceFd (-1)
	, _crtc (nullptr)
{
	_input = deviceIdx;
	_useImageResampler = true;
}

DRMFrameGrabber::~DRMFrameGrabber()
{
	freeResources();
	closeDevice();
}

void DRMFrameGrabber::freeResources()
{
	for(auto const& [id, connector] : _connectors)
	{
		for(auto const& [name, property] : connector->props)
		{
			drmModeFreeProperty(property.spec);
		}
		drmModeFreeConnector(connector->ptr);
		delete connector;
	}
	_connectors.clear();

	for(auto const& [id, encoder] : _encoders)
	{
		for(auto const& [name, property] : encoder->props)
		{
			drmModeFreeProperty(property.spec);
		}
		drmModeFreeEncoder(encoder->ptr);
		delete encoder;
	}
	_encoders.clear();

	if (_crtc != nullptr)
	{
		drmModeFreeCrtc(_crtc);
		_crtc = nullptr;
	}

	for(auto const& [id, plane] : _planes)
	{
		if (plane != nullptr)
		{
			drmModeFreePlane(plane);
		}
	}
	_planes.clear();

	for(auto const& [id, framebuffer] : _framebuffers)
	{
		drmModeFreeFB2(framebuffer);
	}
	_framebuffers.clear();
}

bool DRMFrameGrabber::setupScreen()
{
	freeResources();
	closeDevice();

	bool success = getScreenInfo();
	setEnabled(success);
	
	return success;
}

bool DRMFrameGrabber::setWidthHeight(int width, int height)
{
	if (Grabber::setWidthHeight(width, height))
	{
		return setupScreen();
	}

	return false;
}

int DRMFrameGrabber::grabFrame(Image<ColorRgb> & image)
{
	if (!_isEnabled || _isDeviceInError)
	{
		freeResources();
		closeDevice();
		return -1;
	}
	
	if ( !getScreenInfo() )
	{
		freeResources();
		closeDevice();
		return -1;
	}
	
	bool newImage {false};
	for(auto const& [id, framebuffer] : _framebuffers)
	{
		_pixelFormat = GetPixelFormatForDrmFormat(framebuffer->pixel_format);
		uint64_t modifier = framebuffer->modifier;
		int fb_dmafd = 0;

		DebugIf(verbose, _log, "Framebuffer ID: %d - Width: %d - Height: %d  - DRM Format: %c%c%c%c - PixelFormat: %s"
			, id // framebuffer ID
			, framebuffer->width // width
			, framebuffer->height // height
			, framebuffer->pixel_format         & 0xff
			, (framebuffer->pixel_format >> 8)  & 0xff
			, (framebuffer->pixel_format >> 16) & 0xff
			, (framebuffer->pixel_format >> 24) & 0xff
			, QSTRING_CSTR(pixelFormatToString(_pixelFormat))
		);

		if (_pixelFormat != PixelFormat::NO_CHANGE && modifier == DRM_FORMAT_MOD_LINEAR)
		{
			int w = framebuffer->width;
			int h = framebuffer->height;
			Grabber::setWidthHeight(w, h);

			int size = 0;
			int lineLength = 0;

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

			int ret = drmPrimeHandleToFD(_deviceFd, framebuffer->handles[0], O_RDONLY, &fb_dmafd);
			if (ret < 0)
			{
				continue;
			}

			uint8_t *mmapFrameBuffer;
			mmapFrameBuffer = (uint8_t*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fb_dmafd, 0);
			if (mmapFrameBuffer != MAP_FAILED)
			{
				_imageResampler.processImage(mmapFrameBuffer, w, h, lineLength, _pixelFormat, image);
				newImage = true;
				
				munmap(mmapFrameBuffer, size);
				close(fb_dmafd);
				break;
			}
			else
			{
				Error(_log, "Format: %c%c%c%c failed. Error: %s"
					, framebuffer->pixel_format         & 0xff
					, (framebuffer->pixel_format >> 8)  & 0xff
					, (framebuffer->pixel_format >> 16) & 0xff
					, (framebuffer->pixel_format >> 24) & 0xff
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

					int ret = drmPrimeHandleToFD(_deviceFd, framebuffer->handles[0], O_RDONLY, &fb_dmafd);
					if (ret < 0)
					{
						continue;
					}

					int w = framebuffer->width;
					int h = framebuffer->height;
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

					auto* src_buf = (uint8_t*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fb_dmafd, 0);
					if (src_buf != MAP_FAILED)
					{
						auto* dst_buf = (uint8_t*)malloc(size);
						uint32_t column_size = column_width_bytes * column_height;

						for (int plane = 0; plane < num_planes; plane++)
						{
							for (int i = 0; i < plane_height[plane]; i++)
							{
								for (int j = 0; j < plane_width[plane]; j++)
								{
									size_t src_offset = framebuffer->offsets[plane];
									size_t dst_offset = framebuffer->offsets[plane];

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
						newImage = true;

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
		{
			Debug(_log, "Currently unsupported format: %c%c%c%c"
				, framebuffer->pixel_format         & 0xff
				, (framebuffer->pixel_format >> 8)  & 0xff
				, (framebuffer->pixel_format >> 16) & 0xff
				, (framebuffer->pixel_format >> 24) & 0xff
			);
		}
	}

	freeResources();
	closeDevice();
	
	// ToDo - Check how to handle sceanrios where images could not be captured. Aer there scenarios where the grabber should be set in  error.
	DebugIf(!newImage & verbose, _log, "No image captured");
	
	return newImage ? 0 : -1;
}

bool DRMFrameGrabber::openDevice()
{
	_deviceFd = ::open(QSTRING_CSTR(getDeviceName()), O_RDWR | O_CLOEXEC);
	if (_deviceFd < 0)
	{
		QString errorReason = QString("Error opening %1, [%2] %3").arg(_deviceFd).arg(errno).arg(std::strerror(errno));
		this->setInError(errorReason);
		return false;
	}

	return true;
}

bool DRMFrameGrabber::closeDevice()
{
	if (_deviceFd < 0)
	{
		return true;
	}
	
	bool success = (::close(_deviceFd) == 0);
	_deviceFd = -1;

	return success;
}

 QSize DRMFrameGrabber::getScreenSize() const
 {
	return getScreenSize(getDeviceName());
 }

QSize DRMFrameGrabber::getScreenSize(const QString& device) const
{
    int width = 0;
    int height = 0;

    // 1. Open the DRM device
    int drmfd = ::open(QSTRING_CSTR(device), O_RDWR);
    if (drmfd < 0) 
    {
        return QSize(width, height);
    }

    // 2. Get device resources
    drmModeResPtr resources = drmModeGetResources(drmfd);
    if (!resources) 
    {
        ::close(drmfd);
        return QSize(width, height);
    }

    // 3. Iterate through connectors to find a connected one
    for (int i = 0; i < resources->count_connectors; ++i) 
    {
        drmModeConnectorPtr connector = drmModeGetConnector(drmfd, resources->connectors[i]);
        if (!connector)
        {
            continue;
        }

        // Check if the connector is connected to a display
        if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0)
        {
            // 4. Find the current active mode
            // The 'encoder_id' links a connector to a CRTC. If it's non-zero, it's active.
            if (connector->encoder_id)
            {
                drmModeEncoderPtr encoder = drmModeGetEncoder(drmfd, connector->encoder_id);
                if (encoder && encoder->crtc_id)
                {
                     drmModeCrtcPtr crtc = drmModeGetCrtc(drmfd, encoder->crtc_id);
                     if (crtc) 
                     {
                        width = crtc->width;
                        height = crtc->height;
                        drmModeFreeCrtc(crtc);
                     }
                }
                if (encoder) 
                {
					drmModeFreeEncoder(encoder);
				}
            }
            
            // If we found a size, we are done with this connector
            if (width > 0 && height > 0)
            {
                 drmModeFreeConnector(connector);
                 break; // Exit the loop once we've found an active screen
            }
        }
        
        // 5. Clean up the current connector resource
        drmModeFreeConnector(connector);
    }

    // 6. Clean up resources and close the device
    drmModeFreeResources(resources);
    ::close(drmfd);

    return QSize(width, height);
}

bool DRMFrameGrabber::getScreenInfo()
{
	if ( !openDevice() )
	{
		return false;
	}
		
	// TODO: move to discover()
	drmSetClientCap(_deviceFd, DRM_CLIENT_CAP_ATOMIC, 1);
	drmSetClientCap(_deviceFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

	// enumerate resources
	drmModeResPtr resources = drmModeGetResources(_deviceFd);
	if (!resources)
	{
		this->setInError(QString("Unable to get DRM resources on %1").arg(getDeviceName()));
		freeResources();
		closeDevice();
		return false;
	}

	for(int i = 0; i < resources->count_connectors; i++)
	{
		auto connector = new Connector();
		connector->ptr = drmModeGetConnector(_deviceFd, resources->connectors[i]);
		_connectors.insert(std::pair<uint32_t, Connector*>(resources->connectors[i], connector));
	}

	for(int i = 0; i < resources->count_encoders; i++)
	{
		auto encoder = new Encoder();
		encoder->ptr = drmModeGetEncoder(_deviceFd, resources->encoders[i]);
		_encoders.insert(std::pair<uint32_t, Encoder*>(resources->encoders[i], encoder));
	}

	for(int i = 0; i < resources->count_crtcs; i++)
	{
		_crtc = drmModeGetCrtc(_deviceFd, resources->crtcs[i]);
		if (_crtc->mode_valid)
		{
			break;
		}

		drmModeFreeCrtc(_crtc);
		_crtc = nullptr;
	}

	drmModePlaneResPtr planeResources = drmModeGetPlaneResources(_deviceFd);
	for(unsigned int i = 0; i < planeResources->count_planes; i++)
	{
		auto properties = drmModeObjectGetProperties(_deviceFd, planeResources->planes[i], DRM_MODE_OBJECT_PLANE);
		if (properties == nullptr) 
		{
			continue;
		}

		bool foundPrimary (false);
		for(int j = 0; i < properties->count_props; i++)
		{
			auto prop = drmModeGetProperty(_deviceFd, properties->props[j]);
			if (strcmp(prop->name, "type") == 0 && properties->prop_values[j] == DRM_PLANE_TYPE_PRIMARY)
			{
				auto plane = drmModeGetPlane(_deviceFd, planeResources->planes[i]);
				if (_crtc == nullptr) // Handle scenario where monitor got disconnected
				{
					drmModeFreePlane(plane);
					drmModeFreeProperty(prop);
					drmModeFreeObjectProperties(properties);
				
					return false;
				}
				
				if (plane->crtc_id == _crtc->crtc_id)
				{
					_planes.insert(std::pair<uint32_t, drmModePlanePtr>(planeResources->planes[i], plane));
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
		{
			break;
		}
	}

	// get all properties
	for(auto const& [id, connector] : _connectors)
	{
		auto properties = drmModeObjectGetProperties(_deviceFd, id, DRM_MODE_OBJECT_CONNECTOR);
		if (properties == nullptr) 
		{
			continue;
		}
		
		for(unsigned int i = 0; i < properties->count_props; i++)
		{
			auto prop = drmModeGetProperty(_deviceFd, properties->props[i]);
			connector->props.insert(std::pair<std::string, DrmProperty>(std::string(prop->name),
				{ .spec=prop, .value=properties->prop_values[i] }));
		}
	}

	for(auto const& [id, encoder] : _encoders)
	{
		auto properties = drmModeObjectGetProperties(_deviceFd, id, DRM_MODE_OBJECT_ENCODER);
		if (properties == nullptr) 
		{
			continue;
		}
		
		for(unsigned int i = 0; i < properties->count_props; i++)
		{
			auto prop = drmModeGetProperty(_deviceFd, properties->props[i]);
			encoder->props.insert(std::pair<std::string, DrmProperty>(std::string(prop->name),
				{ .spec=prop, .value=properties->prop_values[i] }));
		}
	}

	for(auto const& [id, plane] : _planes)
	{
		for (unsigned int j = 0; j < plane->count_formats; ++j)
		{
			drmModeFB2Ptr fb = drmModeGetFB2(_deviceFd, plane->fb_id);
			if (fb == nullptr)
			{
				continue;
			}
			_framebuffers.insert(std::pair<uint32_t, drmModeFB2Ptr>(plane->fb_id, fb));
		}
	}
	
	return !_framebuffers.empty();
}

QJsonObject DRMFrameGrabber::discover(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QJsonObject inputsDiscovered;

	//Find framebuffer devices 0-9
	QDir deviceDirectory (DRM_DIR_NAME);
	QStringList deviceFilter(QString("%1%2").arg(DRM_PRIMARY_MINOR_NAME).arg('?'));
	deviceDirectory.setNameFilters(deviceFilter);
	deviceDirectory.setSorting(QDir::Name);
	QFileInfoList deviceFiles = deviceDirectory.entryInfoList(QDir::System);

	int drmIdx (0);
	QJsonArray video_inputs;

	for (const auto& deviceFile : deviceFiles)
	{
		QString const fileName = deviceFile.fileName();
		drmIdx = fileName.right(1).toInt();
		QString device = deviceFile.absoluteFilePath();
		DebugIf(verbose, _log, "DRM device [%s] found", QSTRING_CSTR(device));

		QSize screenSize = getScreenSize(device);
		
		// ToDo Check how to handle in the UI that the seleced device idx is not the same as the format array index
		// For now not connected devices with be show with a 0x0 size
		//if ( !screenSize.isEmpty() )
		{
			QJsonArray fps = { "1", "5", "10", "15", "20", "25", "30", "40", "50", "60" };

			QJsonObject input;

			QString displayName;
			displayName = QString("Output %1").arg(drmIdx);

			input["name"] = displayName;
			input["inputIdx"] = drmIdx;

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

			input["formats"] = formats;
			video_inputs.append(input);
		}

		if (!video_inputs.isEmpty())
		{
			inputsDiscovered["device"] = "drm";
			inputsDiscovered["device_name"] = "DRM";
			inputsDiscovered["type"] = "screen";
			inputsDiscovered["video_inputs"] = video_inputs;

			QJsonObject defaults;
			QJsonObject video_inputs_default;
			QJsonObject resolution_default;
			
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

