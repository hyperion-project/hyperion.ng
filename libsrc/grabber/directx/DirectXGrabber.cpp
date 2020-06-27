#include <windows.h>
#include <grabber/DirectXGrabber.h>
#include <QImage>
#pragma comment(lib, "d3d9.lib")

DirectXGrabber::DirectXGrabber(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, int display)
	: Grabber("DXGRABBER", 0, 0, cropLeft, cropRight, cropTop, cropBottom)
	, _pixelDecimation(pixelDecimation)
	, _displayWidth(0)
	, _displayHeight(0)
	, _src_x(0)
	, _src_y(0)
	, _src_x_max(0)
	, _src_y_max(0)
	, _d3d9(nullptr)
	, _device(nullptr)
	, _surface(nullptr)
{

	// init
	setupDisplay();
}

DirectXGrabber::~DirectXGrabber()
{
	freeResources();
}

void DirectXGrabber::freeResources()
{
	if (_surface)
		_surface->Release();

	if (_device)
		_device->Release();

	if (_d3d9)
		_d3d9->Release();
}

bool DirectXGrabber::setupDisplay()
{
	freeResources();

	D3DDISPLAYMODE ddm;
	D3DPRESENT_PARAMETERS d3dpp;

	if ((_d3d9 = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
	{
		Error(_log, "Failed to create Direct3D");
		return false;
	}

	if (FAILED(_d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &ddm)))
	{
		Error(_log, "Failed to get current display mode");
		return false;
	}

	SecureZeroMemory(&d3dpp, sizeof(D3DPRESENT_PARAMETERS));

	d3dpp.Windowed = TRUE;
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	d3dpp.BackBufferFormat = ddm.Format;
	d3dpp.BackBufferHeight = _displayHeight = ddm.Height;
	d3dpp.BackBufferWidth = _displayWidth = ddm.Width;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = nullptr;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	if (FAILED(_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &_device)))
	{
		Error(_log, "CreateDevice failed");
		return false;
	}

	if (FAILED(_device->CreateOffscreenPlainSurface(ddm.Width, ddm.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &_surface, nullptr)))
	{
		Error(_log, "CreateOffscreenPlainSurface failed");
		return false;
	}

	int width = (_displayWidth > unsigned(_cropLeft + _cropRight))
		? ((_displayWidth - _cropLeft - _cropRight) / _pixelDecimation)
		: (_displayWidth / _pixelDecimation);

	int height = (_displayHeight > unsigned(_cropTop + _cropBottom))
		? ((_displayHeight - _cropTop - _cropBottom) / _pixelDecimation)
		: (_displayHeight / _pixelDecimation);


	// calculate final image dimensions and adjust top/left cropping in 3D modes
	switch (_videoMode)
	{
		case VIDEO_3DSBS:
			_width  = width / 2;
			_height = height;
			_src_x = _cropLeft / 2;
			_src_y = _cropTop;
			_src_x_max = (_displayWidth / 2) - _cropRight;
			_src_y_max = _displayHeight - _cropBottom;
			break;
		case VIDEO_3DTAB:
			_width  = width;
			_height = height / 2;
			_src_x = _cropLeft;
			_src_y = _cropTop / 2;
			_src_x_max = _displayWidth - _cropRight;
			_src_y_max = (_displayHeight / 2) - _cropBottom;
			break;
		case VIDEO_2D:
		default:
			_width  = width;
			_height = height;
			_src_x = _cropLeft;
			_src_y = _cropTop;
			_src_x_max = _displayWidth - _cropRight;
			_src_y_max = _displayHeight - _cropBottom;
			break;
	}

	Info(_log, "Update output image resolution to [%dx%d]", _width, _height);
	return true;
}

int DirectXGrabber::grabFrame(Image<ColorRgb> & image)
{
	if (!_enabled)
		return 0;

	if (FAILED(_device->GetFrontBufferData(0, _surface)))
	{
		// reinit, this will disable capture on failure
		setEnabled(setupDisplay());
		return -1;
	}

	D3DLOCKED_RECT lockedRect;
	if (FAILED(_surface->LockRect(&lockedRect, nullptr, D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY)))
		return 0;

    QImage frame = QImage(reinterpret_cast<const uchar*>(lockedRect.pBits), _displayWidth, _displayHeight, QImage::Format_RGB32).copy( _src_x, _src_y, _src_x_max, _src_y_max).scaled(_width, _height);
	_surface->UnlockRect();

	for (int y = 0; y < frame.height(); ++y)
		for (int x = 0; x < frame.width(); ++x)
		{
			QColor inPixel(frame.pixel(x, y));
			ColorRgb & outPixel = image(x, y);
			outPixel.red   = inPixel.red();
			outPixel.green = inPixel.green();
			outPixel.blue  = inPixel.blue();
		}

	return 0;
}

void DirectXGrabber::setVideoMode(VideoMode mode)
{
	Grabber::setVideoMode(mode);
	setupDisplay();
}

void DirectXGrabber::setPixelDecimation(int pixelDecimation)
{
	_pixelDecimation = pixelDecimation;
}

void DirectXGrabber::setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom)
{
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
	setupDisplay();
}
