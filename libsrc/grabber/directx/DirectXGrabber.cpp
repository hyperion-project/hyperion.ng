#include <windows.h>
#include <grabber/DirectXGrabber.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib,"d3dx9.lib")

DirectXGrabber::DirectXGrabber(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, int display)
	: Grabber("DXGRABBER", 0, 0, cropLeft, cropRight, cropTop, cropBottom)
	, _pixelDecimation(pixelDecimation)
	, _display(unsigned(display))
	, _displayWidth(0)
	, _displayHeight(0)
	, _srcRect(0)
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

	delete _srcRect;
}

bool DirectXGrabber::setupDisplay()
{
	freeResources();

	D3DDISPLAYMODE ddm;
	D3DPRESENT_PARAMETERS d3dpp;
	HMONITOR hMonitor = nullptr;
	MONITORINFO monitorInfo = { 0 };

	if ((_d3d9 = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
	{
		Error(_log, "Failed to create Direct3D");
		return false;
	}

	SecureZeroMemory(&monitorInfo, sizeof(monitorInfo));
	monitorInfo.cbSize = sizeof(MONITORINFO);

	hMonitor = _d3d9->GetAdapterMonitor(_display);
	if (hMonitor == nullptr || GetMonitorInfo(hMonitor, &monitorInfo) == FALSE)
	{
		Info(_log, "Specified display %d is not available. Primary display %d is used", _display, D3DADAPTER_DEFAULT);
		_display = D3DADAPTER_DEFAULT;
	}

	if (FAILED(_d3d9->GetAdapterDisplayMode(_display, &ddm)))
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

	if (FAILED(_d3d9->CreateDevice(_display, D3DDEVTYPE_HAL, nullptr, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &_device)))
	{
		Error(_log, "CreateDevice failed");
		return false;
	}

	if (FAILED(_device->CreateOffscreenPlainSurface(ddm.Width, ddm.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &_surface, nullptr)))
	{
		Error(_log, "CreateOffscreenPlainSurface failed");
		return false;
	}

	int width = _displayWidth / _pixelDecimation;
	int height =_displayHeight / _pixelDecimation;

	// calculate final image dimensions and adjust top/left cropping in 3D modes
	_srcRect = new RECT;
	switch (_videoMode)
	{
		case VideoMode::VIDEO_3DSBS:
			_width         = width / 2;
			_height        = height;
			_srcRect->left = _cropLeft * _pixelDecimation / 2;
			_srcRect->top  = _cropTop * _pixelDecimation;
			_srcRect->right = (_displayWidth / 2) - (_cropRight * _pixelDecimation);
			_srcRect->bottom = _displayHeight - (_cropBottom * _pixelDecimation);
			break;
		case VideoMode::VIDEO_3DTAB:
			_width  = width;
			_height = height / 2;
			_srcRect->left = _cropLeft * _pixelDecimation;
			_srcRect->top  = (_cropTop * _pixelDecimation) / 2;
			_srcRect->right = _displayWidth - (_cropRight * _pixelDecimation);
			_srcRect->bottom = (_displayHeight / 2) - (_cropBottom * _pixelDecimation);
			break;
		case VideoMode::VIDEO_2D:
		default:
			_width  = width;
			_height = height;
			_srcRect->left = _cropLeft * _pixelDecimation;
			_srcRect->top  = _cropTop * _pixelDecimation;
			_srcRect->right = _displayWidth - _cropRight * _pixelDecimation;
			_srcRect->bottom = _displayHeight - _cropBottom * _pixelDecimation;
			break;
	}

	if (FAILED(_device->CreateOffscreenPlainSurface(_width, _height, D3DFMT_R8G8B8, D3DPOOL_SCRATCH, &_surfaceDest, nullptr)))
	{
		Error(_log, "CreateOffscreenPlainSurface failed");
		return false;
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
		Error(_log, "Unable to get Buffer Surface Data");
		setEnabled(setupDisplay());
		return -1;
	}

	D3DXLoadSurfaceFromSurface(_surfaceDest, nullptr, nullptr, _surface, nullptr, _srcRect, D3DX_DEFAULT, 0);

	D3DLOCKED_RECT lockedRect;
	if (FAILED(_surfaceDest->LockRect(&lockedRect, nullptr, D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY)))
	{
		Error(_log, "Unable to lock destination Front Buffer Surface");
		return 0;
	}

	for(int i=0 ; i < _height ; i++)
		memcpy((unsigned char*)image.memptr() + i * _width * 3, (unsigned char*)lockedRect.pBits + i * lockedRect.Pitch, _width * 3);

	for (int idx = 0; idx < _width * _height; idx++)
		image.memptr()[idx] = ColorRgb{image.memptr()[idx].blue, image.memptr()[idx].green, image.memptr()[idx].red};

	if (FAILED(_surfaceDest->UnlockRect()))
	{
		Error(_log, "Unable to unlock destination Front Buffer Surface");
		return 0;
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

void DirectXGrabber::setDisplayIndex(int index)
{
	if(_display != unsigned(index))
	{
		_display = unsigned(index);
		setupDisplay();
	}
}
