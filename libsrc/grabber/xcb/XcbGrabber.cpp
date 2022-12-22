#include <utils/Logger.h>
#include <grabber/XcbGrabber.h>

#include "XcbCommands.h"
#include "XcbCommandExecutor.h"

#include <xcb/xcb_event.h>

#include <QCoreApplication>

#include <memory>

// Constants
namespace {
	const bool verbose = false;
} //End of constants

#define DOUBLE_TO_FIXED(d) ((xcb_render_fixed_t) ((d) * 65536))

XcbGrabber::XcbGrabber(int cropLeft, int cropRight, int cropTop, int cropBottom)
	: Grabber("XCBGRABBER", cropLeft, cropRight, cropTop, cropBottom)
	, _connection{}
	, _screen{}
	, _pixmap{}
	, _srcFormat{}
	, _dstFormat{}
	, _srcPicture{}
	, _dstPicture{}
	, _transform{}
	, _shminfo{}
	, _screenWidth{}
	, _screenHeight{}
	, _src_x(cropLeft)
	, _src_y(cropTop)
	, _XcbRenderAvailable{}
	, _XcbRandRAvailable{}
	, _XcbShmAvailable{}
	, _XcbShmPixmapAvailable{}
	, _isWayland (false)
	, _logger{}
	, _shmData{}
	, _XcbRandREventBase{-1}
{
	_logger = Logger::getInstance("XCB");

	// cropping is performed by XcbRender, XcbShmGetImage or XcbGetImage
	_useImageResampler = false;
	_imageResampler.setCropping(0, 0, 0, 0);
}

XcbGrabber::~XcbGrabber()
{
	if (_connection != nullptr)
	{
		freeResources();
		xcb_disconnect(_connection);
	}
}

void XcbGrabber::freeResources()
{
	if (_XcbRandRAvailable)
	{
		qApp->removeNativeEventFilter(this);
	}

	if(_XcbShmAvailable)
	{
		query<ShmDetach>(_connection, _shminfo);
		shmdt(_shmData);
		shmctl(_shminfo, IPC_RMID, 0);

	}

	if (_XcbRenderAvailable)
	{
		query<FreePixmap>(_connection, _pixmap);
		query<RenderFreePicture>(_connection, _srcPicture);
		query<RenderFreePicture>(_connection, _dstPicture);
	}
}

void XcbGrabber::setupResources()
{
	if (_XcbRandRAvailable)
	{
		qApp->installNativeEventFilter(this);
	}

	if(_XcbShmAvailable)
	{
		_shminfo = xcb_generate_id(_connection);
		int id = shmget(IPC_PRIVATE, size_t(_width) * size_t(_height) * 4, IPC_CREAT | 0777);
		_shmData = static_cast<uint8_t*>(shmat(id, nullptr, 0));
		query<ShmAttach>(_connection, _shminfo, id, 0);
	}

	if (_XcbRenderAvailable)
	{
		_useImageResampler = false;
		_imageResampler.setHorizontalPixelDecimation(1);
		_imageResampler.setVerticalPixelDecimation(1);

		if(_XcbShmPixmapAvailable)
		{
			_pixmap = xcb_generate_id(_connection);
			query<ShmCreatePixmap>(
				_connection, _pixmap, _screen->root, _width,
				_height, _screen->root_depth, _shminfo, 0);
		}
		else
		{
			_pixmap = xcb_generate_id(_connection);
			query<CreatePixmap>(_connection, _screen->root_depth, _pixmap, _screen->root, _width, _height);
		}

		_srcFormat = findFormatForVisual(_screen->root_visual);
		_dstFormat = findFormatForVisual(_screen->root_visual);

		_srcPicture = xcb_generate_id(_connection);
		_dstPicture = xcb_generate_id(_connection);

		const uint32_t value_mask = XCB_RENDER_CP_REPEAT;
		const uint32_t values[] = { XCB_RENDER_REPEAT_NONE };

		query<RenderCreatePicture>(_connection, _srcPicture, _screen->root, _srcFormat, value_mask, values);
		query<RenderCreatePicture>(_connection, _dstPicture, _pixmap, _dstFormat, value_mask, values);

		const std::string filter = "fast";
		query<RenderSetPictureFilter>(_connection, _srcPicture, filter.size(), filter.c_str(), 0, nullptr);
	}
	else
	{
		_useImageResampler = true;
		_imageResampler.setHorizontalPixelDecimation(_pixelDecimation);
		_imageResampler.setVerticalPixelDecimation(_pixelDecimation);
	}
}

