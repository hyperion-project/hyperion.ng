#include <grabber/drm/DRMFrameGrabber.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string>
#include <iostream>
#include <vector>

#include <QThread>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>
#include <QSize>
#include <QMap>

Q_LOGGING_CATEGORY(grabber_drm, "grabber.screen.drm")


// Add missing AMD format modifier definitions for downward compatibility
#ifndef AMD_FMT_MOD_TILE_VER_GFX11
#define AMD_FMT_MOD_TILE_VER_GFX11 4
#endif
#ifndef AMD_FMT_MOD_TILE_VER_GFX12
#define AMD_FMT_MOD_TILE_VER_GFX12 5
#endif


#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define ALIGN(v, a) (((v) + (a) - 1) & ~((a) - 1))

static QString getDrmFormat(uint32_t format)
{
	return QString::fromLatin1(reinterpret_cast<const char*>(&format), sizeof format);
}

static QString getDrmModifierName(uint64_t modifier)
{
	uint64_t vendor = modifier >> 56;
	uint64_t mod = modifier & 0x00FFFFFFFFFFFFFF;
	QString name = QString("VENDOR: 0x%1, MOD: 0x%2").arg(vendor, 2, 16, QChar('0')).arg(mod, 14, 16, QChar('0'));

	switch (modifier)
	{
	case DRM_FORMAT_MOD_INVALID:
		return "DRM_FORMAT_MOD_INVALID";
	case DRM_FORMAT_MOD_LINEAR:
		return "DRM_FORMAT_MOD_LINEAR";
	default:
		break;
	}

	switch (vendor)
	{
	case DRM_FORMAT_MOD_VENDOR_INTEL:
		switch (mod)
		{
		case I915_FORMAT_MOD_X_TILED:
			return "I915_FORMAT_MOD_X_TILED";
		case I915_FORMAT_MOD_Y_TILED:
			return "I915_FORMAT_MOD_Y_TILED";
		case I915_FORMAT_MOD_Yf_TILED:
			return "I915_FORMAT_MOD_Yf_TILED";
		case I915_FORMAT_MOD_Y_TILED_CCS:
			return "I915_FORMAT_MOD_Y_TILED_CCS";
		case I915_FORMAT_MOD_Yf_TILED_CCS:
			return "I915_FORMAT_MOD_Yf_TILED_CCS";
		default:
			return QString("DRM_FORMAT_MOD_INTEL_UNKNOWN [0x%1]").arg(mod, 14, 16, QChar('0'));
		}
	case DRM_FORMAT_MOD_VENDOR_AMD:
		if (mod & AMD_FMT_MOD_TILE_VER_GFX9)
			return "AMD_FMT_MOD_TILE_VER_GFX9";
		if (mod & AMD_FMT_MOD_TILE_VER_GFX10)
			return "AMD_FMT_MOD_TILE_VER_GFX10";
		if (mod & AMD_FMT_MOD_TILE_VER_GFX11)
			return "AMD_FMT_MOD_TILE_VER_GFX11";
		if (mod & AMD_FMT_MOD_TILE_VER_GFX12)
			return "AMD_FMT_MOD_TILE_VER_GFX12";
		if (mod & AMD_FMT_MOD_DCC_BLOCK_128B)
			return "AMD_FMT_MOD_DCC_BLOCK_128B";
		if (mod & AMD_FMT_MOD_DCC_BLOCK_256B)
			return "AMD_FMT_MOD_DCC_BLOCK_256B";
		return QString("DRM_FORMAT_MOD_AMD_UNKNOWN [0x%1]").arg(mod, 14, 16, QChar('0'));
	case DRM_FORMAT_MOD_VENDOR_NVIDIA:
		if (mod & 0x10)
			return "DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D";
		return QString("DRM_FORMAT_MOD_NVIDIA_UNKNOWN [0x%1]").arg(mod , 14, 16, QChar('0'));
	case DRM_FORMAT_MOD_VENDOR_BROADCOM:
		switch (fourcc_mod_broadcom_mod(modifier))
		{
		case DRM_FORMAT_MOD_BROADCOM_SAND32:
			return "DRM_FORMAT_MOD_BROADCOM_SAND32";
		case DRM_FORMAT_MOD_BROADCOM_SAND64:
			return "DRM_FORMAT_MOD_BROADCOM_SAND64";
		case DRM_FORMAT_MOD_BROADCOM_SAND128:
			return "DRM_FORMAT_MOD_BROADCOM_SAND128";
		case DRM_FORMAT_MOD_BROADCOM_SAND256:
			return "DRM_FORMAT_MOD_BROADCOM_SAND256";
		case DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED:
			return "DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED";
		default:
			return QString("DRM_FORMAT_MOD_BROADCOM_UNKNOWN [0x%1]").arg(mod, 14, 16, QChar('0'));
		}
	case DRM_FORMAT_MOD_VENDOR_ARM:
		if ((modifier & DRM_FORMAT_MOD_ARM_AFBC(0)) == DRM_FORMAT_MOD_ARM_AFBC(0))
			return "DRM_FORMAT_MOD_ARM_AFBC";
		return QString("DRM_FORMAT_MOD_ARM_UNKNOWN [0x%1]").arg(mod, 14, 16, QChar('0'));
		
	default:
		break;
	}

	return name;
}

