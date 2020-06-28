
#include <QElapsedTimer>

#include <utils/Image.h>
#include <utils/ColorRgb.h>

// X11 includes
#include <X11/Xlib.h>
#include <X11/Xutil.h>

void foo_1(int pixelDecimation)
{
	int cropWidth  = 0;
	int cropHeight = 0;

	Image<ColorRgb> image(64, 64);

	 /// Reference to the X11 display (nullptr if not opened)
	Display * x11Display;

	const char * display_name = nullptr;
	x11Display = XOpenDisplay(display_name);

	std::cout << "Opened display: " << x11Display << std::endl;

	XWindowAttributes window_attributes_return;
	XGetWindowAttributes(x11Display, DefaultRootWindow(x11Display), &window_attributes_return);

	int screenWidth  = window_attributes_return.width;
	int screenHeight = window_attributes_return.height;
	std::cout << "[" << screenWidth << "x" << screenHeight <<"]" << std::endl;

	// Update the size of the buffer used to transfer the screenshot
	int width  = (screenWidth  - 2 * cropWidth  + pixelDecimation/2) / pixelDecimation;
	int height = (screenHeight - 2 * cropHeight + pixelDecimation/2) / pixelDecimation;
	image.resize(width, height);

	const int croppedWidth  = screenWidth  - 2*cropWidth;
	const int croppedHeight = screenHeight - 2*cropHeight;

	QElapsedTimer timer;
	timer.start();

	XImage * xImage = XGetImage(x11Display, DefaultRootWindow(x11Display), cropWidth, cropHeight, croppedWidth, croppedHeight, AllPlanes, ZPixmap);

	std::cout << "Captured image: " << xImage << std::endl;

	// Copy the capture XImage to the local image (and apply required decimation)
	ColorRgb * outputPtr = image.memptr();
	for (int iY=(pixelDecimation/2); iY<croppedHeight; iY+=pixelDecimation)
	{
		for (int iX=(pixelDecimation/2); iX<croppedWidth; iX+=pixelDecimation)
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

	std::cout << "Time required: " << timer.elapsed() << " ms" << std::endl;

	XCloseDisplay(x11Display);
}

void foo_2(int pixelDecimation)
{
	int cropWidth  = 0;
	int cropHeight = 0;

	Image<ColorRgb> image(64, 64);

	 /// Reference to the X11 display (nullptr if not opened)
	Display * x11Display;

	const char * display_name = nullptr;
	x11Display = XOpenDisplay(display_name);

	XWindowAttributes window_attributes_return;
	XGetWindowAttributes(x11Display, DefaultRootWindow(x11Display), &window_attributes_return);

	int screenWidth  = window_attributes_return.width;
	int screenHeight = window_attributes_return.height;
	std::cout << "[" << screenWidth << "x" << screenHeight <<"]" << std::endl;

	// Update the size of the buffer used to transfer the screenshot
	int width  = (screenWidth  - 2 * cropWidth  + pixelDecimation/2) / pixelDecimation;
	int height = (screenHeight - 2 * cropHeight + pixelDecimation/2) / pixelDecimation;
	image.resize(width, height);

	const int croppedWidth  = screenWidth  - 2*cropWidth;
	const int croppedHeight = screenHeight - 2*cropHeight;

	QElapsedTimer timer;
	timer.start();

	// Copy the capture XImage to the local image (and apply required decimation)
	ColorRgb * outputPtr = image.memptr();
	for (int iY=(pixelDecimation/2); iY<croppedHeight; iY+=pixelDecimation)
	{
		for (int iX=(pixelDecimation/2); iX<croppedWidth; iX+=pixelDecimation)
		{
			XImage * xImage = XGetImage(x11Display, DefaultRootWindow(x11Display), iX, iY, 1, 1, AllPlanes, ZPixmap);
			// Extract the pixel from the X11-image
			const uint32_t pixel = uint32_t(XGetPixel(xImage, 0, 0));

			// Assign the color value
			outputPtr->red   = uint8_t((pixel >> 16) & 0xff);
			outputPtr->green = uint8_t((pixel >> 8)  & 0xff);
			outputPtr->blue  = uint8_t((pixel >> 0)  & 0xff);

			// Move to the next output pixel
			++outputPtr;

			// Cleanup allocated resources of the X11 grab
			XDestroyImage(xImage);
		}
	}
	std::cout << "Time required: " << timer.elapsed() << " ms" << std::endl;


	XCloseDisplay(x11Display);
}

int main()
{
	foo_1(10);
	foo_2(10);
	return 0;
}
