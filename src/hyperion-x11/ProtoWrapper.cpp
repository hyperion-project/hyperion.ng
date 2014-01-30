
//
#include "ProtoWrapper.h"

ProtoWrapper::ProtoWrapper(const std::string & protoAddress, const bool skipReply) :
	_priority(200),
	_duration_ms(2000),
	_connection(protoAddress)
{
	_connection.setSkipReply(skipReply);
}

void ProtoWrapper::process(const Image<ColorRgb> & image)
{
	std::cout << "Attempt to send screenshot" << std::endl;
	_connection.setImage(image, _priority, _duration_ms);
}
