// hyperion-v4l2 includes
#include "ImageHandler.h"

ImageHandler::ImageHandler(const std::string & address, int priority, double signalThreshold, bool skipProtoReply) :
		_priority(priority),
		_connection(address),
		_signalThreshold(signalThreshold),
		_signalProcessor(100, 50, 0, uint8_t(std::min(255, std::max(0, int(255*signalThreshold)))))
{
	_connection.setSkipReply(skipProtoReply);
}

void ImageHandler::receiveImage(const Image<ColorRgb> & image)
{
	// check if we should do signal detection
	if (_signalThreshold < 0)
	{
		_connection.setImage(image, _priority, 1000);
	}
	else
	{
		if (_signalProcessor.process(image))
		{
			std::cout << "Signal state = " << (_signalProcessor.getCurrentBorder().unknown ? "off" : "on") << std::endl;
		}

		// consider an unknown border as no signal
		// send the image to Hyperion if we have a signal
		if (!_signalProcessor.getCurrentBorder().unknown)
		{
			_connection.setImage(image, _priority, 1000);
		}
	}
}

void ImageHandler::imageCallback(void *arg, const Image<ColorRgb> &image)
{
	ImageHandler * handler = static_cast<ImageHandler *>(arg);
	handler->receiveImage(image);
}

