// STL includes
#include <iostream>
#include <cstdint>

// X11 includes
#include <X11/Xutil.h>

// X11Grabber includes
#include <grabber/X11Grabber.h>

X11Grabber::X11Grabber(int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation) :
    _imageResampler(),
    _cropLeft(cropLeft),
    _cropRight(cropRight),
    _cropTop(cropTop),
    _cropBottom(cropBottom),
    _x11Display(nullptr),
    _screenWidth(0),
    _screenHeight(0),
    _croppedWidth(0),
    _croppedHeight(0),
    _image(0,0)
{
    _imageResampler.setHorizontalPixelDecimation(horizontalPixelDecimation);
    _imageResampler.setVerticalPixelDecimation(verticalPixelDecimation);
    _imageResampler.setCropping(0, 0, 0, 0); // cropping is performed by XShmGetImage
}

X11Grabber::~X11Grabber()
{
    if (_x11Display != nullptr)
    {
	freeResources();
        XCloseDisplay(_x11Display);
    }
}

void X11Grabber::freeResources()
{
    // Cleanup allocated resources of the X11 grab
    XShmDetach(_x11Display, &_shminfo);
    XDestroyImage(_xImage);
    shmdt(_shminfo.shmaddr);
    shmctl(_shminfo.shmid, IPC_RMID, 0);
}

void X11Grabber::setupResources()
{
    _xImage = XShmCreateImage(_x11Display, _windowAttr.visual,
                _windowAttr.depth, ZPixmap, NULL, &_shminfo,
                _croppedWidth, _croppedHeight);
    
    _shminfo.shmid = shmget(IPC_PRIVATE, _xImage->bytes_per_line * _xImage->height, IPC_CREAT|0777);
    _shminfo.shmaddr = _xImage->data = (char*)shmat(_shminfo.shmid,0,0);
    _shminfo.readOnly = False;
    
    XShmAttach(_x11Display, &_shminfo);
}

bool X11Grabber::Setup()
{
    _x11Display = XOpenDisplay(NULL);
    if (_x11Display == nullptr)
    {
      std::cerr << "Unable to open display";
      if (getenv("DISPLAY"))
	std::cerr <<  " " << std::string(getenv("DISPLAY")) << std::endl;
      else
	std::cerr << ". DISPLAY environment variable not set" << std::endl;
      return false;
     }
     
    _window = DefaultRootWindow(_x11Display);

    return true;
 }

Image<ColorRgb> & X11Grabber::grab()
{
    updateScreenDimensions();
    
    XShmGetImage(_x11Display, _window, _xImage, _cropLeft, _cropTop, 0x00FFFFFF);
    if (_xImage == nullptr)
    {
        std::cerr << "Grab failed" << std::endl;
        return _image;
    }

    _imageResampler.processImage(reinterpret_cast<const uint8_t *>(_xImage->data), _xImage->width, _xImage->height, _xImage->bytes_per_line, PIXELFORMAT_BGR32, _image);

    return _image;
}

int X11Grabber::updateScreenDimensions()
{
    const Status status = XGetWindowAttributes(_x11Display, _window, &_windowAttr);
    if (status == 0)
    {
        std::cerr << "Failed to obtain window attributes" << std::endl;
        return -1;
    }

    if (_screenWidth == unsigned(_windowAttr.width) && _screenHeight == unsigned(_windowAttr.height))
    {
        // No update required
        return 0;
    }
    
    std::cout << "Update of screen resolution: [" << _screenWidth << "x" << _screenHeight <<"] => ";

    if (_screenWidth || _screenHeight)
      freeResources();
    
    _screenWidth  = _windowAttr.width;
    _screenHeight = _windowAttr.height;
    
    std::cout << "[" << _screenWidth << "x" << _screenHeight <<"]" << std::endl;
    
    if (_screenWidth > unsigned(_cropLeft + _cropRight))
      _croppedWidth  = _screenWidth - _cropLeft - _cropRight;
    else
      _croppedWidth  = _screenWidth;
    
    if (_screenHeight > unsigned(_cropTop + _cropBottom))
      _croppedHeight = _screenHeight - _cropTop - _cropBottom;
    else
      _croppedHeight = _screenHeight;
    
    setupResources();

    return 0;
}