xcb_screen_t * XcbGrabber::getScreen(const xcb_setup_t *setup, int screen_num) const
{
	xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
	xcb_screen_t * screen    = nullptr;

	for (; it.rem > 0; xcb_screen_next(&it))
	{
		if (!screen_num)
		{
			screen = it.data;
			break;
		}
		screen_num--;
	}
	return screen;
}

void XcbGrabber::setupRandr()
{
	auto randrQueryExtensionReply = xcb_get_extension_data(_connection, &xcb_randr_id);
	_XcbRandRAvailable = randrQueryExtensionReply != nullptr;
	_XcbRandREventBase = randrQueryExtensionReply ? randrQueryExtensionReply->first_event : -1;
}

void XcbGrabber::setupRender()
{
	auto renderQueryVersionReply = query<RenderQueryVersion>(_connection, 0, 0);

	_XcbRenderAvailable = renderQueryVersionReply != nullptr;
}

void XcbGrabber::setupShm()
{
	auto shmQueryExtensionReply = xcb_get_extension_data(_connection, &xcb_render_id);
	_XcbShmAvailable = shmQueryExtensionReply != nullptr;

	_XcbShmPixmapAvailable = false;
	if (_XcbShmAvailable)
	{
		auto shmQueryVersionReply = query<ShmQueryVersion>(_connection);

		_XcbShmPixmapAvailable = shmQueryVersionReply ? shmQueryVersionReply->shared_pixmaps : false;
	}
}

bool XcbGrabber::open()
{
	bool rc = false;

	if (getenv("WAYLAND_DISPLAY") != nullptr)
	{
		_isWayland = true;
	}
	else
	{
		_connection = xcb_connect(nullptr, &_screen_num);

		int ret = xcb_connection_has_error(_connection);
		if (ret != 0)
		{
			Debug(_logger, "Cannot open display, error %d", ret);
		}
		else
		{
			const xcb_setup_t * setup = xcb_get_setup(_connection);
			_screen = getScreen(setup, _screen_num);
			if ( _screen != nullptr)
			{
				rc = true;
			}
		}
	}

	return rc;
}

bool XcbGrabber::setupDisplay()
{
	bool result = false;

	if ( ! open() )
	{
		if ( _isWayland  )
		{
			Error(_log, "Grabber does not work under Wayland!");
		}
		else
		{
			if (getenv("DISPLAY") != nullptr)
			{
				Error(_log, "Unable to open display [%s], screen %d does not exist", getenv("DISPLAY"), _screen_num);
			}
			else
			{
				Error(_log, "DISPLAY environment variable not set");
			}
			freeResources();
		}
	}
	else
	{
		setupRandr();
		setupRender();
		setupShm();

		Info(_log, "%s", QSTRING_CSTR(QString("XcbRandR=[%1] XcbRender=[%2] XcbShm=[%3] XcbPixmap=[%4]")
			 .arg(_XcbRandRAvailable ? "available" : "unavailable",
			 _XcbRenderAvailable     ? "available" : "unavailable",
			 _XcbShmAvailable        ? "available" : "unavailable",
			 _XcbShmPixmapAvailable  ? "available" : "unavailable"))
			 );

		result = (updateScreenDimensions(true) >= 0);
		ErrorIf(!result, _log, "XCB Grabber start failed");
		setEnabled(result);
	}
	return result;
}