static PixelFormat GetPixelFormatForDrmFormat(uint32_t format)
{
    switch (format)
	{
	case DRM_FORMAT_XRGB8888:
		return PixelFormat::BGR32;
	case DRM_FORMAT_ARGB8888:
		return PixelFormat::BGR32;
	case DRM_FORMAT_XBGR8888:
		return PixelFormat::RGB32;
	case DRM_FORMAT_ABGR8888:
		return PixelFormat::RGB32;
	case DRM_FORMAT_NV12:
		return PixelFormat::NV12;
	case DRM_FORMAT_YUV420:
		return PixelFormat::I420;
	default:
		return PixelFormat::NO_CHANGE;
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

// Forward declarations for QDebug operators
QDebug operator<<(QDebug dbg, const drmModeFB2* fb);
QDebug operator<<(QDebug dbg, const drmModePlane* plane);

DRMFrameGrabber::DRMFrameGrabber(int deviceIdx, int cropLeft, int cropRight, int cropTop, int cropBottom)
    : Grabber("GRABBER-DRM", cropLeft, cropRight, cropTop, cropBottom), _deviceFd(-1), _crtc(nullptr)
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
	_connectors.clear();
	_encoders.clear();

	if (_crtc != nullptr)
	{
		drmModeFreeCrtc(_crtc);
		_crtc = nullptr;
	}

	for (auto const &[id, plane] : _planes)
	{
		if (plane != nullptr)
		{
			drmModeFreePlane(plane);
		}
	}
	_planes.clear();

	for (auto const &[id, framebuffer] : _framebuffers)
	{
		drmModeFreeFB2(framebuffer);
	}
	_framebuffers.clear();
}

bool DRMFrameGrabber::setupScreen()
{
	freeResources();
	closeDevice();

	bool success = openDevice() && getScreenInfo();
	setEnabled(success);

	if (!success)
	{
		freeResources();
		closeDevice();
	}

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

int DRMFrameGrabber::grabFrame(Image<ColorRgb> &image)
{
	if (!_isEnabled || _isDeviceInError)
	{
		return -1;
	}

	if (_framebuffers.empty())
	{
		Error(_log, "No framebuffers found. Was setupScreen() successful?");
		return -1;
	}

	bool newImage{false};
	QString errorString;

	for (auto const &[id, framebuffer] : _framebuffers)
	{
        _pixelFormat = GetPixelFormatForDrmFormat(framebuffer->pixel_format);
        uint64_t modifier = framebuffer->modifier;
        int fb_dmafd = 0;

        qCDebug(grabber_drm) << QString("Framebuffer ID: %1 - Width: %2 - Height: %3  - DRM Format: %4 - PixelFormat: %5, Modifier: %6")
                .arg(id) // framebuffer ID
                .arg(framebuffer->width) // width
                .arg(framebuffer->height) // height
                .arg(QSTRING_CSTR(getDrmFormat(framebuffer->pixel_format)))
                .arg(QSTRING_CSTR(pixelFormatToString(_pixelFormat)))
                .arg(QSTRING_CSTR(getDrmModifierName(modifier)));

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
            if (ret != 0)
            {
                Error(_log, "drmPrimeHandleToFD failed (handle=%u): %s", framebuffer->handles[0], strerror(errno));
                continue;
            }

            uint8_t *mmapFrameBuffer;
            mmapFrameBuffer = (uint8_t *)mmap(nullptr, size, PROT_READ, MAP_SHARED, fb_dmafd, 0);
            if (mmapFrameBuffer != MAP_FAILED)
            {
                _imageResampler.processImage(mmapFrameBuffer, w, h, lineLength, _pixelFormat, image);
                newImage = true;

                munmap(mmapFrameBuffer, size);
                close(fb_dmafd);
                break;
            }

            Error(_log, "Format: %s failed. Error: %s", QSTRING_CSTR(getDrmFormat(framebuffer->pixel_format)), strerror(errno));
            close(fb_dmafd);
            break;
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
				default:
				{
					errorString = QString("Broadcom modifier '%1' currently not supported").arg(getDrmFormat(framebuffer->pixel_format));
					break;
				}
				}

                int ret = drmPrimeHandleToFD(_deviceFd, framebuffer->handles[0], O_RDONLY, &fb_dmafd);

                if (ret < 0)
                {
                    Error(_log, "drmPrimeHandleToFD failed (broadcom handle=%u): %s", framebuffer->handles[0], strerror(errno));
                    continue;
                }

				int w = framebuffer->width;
				int h = framebuffer->height;
				Grabber::setWidthHeight(w, h);

				int num_planes = 0;
				std::vector<size_t> plane_bpp(4, 0);
				std::vector<int> plane_width(4, 0);
				std::vector<int> plane_height(4, 0);
				std::vector<int> plane_stride(4, 0);
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
				if (size == 0)
				{
					Error(_log, "Computed framebuffer size is 0 for Broadcom SAND layout");
					close(fb_dmafd);
					break;
				}

				auto *src_buf = (uint8_t *)mmap(nullptr, size, PROT_READ, MAP_SHARED, fb_dmafd, 0);
				if (src_buf != MAP_FAILED)
				{
					std::vector<uint8_t> dst_buf(size);
					uint8_t* dst_ptr = dst_buf.data();
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
									*(dst_ptr + dst_offset) = *(src_buf + src_offset);
									break;
								case 16:
									*(uint16_t *)(dst_ptr + dst_offset) = *(uint16_t *)(src_buf + src_offset);
									break;
								default:
									Error(_log, "Unsupported bpp %d in Broadcom SAND layout", plane_bpp[plane]);
									break;
								}
							}
						}
					}

					_imageResampler.processImage(dst_ptr, w, h, w, _pixelFormat, image);
					newImage = true;

					munmap(src_buf, size);
					close(fb_dmafd);
					break;
				}

				close(fb_dmafd);
				break;
			}

			case DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED:
			{
				errorString = QString("Broadcom modifier 'DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED' currently not supported");
				break;
			}

            default:
                errorString = QString("Unknown Broadcom modifier");
            }
        }
        else
        {
			errorString = QString("Currently unsupported format: %1 and/or modifier: %2")
									  .arg(getDrmFormat(framebuffer->pixel_format))
									  .arg(getDrmModifierName(framebuffer->modifier));
        }
    }
	
	if (!errorString.isEmpty())
	{
		this->setInError(errorString);
		return -1;
	}

	if (!newImage)
	{
		qCDebug(grabber_drm) << "No image captured from DRM framebuffer.";
		return -1;
	}

	return 0;
}

