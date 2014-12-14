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
    _image(0,0)
{
    _imageResampler.setHorizontalPixelDecimation(horizontalPixelDecimation);
    _imageResampler.setVerticalPixelDecimation(verticalPixelDecimation);
    _imageResampler.setCropping(0, 0, 0, 0); // cropping is performed by XGetImage
}

X11Grabber::~X11Grabber()
{
    if (_x11Display != nullptr)
    {
        XCloseDisplay(_x11Display);
    }
}

int X11Grabber::open()
{
    const char * display_name = nullptr;
    _x11Display = XOpenDisplay(display_name);

    if (_x11Display == nullptr)
    {
        std::cerr << "Failed to open the default X11-display" << std::endl;
        return -1;
    }

    return 0;
}

Image<ColorRgb> & X11Grabber::grab()
{
    if (_x11Display == nullptr)
    {
        open();
    }

    updateScreenDimensions();

    const unsigned croppedWidth  = _screenWidth - _cropLeft - _cropRight;
    const unsigned croppedHeight = _screenHeight - _cropTop - _cropBottom;

    // Capture the current screen
    XImage * xImage = XGetImage(_x11Display, DefaultRootWindow(_x11Display), _cropLeft, _cropTop, croppedWidth, croppedHeight, AllPlanes, ZPixmap);
    if (xImage == nullptr)
    {
        std::cerr << "Grab failed" << std::endl;
        return _image;
    }

    _imageResampler.processImage(reinterpret_cast<const uint8_t *>(xImage->data), xImage->width, xImage->height, xImage->bytes_per_line, PIXELFORMAT_BGR32, _image);

    // Cleanup allocated resources of the X11 grab
    XDestroyImage(xImage);

    return _image;
}

int X11Grabber::updateScreenDimensions()
{
    XWindowAttributes window_attributes_return;
    const Status status = XGetWindowAttributes(_x11Display, DefaultRootWindow(_x11Display), &window_attributes_return);
    if (status == 0)
    {
        std::cerr << "Failed to obtain window attributes" << std::endl;
        return -1;
    }

    if (_screenWidth == unsigned(window_attributes_return.width) && _screenHeight == unsigned(window_attributes_return.height))
    {
        // No update required
        return 0;
    }
    std::cout << "Update of screen resolution: [" << _screenWidth << "x" << _screenHeight <<"] => ";
    _screenWidth  = window_attributes_return.width;
    _screenHeight = window_attributes_return.height;
    std::cout << "[" << _screenWidth << "x" << _screenHeight <<"]" << std::endl;

    return 0;
}
