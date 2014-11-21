// STL includes
#include <iostream>
#include <cstdint>

// X11 includes
#include <X11/Xutil.h>

// X11Grabber includes
#include <grabber/X11Grabber.h>

X11Grabber::X11Grabber(const unsigned cropHorizontal, const unsigned cropVertical, const unsigned pixelDecimation) :
    _pixelDecimation(pixelDecimation),
    _cropWidth(cropHorizontal),
    _cropHeight(cropVertical),
    _x11Display(nullptr),
    _screenWidth(0),
    _screenHeight(0),
    _image(0,0)
{
    // empty
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

    const int croppedWidth  = _screenWidth  - 2*_cropWidth;
    const int croppedHeight = _screenHeight - 2*_cropHeight;

    // Capture the current screen
    XImage * xImage = XGetImage(_x11Display, DefaultRootWindow(_x11Display), _cropWidth, _cropHeight, croppedWidth, croppedHeight, AllPlanes, ZPixmap);
    if (xImage == nullptr)
    {
        std::cerr << "Grab failed" << std::endl;
        return _image;
    }

    // Copy the capture XImage to the local image (and apply required decimation)
    ColorRgb * outputPtr = _image.memptr();
    for (int iY=(_pixelDecimation/2); iY<croppedHeight; iY+=_pixelDecimation)
    {
        for (int iX=(_pixelDecimation/2); iX<croppedWidth; iX+=_pixelDecimation)
        {
            // Extract the pixel from the X11-image
            const uint32_t pixel = uint32_t(XGetPixel(xImage, iX, iY));

            // Assign the color value
            outputPtr->red   = uint8_t((pixel >> 16) & 0xff);
            outputPtr->green = uint8_t((pixel >> 8)  & 0xff);
            outputPtr->blue  = uint8_t((pixel >> 0)  & 0xff);

            // Move to the next output pixel
            ++outputPtr;
        }
    }
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

    // Update the size of the buffer used to transfer the screenshot
    int width  = (_screenWidth  - 2 * _cropWidth  + _pixelDecimation/2) / _pixelDecimation;
    int height = (_screenHeight - 2 * _cropHeight + _pixelDecimation/2) / _pixelDecimation;
    _image.resize(width, height);

    return 0;
}
