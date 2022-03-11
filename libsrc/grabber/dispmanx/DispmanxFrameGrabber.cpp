
// STL includes
#include <cassert>
#include <iostream>

//Qt
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSize>

// Constants
namespace {
	const bool verbose = false;
	const int DEFAULT_DEVICE = 0;
} //End of constants

// Local includes
#include "grabber/DispmanxFrameGrabber.h"

DispmanxFrameGrabber::DispmanxFrameGrabber()
	: Grabber("DISPMANXGRABBER")
	, _lib(nullptr)
	, _vc_display(0)
	, _vc_resource(0)
	, _vc_flags(DISPMANX_TRANSFORM_T(0))
	, _captureBuffer(new ColorRgba[0])
	, _captureBufferSize(0)
	, _image_rgba()
{
	_useImageResampler = true;
}

bool DispmanxFrameGrabber::isAvailable()
{
#ifdef BCM_FOUND
	void* bcm_host = dlopen(std::string("" BCM_LIBRARY).c_str(), RTLD_LAZY | RTLD_GLOBAL);
	if (bcm_host != nullptr)
	{
		dlclose(bcm_host);
		return true;
	}

	return false;
#else
	return false;
#endif
}

bool DispmanxFrameGrabber::open()
{
#ifdef BCM_FOUND
	if (_lib != nullptr)
		return true;

	std::string library = std::string("" BCM_LIBRARY);
	_lib = dlopen(library.c_str(), RTLD_LAZY | RTLD_GLOBAL);
	if (!_lib)
	{
		DebugIf(verbose, _log, "dlopen for %s failed, error = %s", library.c_str(), dlerror());
		return false;
	}

	dlerror(); /* Clear any existing error */

	if (!(*(void**)(&wr_bcm_host_init) =					dlsym(_lib,"bcm_host_init"))) goto dlError;
	if (!(*(void**)(&wr_bcm_host_deinit) = 					dlsym(_lib,"bcm_host_deinit"))) goto dlError;
	if (!(*(void**)(&wr_vc_dispmanx_display_close) = 		dlsym(_lib,"vc_dispmanx_display_close"))) goto dlError;
	if (!(*(void**)(&wr_vc_dispmanx_display_open) = 		dlsym(_lib,"vc_dispmanx_display_open"))) goto dlError;
	if (!(*(void**)(&wr_vc_dispmanx_display_get_info) =		dlsym(_lib, "vc_dispmanx_display_get_info"))) goto dlError;
	if (!(*(void**)(&wr_vc_dispmanx_resource_create) = 		dlsym(_lib,"vc_dispmanx_resource_create"))) goto dlError;
	if (!(*(void**)(&wr_vc_dispmanx_resource_delete) = 		dlsym(_lib, "vc_dispmanx_resource_delete"))) goto dlError;
	if (!(*(void**)(&wr_vc_dispmanx_resource_read_data) =	dlsym(_lib, "vc_dispmanx_resource_read_data"))) goto dlError;
	if (!(*(void**)(&wr_vc_dispmanx_rect_set) =				dlsym(_lib, "vc_dispmanx_rect_set"))) goto dlError;
	if (!(*(void**)(&wr_vc_dispmanx_snapshot) =				dlsym(_lib, "vc_dispmanx_snapshot"))) goto dlError;

	wr_bcm_host_init();
	return true;

dlError:
	DebugIf(verbose, _log, "dlsym for %s::%s failed, error = %s", library.c_str(), dlerror());
	dlclose(_lib);
    return false;
#else
	return false;
#endif
}

DispmanxFrameGrabber::~DispmanxFrameGrabber()
{
	freeResources();
}

bool DispmanxFrameGrabber::setupScreen()
{
	bool rc (false);

	int deviceIdx (DEFAULT_DEVICE);

	QSize screenSize = getScreenSize(deviceIdx);
	if ( screenSize.isEmpty() )
	{
		Error(_log, "Failed to open display [%d]! Probably no permissions to access the capture interface", deviceIdx);
		setEnabled(false);
	}
	else
	{
		setWidthHeight(screenSize.width(), screenSize.height());
		Info(_log, "Display [%d] opened with resolution: %dx%d", deviceIdx, screenSize.width(), screenSize.height());
		setEnabled(true);
		rc = true;
	}
	return rc;
}

void DispmanxFrameGrabber::freeResources()
{
	delete[] _captureBuffer;

	if (_lib != nullptr)
	{
		// Clean up resources
		wr_vc_dispmanx_resource_delete(_vc_resource);
		// De-init BCM
		wr_bcm_host_deinit();

		dlclose(_lib);
		_lib = nullptr;
	}
}

bool DispmanxFrameGrabber::setWidthHeight(int width, int height)
{
	bool rc = false;
	if(open() && Grabber::setWidthHeight(width, height))
	{
		if(_vc_resource != 0)
		{
			wr_vc_dispmanx_resource_delete(_vc_resource);
		}

		Debug(_log,"Create the resources for capturing image");
		uint32_t vc_nativeImageHandle;
		_vc_resource = wr_vc_dispmanx_resource_create(
			VC_IMAGE_RGBA32,
			width,
			height,
			&vc_nativeImageHandle);
		assert(_vc_resource);

		if (_vc_resource != 0)
		{
			Debug(_log,"Define the capture rectangle with the same size");
			wr_vc_dispmanx_rect_set(&_rectangle, 0, 0, width, height);
			rc = true;
		}
	}
	return rc;
}

void DispmanxFrameGrabber::setFlags(DISPMANX_TRANSFORM_T vc_flags)
{
	_vc_flags = vc_flags;
}