bool DRMFrameGrabber::openDevice()
{
	if (_deviceFd >= 0)
	{
		return true;
	}

	if (!_isAvailable)
	{
		return false;
	}

	// Try read-only first to minimize required privileges. Some drivers require O_RDWR; fallback in that case.
	_deviceFd = ::open(QSTRING_CSTR(getDeviceName()), O_RDONLY | O_CLOEXEC);
	if (_deviceFd < 0)
	{
		// fallback to read-write if required by driver
		_deviceFd = ::open(QSTRING_CSTR(getDeviceName()), O_RDWR | O_CLOEXEC);
	}
	if (_deviceFd < 0)
	{
		QString errorReason = QString("Error opening %1, [%2] %3").arg(getDeviceName()).arg(errno).arg(std::strerror(errno));
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

QSize DRMFrameGrabber::getScreenSize(const QString &device) const
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
	const drmModeResPtr resources = drmModeGetResources(drmfd);
	if (!resources)
	{
		::close(drmfd);
		return QSize(width, height);
	}

	// 3. Iterate through connectors to find a connected one
	for (int i = 0; i < resources->count_connectors; ++i)
	{
		drmModeConnectorPtr connector = drmModeGetConnector(drmfd, resources->connectors[i]);
		if (connector == nullptr)
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
				if (encoder != nullptr && encoder->crtc_id)
				{
					drmModeCrtcPtr crtc = drmModeGetCrtc(drmfd, encoder->crtc_id);
					if (crtc)
					{
						width = crtc->width;
						height = crtc->height;
						drmModeFreeCrtc(crtc);
					}
				}
				if (encoder != nullptr)
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
	if (_deviceFd < 0)
	{
		this->setInError("DRM device not open");
		return false;
	}

	if (drmSetClientCap(_deviceFd, DRM_CLIENT_CAP_ATOMIC, 1) < 0)
	{
		Debug(_log, "drmSetClientCap(DRM_CLIENT_CAP_ATOMIC) failed: %s", strerror(errno));
	}
	if (drmSetClientCap(_deviceFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) < 0)
	{
		Debug(_log, "drmSetClientCap(DRM_CLIENT_CAP_UNIVERSAL_PLANES) failed: %s", strerror(errno));
	}

	// enumerate resources
	drmModeResPtr resources = drmModeGetResources(_deviceFd);
	if (!resources)
	{
		this->setInError(QString("Unable to get DRM resources on %1").arg(getDeviceName()));
		return false;
	}

	for (int i = 0; i < resources->count_connectors; i++)
	{
		auto connector = std::make_unique<Connector>();
		connector->ptr = drmModeGetConnector(_deviceFd, resources->connectors[i]);
		_connectors.insert_or_assign(resources->connectors[i], std::move(connector));
	}

	for (int i = 0; i < resources->count_encoders; i++)
	{
		auto encoder = std::make_unique<Encoder>();
		encoder->ptr = drmModeGetEncoder(_deviceFd, resources->encoders[i]);
		_encoders.insert_or_assign(resources->encoders[i], std::move(encoder));
	}

	for (int i = 0; i < resources->count_crtcs; i++)
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
	if (planeResources)
	{
		for (unsigned int i = 0; i < planeResources->count_planes; ++i)
		{
			auto properties = drmModeObjectGetProperties(_deviceFd, planeResources->planes[i], DRM_MODE_OBJECT_PLANE);
			if (properties == nullptr)
			{
				continue;
			}

			bool foundPrimary = false;
			for (unsigned int j = 0; j < properties->count_props; ++j)
			{
				auto prop = drmModeGetProperty(_deviceFd, properties->props[j]);
				if (prop == nullptr)
				{
					continue;
				}

				if (strcmp(prop->name, "type") == 0 && properties->prop_values[j] == DRM_PLANE_TYPE_PRIMARY)
				{
					auto plane = drmModeGetPlane(_deviceFd, planeResources->planes[i]);
					if (!plane)
					{
						drmModeFreeProperty(prop);
						continue;
					}

					// If no CRTC found (monitor disconnected), skip this plane
					if (_crtc == nullptr)
					{
						drmModeFreePlane(plane);
						drmModeFreeProperty(prop);
						drmModeFreeObjectProperties(properties);
						continue;
					}

					if (plane->crtc_id == _crtc->crtc_id)
					{
                        qCDebug(grabber_drm) << plane;
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
				break;
		}

		drmModeFreePlaneResources(planeResources);
	}
	else
	{
		Debug(_log, "drmModeGetPlaneResources returned NULL or failed: %s", strerror(errno));
	}

	// get all properties
	for (auto const &[id, connector] : _connectors)
	{
		auto properties = drmModeObjectGetProperties(_deviceFd, id, DRM_MODE_OBJECT_CONNECTOR);
		if (properties == nullptr)
		{
			continue;
		}

		for (unsigned int i = 0; i < properties->count_props; i++)
		{
			auto prop = drmModeGetProperty(_deviceFd, properties->props[i]);
			connector->props.insert(std::pair<std::string, DrmProperty>(std::string(prop->name),
																		{.spec = prop, .value = properties->prop_values[i]}));
		}
	}

	for (auto const &[id, encoder] : _encoders)
	{
		auto properties = drmModeObjectGetProperties(_deviceFd, id, DRM_MODE_OBJECT_ENCODER);
		if (properties == nullptr)
		{
			continue;
		}

		for (unsigned int i = 0; i < properties->count_props; i++)
		{
			auto prop = drmModeGetProperty(_deviceFd, properties->props[i]);
			encoder->props.insert(std::pair<std::string, DrmProperty>(std::string(prop->name),
																	  {.spec = prop, .value = properties->prop_values[i]}));
		}
	}

	for (auto const &[id, plane] : _planes)
	{
        drmModeFB2Ptr fb = drmModeGetFB2(_deviceFd, plane->fb_id);
        qCDebug(grabber_drm) << fb;
        if (fb == nullptr)
        {
            continue;
        }

		if (fb->handles[0] == 0)
		{
			setInError("Not able to acquire framebuffer handles. Screen capture not possible. Check permissions.");
			drmModeFreeFB2(fb);
			continue;
		}

		_framebuffers.insert(std::pair<uint32_t, drmModeFB2Ptr>(plane->fb_id, fb));
    }

    drmModeFreeResources(resources);

    qCDebug(grabber_drm) << QString("Framebuffer count: %1").arg(_framebuffers.size());

    return !_framebuffers.empty();
}

QJsonArray DRMFrameGrabber::getInputDeviceDetails() const
{
	// Find framebuffer devices 0-9
	QDir deviceDirectory(DRM_DIR_NAME);
	QStringList deviceFilter(QString("%1%2").arg(DRM_PRIMARY_MINOR_NAME).arg('?'));
	deviceDirectory.setNameFilters(deviceFilter);
	deviceDirectory.setSorting(QDir::Name);
	QFileInfoList deviceFiles = deviceDirectory.entryInfoList(QDir::System);

	QJsonArray video_inputs;
	for (const auto &deviceFile : deviceFiles)
	{
		QString const fileName = deviceFile.fileName();
		int deviceIdx = fileName.right(1).toInt();
		QString device = deviceFile.absoluteFilePath();
		qCDebug(grabber_drm) << QString("DRM device [%1] found").arg(QSTRING_CSTR(device));

		QSize screenSize = getScreenSize(device);
		//Only add devices with a valid screen size, i.e. where a monitor is connected
		if ( !screenSize.isEmpty() )
		{
			QJsonArray fps = {"1", "5", "10", "15", "20", "25", "30", "40", "50", "60"};

			QJsonObject input;

			QString displayName;
			displayName = QString("Output %1").arg(deviceIdx);

			input["name"] = displayName;
			input["inputIdx"] = deviceIdx;

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
	}

	return video_inputs;
}

QJsonObject DRMFrameGrabber::discover(const QJsonObject &params)
{
	qCDebug(grabber_drm) << "params: " << QJsonDocument(params).toJson(QJsonDocument::Compact);

	if (!isAvailable(false))
	{
		return {};
	}

	QJsonObject inputsDiscovered;

	QJsonArray const video_inputs = getInputDeviceDetails();
	if (video_inputs.isEmpty())
	{
		qCDebug(grabber_drm) << "No displays found to capture from!";
		return {};
	}

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

	qCDebug(grabber_drm) << "Discovered input devices:" << QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact).constData();

	return inputsDiscovered;
}

QDebug operator<<(QDebug dbg, const drmModeFB2* fb)
{
	QDebugStateSaver saver(dbg);
	dbg.nospace();

	if (!fb)
	{
		dbg << "drmModeFB2Ptr(null)";
		return dbg;
	}

	dbg << "drmModeFB2("
		<< "id: " << fb->fb_id
		<< ", size: " << fb->width << "x" << fb->height
		<< ", DRM format: " << getDrmFormat(fb->pixel_format)
		<< ", DRM modifier: " << getDrmModifierName(fb->modifier)
		<< ", flags: " << Qt::hex << fb->flags << Qt::dec
		<< ", handles: " << fb->handles[0] << fb->handles[1] << fb->handles[2] << fb->handles[3]
		<< ", pitches: " << fb->pitches[0] << fb->pitches[1] << fb->pitches[2] << fb->pitches[3]
		<< ", offsets: " << fb->offsets[0] << fb->offsets[1] << fb->offsets[2] << fb->offsets[3]
		<< ")";

	return dbg;
}

QDebug operator<<(QDebug dbg, const drmModePlane* plane)
{
	QDebugStateSaver saver(dbg);
	dbg.nospace();

	if (!plane)
	{
		dbg << "drmModePlanePtr(null)";
		return dbg;
	}

	dbg << "drmModePlane("
		<< "id: " << plane->plane_id
		<< ", CRTC id: " << plane->crtc_id
		<< ", FB id: " << plane->fb_id
		<< ", pos: " << plane->x << "," << plane->y
		<< ", CRTC pos: " << plane->crtc_x << "," << plane->crtc_y
		<< ", possible CRTCs: " << Qt::hex << plane->possible_crtcs << Qt::dec
		<< ", gamma size: " << plane->gamma_size
		<< ", format count: " << plane->count_formats
		<< ")";

	return dbg;
}
