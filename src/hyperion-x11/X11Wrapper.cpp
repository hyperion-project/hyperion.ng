
// Hyperion-X11 includes
#include "X11Wrapper.h"

X11Wrapper::X11Wrapper(const unsigned cropHorizontal, const unsigned cropVertical, const unsigned pixelDecimation) :
    _timer(this),
    _grabber(cropHorizontal, cropVertical, pixelDecimation)
{
    // Connect capturing to the timeout signal of the timer
    connect(&_timer, SIGNAL(timeout()), this, SLOT(capture()));
}

const Image<ColorRgb> & X11Wrapper::getScreenshot()
{
    const Image<ColorRgb> & screenshot = _grabber.grab();
    return screenshot;
}

void X11Wrapper::start()
{
    _timer.start(100);
}

void X11Wrapper::stop()
{
    _timer.stop();
}

void X11Wrapper::capture()
{
    const Image<ColorRgb> & screenshot = _grabber.grab();
    emit sig_screenshot(screenshot);
}
