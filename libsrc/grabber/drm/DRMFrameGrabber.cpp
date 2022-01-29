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
#include <grabber/DRMFrameGrabber.h>

// Constants
namespace {
	const bool verbose = true;

	// drm discovery service
	const char DISCOVERY_DIRECTORY[] = "/dev/dri";
	const char DISCOVERY_FILEPATTERN[] = "card?";
} //End of constants

static PixelFormat GetPixelFormatForDrmFormat(uint32_t format)
{
	switch (format)
	{
		case DRM_FORMAT_XRGB8888: return PixelFormat::RGB32;
		case DRM_FORMAT_ARGB8888: return PixelFormat::RGB32;
		case DRM_FORMAT_XBGR8888: return PixelFormat::BGR32;
		case DRM_FORMAT_NV12: return PixelFormat::NV12;
		case DRM_FORMAT_YUV420: return PixelFormat::I420;
		default: return PixelFormat::NO_CHANGE;
	}
}

DRMFrameGrabber::DRMFrameGrabber(const QString & device)
	: Grabber("DRM")
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

					int size;
					int ret = drmPrimeHandleToFD(_deviceFd, framebuffer.second->handles[0], O_RDONLY, &fb_dmafd);
					if (ret < 0)
					{
						continue;
					}

					uint8_t *mmapFrameBuffer;
					if (_pixelFormat == PixelFormat::I420 || _pixelFormat == PixelFormat::NV12)
					{
						size = (6 * w * h) / 4;
						mmapFrameBuffer = (uint8_t*)mmap(0, size, PROT_READ, MAP_PRIVATE, fb_dmafd, 0);
						if (mmapFrameBuffer != MAP_FAILED)
						{
							_imageResampler.processImage(mmapFrameBuffer, w, h, size, _pixelFormat, image);
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
					}
					else if (_pixelFormat == PixelFormat::RGB32 || _pixelFormat == PixelFormat::BGR32)
					{
						size = w * h * 4;
						mmapFrameBuffer = (uint8_t*)mmap(0, size, PROT_READ, MAP_PRIVATE, fb_dmafd, 0);
						if (mmapFrameBuffer != MAP_FAILED)
						{
							_imageResampler.processImage(mmapFrameBuffer, w, h, size, _pixelFormat, image);
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
					}

					close(fb_dmafd);

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
		drmIdx = (*deviceFileIterator).fileName().rightRef(1).toInt();
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