int DispmanxFrameGrabber::grabFrame(Image<ColorRgb> & image)
{
	int rc = 0;
	if (_isEnabled && !_isDeviceInError)
	{
		// vc_dispmanx_resource_read_data doesn't seem to work well
		// with arbitrary positions so we have to handle cropping by ourselves
		int cropLeft   = _cropLeft;
		int cropRight  = _cropRight;
		int cropTop    = _cropTop;
		int cropBottom = _cropBottom;

		if (_vc_flags & DISPMANX_SNAPSHOT_FILL)
		{
			// disable cropping, we are capturing the video overlay window
			Debug(_log,"Disable cropping, as the video overlay window is captured");
			cropLeft = cropRight = cropTop = cropBottom = 0;
		}

		unsigned imageWidth  = static_cast<unsigned>(_width - cropLeft - cropRight);
		unsigned imageHeight = static_cast<unsigned>(_height - cropTop - cropBottom);

		// resize the given image if needed
		if (image.width() != imageWidth || image.height() != imageHeight)
		{
			image.resize(imageWidth, imageHeight);
		}

		if (_image_rgba.width() != imageWidth || _image_rgba.height() != imageHeight)
		{
			_image_rgba.resize(imageWidth, imageHeight);
		}

		// Open the connection to the display
		_vc_display = wr_vc_dispmanx_display_open(DEFAULT_DEVICE);
		if (_vc_display < 0)
		{
			Error(_log, "Cannot open display: %d", DEFAULT_DEVICE);
			rc = -1;
		}
		else {

			// Create the snapshot (incl down-scaling)
			int ret = wr_vc_dispmanx_snapshot(_vc_display, _vc_resource, _vc_flags);
			if (ret < 0)
			{
				Error(_log, "Snapshot failed: %d", ret);
				rc = ret;
			}
			else
			{
				// Read the snapshot into the memory
				void* imagePtr   = _image_rgba.memptr();
				void* capturePtr = imagePtr;

				unsigned imagePitch = imageWidth * sizeof(ColorRgba);

				// dispmanx seems to require the pitch to be a multiple of 64
				unsigned capturePitch = (_rectangle.width * sizeof(ColorRgba) + 63) & (~63);

				// grab to temp buffer if image pitch isn't valid or if we are cropping
				if (imagePitch != capturePitch
					 || static_cast<unsigned>(_rectangle.width) != imageWidth
					 || static_cast<unsigned>(_rectangle.height) != imageHeight)
				{
					// check if we need to resize the capture buffer
					unsigned captureSize = capturePitch * static_cast<unsigned>(_rectangle.height) / sizeof(ColorRgba);
					if (_captureBufferSize != captureSize)
					{
						delete[] _captureBuffer;
						_captureBuffer = new ColorRgba[captureSize];
						_captureBufferSize = captureSize;
					}

					capturePtr = &_captureBuffer[0];
				}

				ret = wr_vc_dispmanx_resource_read_data(_vc_resource, &_rectangle, capturePtr, capturePitch);
				if (ret < 0)
				{
					Error(_log, "vc_dispmanx_resource_read_data failed: %d", ret);
					rc = ret;
				}
				else
				{
					_imageResampler.processImage(static_cast<uint8_t*>(capturePtr),
												  _width,
												  _height,
												  static_cast<int>(capturePitch),
												  PixelFormat::RGB32,
												  image);
				}
			}

			wr_vc_dispmanx_display_close(_vc_display);
		}
	}
	return rc;
}

QSize DispmanxFrameGrabber::getScreenSize(int device) const
{
	int width (0);
	int height(0);

	DISPMANX_DISPLAY_HANDLE_T vc_display = wr_vc_dispmanx_display_open(device);
	if ( vc_display > 0)
	{
		// Obtain the display information
		DISPMANX_MODEINFO_T vc_info;
		int result = wr_vc_dispmanx_display_get_info(vc_display, &vc_info);
		(void)result;

		if (result == 0)
		{
			width = vc_info.width;
			height = vc_info.height;

			DebugIf(verbose, _log, "Display found with resolution: %dx%d", width, height);
		}
		// Close the display
		wr_vc_dispmanx_display_close(vc_display);
	}

	return QSize(width, height);
}

QJsonObject DispmanxFrameGrabber::discover(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QJsonObject inputsDiscovered;
	if (open())
	{
		int deviceIdx (DEFAULT_DEVICE);
		QJsonArray video_inputs;

		QSize screenSize = getScreenSize(deviceIdx);
		if ( !screenSize.isEmpty() )
		{
			QJsonArray fps = { 1, 5, 10, 15, 20, 25, 30, 40, 50, 60 };

			QJsonObject in;

			QString displayName;
			displayName = QString("Screen:%1").arg(deviceIdx);

			in["name"] = displayName;
			in["inputIdx"] = deviceIdx;

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
			inputsDiscovered["device"] = "dispmanx";
			inputsDiscovered["device_name"] = "DispmanX";
			inputsDiscovered["type"] = "screen";
			inputsDiscovered["video_inputs"] = video_inputs;

			QJsonObject defaults, video_inputs_default, resolution_default;
			resolution_default["fps"] = _fps;
			video_inputs_default["resolution"] = resolution_default;
			video_inputs_default["inputIdx"] = 0;
			defaults["video_input"] = video_inputs_default;
			inputsDiscovered["default"] = defaults;
		}

		if (inputsDiscovered.isEmpty())
		{
			DebugIf(verbose, _log, "No displays found to capture from!");
		}
	}

	DebugIf(verbose, _log, "device: [%s]", QString(QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return inputsDiscovered;
}