int XcbGrabber::grabFrame(Image<ColorRgb> & image, bool forceUpdate)
{
	if (!_isEnabled)
		return 0;

	if (forceUpdate)
		updateScreenDimensions(forceUpdate);

	if (_XcbRenderAvailable)
	{
		double scale_x = static_cast<double>(_screenWidth / _pixelDecimation) / static_cast<double>(_screenWidth);
		double scale_y = static_cast<double>(_screenHeight / _pixelDecimation) / static_cast<double>(_screenHeight);
		double scale = qMin(scale_y, scale_x);

		_transform = {
			DOUBLE_TO_FIXED(1), DOUBLE_TO_FIXED(0), DOUBLE_TO_FIXED(0),
			DOUBLE_TO_FIXED(0), DOUBLE_TO_FIXED(1), DOUBLE_TO_FIXED(0),
			DOUBLE_TO_FIXED(0), DOUBLE_TO_FIXED(0), DOUBLE_TO_FIXED(scale)
		};

		query<RenderSetPictureTransform>(_connection, _srcPicture, _transform);
		query<RenderComposite>(_connection,
			XCB_RENDER_PICT_OP_SRC, _srcPicture,
			XCB_RENDER_PICTURE_NONE, _dstPicture,
			(_src_x/_pixelDecimation),
			(_src_y/_pixelDecimation),
			0, 0, 0, 0, _width, _height);

		xcb_flush(_connection);

		if (_XcbShmAvailable)
		{
			query<ShmGetImage>(_connection,
				_pixmap, 0, 0, _width, _height,
				~0, XCB_IMAGE_FORMAT_Z_PIXMAP, _shminfo, 0);

			_imageResampler.processImage(
				reinterpret_cast<const uint8_t *>(_shmData),
				_width, _height, _width * 4, PixelFormat::BGR32, image);
		}
		else
		{
			auto result = query<GetImage>(_connection,
				XCB_IMAGE_FORMAT_Z_PIXMAP, _pixmap,
				0, 0, _width, _height, ~0);

			auto buffer = xcb_get_image_data(result.get());

			_imageResampler.processImage(
				reinterpret_cast<const uint8_t *>(buffer),
				_width, _height, _width * 4, PixelFormat::BGR32, image);
		}

	}
	else if (_XcbShmAvailable)
	{
		query<ShmGetImage>(_connection,
			_screen->root, _src_x, _src_y, _width, _height,
			~0, XCB_IMAGE_FORMAT_Z_PIXMAP, _shminfo, 0);

		_imageResampler.processImage(
			reinterpret_cast<const uint8_t *>(_shmData),
			_width, _height, _width * 4, PixelFormat::BGR32, image);
	}
	else
	{
		auto result = query<GetImage>(_connection,
			XCB_IMAGE_FORMAT_Z_PIXMAP, _screen->root,
			_src_x, _src_y, _width, _height, ~0);

		auto buffer = xcb_get_image_data(result.get());

		_imageResampler.processImage(
			reinterpret_cast<const uint8_t *>(buffer),
			_width, _height, _width * 4, PixelFormat::BGR32, image);
	}

	return 0;
}

int XcbGrabber::updateScreenDimensions(bool force)
{
	auto geometry = query<GetGeometry>(_connection, _screen->root);
	if (geometry == nullptr)
	{
		setEnabled(false);
		Error(_log, "Failed to obtain screen geometry");
		return -1;
	}

	if (!_isEnabled)
		setEnabled(true);

	if (!force && _screenWidth == unsigned(geometry->width) &&
		_screenHeight == unsigned(geometry->height))
		return 0;

	if (_screenWidth || _screenHeight)
		freeResources();

	Info(_log, "Update of screen resolution: [%dx%d]  to [%dx%d]", _screenWidth, _screenHeight, geometry->width, geometry->height);

	_screenWidth  = geometry->width;
	_screenHeight = geometry->height;

	int width = 0, height = 0;

	// Image scaling is performed by XRender when available, otherwise by ImageResampler
	if (_XcbRenderAvailable)
	{
		width  =  (_screenWidth > unsigned(_cropLeft + _cropRight))
			? ((_screenWidth - _cropLeft - _cropRight) / _pixelDecimation)
			: _screenWidth / _pixelDecimation;

		height =  (_screenHeight > unsigned(_cropTop + _cropBottom))
			? ((_screenHeight - _cropTop - _cropBottom) / _pixelDecimation)
			: _screenHeight / _pixelDecimation;

		Info(_log, "Using XcbRender for grabbing [%dx%d]", width, height);
	}
	else
	{
		width  =  (_screenWidth > unsigned(_cropLeft + _cropRight))
			? (_screenWidth - _cropLeft - _cropRight)
			: _screenWidth;

		height =  (_screenHeight > unsigned(_cropTop + _cropBottom))
			? (_screenHeight - _cropTop - _cropBottom)
			: _screenHeight;

		Info(_log, "Using XcbGetImage for grabbing [%dx%d]", width, height);
	}

	// Calculate final image dimensions and adjust top/left cropping in 3D modes
	switch (_videoMode)
	{
	case VideoMode::VIDEO_3DSBS:
		_width  = width /2;
		_height = height;
		_src_x  = _cropLeft / 2;
		_src_y  = _cropTop;
		break;
	case VideoMode::VIDEO_3DTAB:
		_width  = width;
		_height = height / 2;
		_src_x  = _cropLeft;
		_src_y  = _cropTop / 2;
		break;
	case VideoMode::VIDEO_2D:
	default:
		_width  = width;
		_height = height;
		_src_x  = _cropLeft;
		_src_y  = _cropTop;
		break;
	}

	setupResources();

	return 1;
}

void XcbGrabber::setVideoMode(VideoMode mode)
{
	Grabber::setVideoMode(mode);
	if(_connection != nullptr)
	{
		updateScreenDimensions(true);
	}
}

