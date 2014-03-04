// hyperion-v4l2 includes
#include "ImageHandler.h"

ImageHandler::ImageHandler(const std::string & address, int priority, bool skipProtoReply) :
		_priority(priority),
		_connection(address)
{
	_connection.setSkipReply(skipProtoReply);
}

ImageHandler::~ImageHandler()
{
}

void ImageHandler::receiveImage(const Image<ColorRgb> & image)
{
	_connection.setImage(image, _priority, 1000);
}
