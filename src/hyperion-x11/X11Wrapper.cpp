
// Hyperion-X11 includes
#include "X11Wrapper.h"

X11Wrapper::X11Wrapper(int grabInterval, int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation) :
    _timer(this),
    _grabber(cropLeft, cropRight, cropTop, cropBottom, horizontalPixelDecimation, verticalPixelDecimation)
{
    _timer.setSingleShot(false);
    _timer.setInterval(grabInterval);

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
    _timer.start();
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