bool XcbGrabber::setPixelDecimation(int pixelDecimation)
{
	bool rc (true);
	if (Grabber::setPixelDecimation(pixelDecimation))
	{
		if(_connection != nullptr)
		{
			if ( updateScreenDimensions(true) < 0 )
			{
				rc = false;
			}
		}
	}
	return rc;
}

void XcbGrabber::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
	if(_connection != nullptr)
		updateScreenDimensions(true);
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
bool XcbGrabber::nativeEventFilter(const QByteArray & eventType, void * message, qintptr * /*result*/)
#else
bool XcbGrabber::nativeEventFilter(const QByteArray & eventType, void * message, long int * /*result*/)
#endif
{
	if (!_XcbRandRAvailable || eventType != "xcb_generic_event_t" || _XcbRandREventBase == -1)
		return false;

	xcb_generic_event_t *e = static_cast<xcb_generic_event_t*>(message);
	const uint8_t xEventType = XCB_EVENT_RESPONSE_TYPE(e);

	if (xEventType == _XcbRandREventBase + XCB_RANDR_SCREEN_CHANGE_NOTIFY)
		updateScreenDimensions(true);

	return false;
}

xcb_render_pictformat_t XcbGrabber::findFormatForVisual(xcb_visualid_t visual) const
{
	auto formats = query<RenderQueryPictFormats>(_connection);
	if (formats == nullptr)
		return {};

#ifdef __APPLE__
	int screen = 0;
#else
	int screen = _screen_num;
#endif
	xcb_render_pictscreen_iterator_t sit =
		xcb_render_query_pict_formats_screens_iterator(formats.get());

	for (; sit.rem; --screen, xcb_render_pictscreen_next(&sit)) {
		if (screen != 0)
			continue;

		xcb_render_pictdepth_iterator_t dit =
			xcb_render_pictscreen_depths_iterator(sit.data);
		for (; dit.rem; xcb_render_pictdepth_next(&dit))
		{
			xcb_render_pictvisual_iterator_t vit
				= xcb_render_pictdepth_visuals_iterator(dit.data);
			for (; vit.rem; xcb_render_pictvisual_next(&vit))
			{
				if (vit.data->visual == visual)
				{
					return vit.data->format;
				}
			}
		}
	}
	return {};
}

QJsonObject XcbGrabber::discover(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QJsonObject inputsDiscovered;
	if ( open() )
	{
		inputsDiscovered["device"] = "xcb";
		inputsDiscovered["device_name"] = "XCB";
		inputsDiscovered["type"] = "screen";

		QJsonArray video_inputs;

		if (_connection != nullptr && _screen != nullptr )
		{
			QJsonArray fps = { 1, 5, 10, 15, 20, 25, 30, 40, 50, 60 };

			const xcb_setup_t * setup = xcb_get_setup(_connection);

			xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
			xcb_screen_t * screen  = nullptr;

			int i = 0;
			// Iterate through all X screens
			for (; it.rem > 0; xcb_screen_next(&it))
			{
				screen = it.data;

				auto geometry = query<GetGeometry>(_connection, screen->root);
				if (geometry == nullptr)
				{
					Debug(_log, "Failed to obtain screen geometry for screen [%d]", i);
				}
				else
				{
					QJsonObject in;

					QString displayName;
					auto property = query<GetProperty>(_connection, 0, screen->root, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, 0);
					if ( property != nullptr )
					{
						if ( xcb_get_property_value_length(property.get()) > 0 )
						{
							displayName = (char *) xcb_get_property_value(property.get());
						}
					}

					if (displayName.isEmpty())
					{
						displayName = QString("Display:%1").arg(i);
					}

					in["name"] = displayName;
					in["inputIdx"] = i;

					QJsonArray formats;
					QJsonArray resolutionArray;
					QJsonObject format;
					QJsonObject resolution;

					resolution["width"] = geometry->width;
					resolution["height"] = geometry->height;
					resolution["fps"] = fps;

					resolutionArray.append(resolution);

					format["resolutions"] = resolutionArray;
					formats.append(format);

					in["formats"] = formats;
					video_inputs.append(in);
				}
				++i;
			}

			if ( !video_inputs.isEmpty() )
			{
				inputsDiscovered["video_inputs"] = video_inputs;

				QJsonObject defaults, video_inputs_default, resolution_default;
				resolution_default["fps"] = _fps;
				video_inputs_default["resolution"] = resolution_default;
				video_inputs_default["inputIdx"] = 0;
				defaults["video_input"] = video_inputs_default;
				inputsDiscovered["default"] = defaults;
			}
		}
	}
	DebugIf(verbose, _log, "device: [%s]", QString(QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return inputsDiscovered;
}
