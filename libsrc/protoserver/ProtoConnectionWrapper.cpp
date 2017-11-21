// protoserver includes
#include "protoserver/ProtoConnectionWrapper.h"

ProtoConnectionWrapper::ProtoConnectionWrapper(const QString &address,
											   int priority,
											   int duration_ms,
											   bool skipProtoReply)
	: _priority(priority)
	, _duration_ms(duration_ms)
	, _connection(address)
{
	_connection.setSkipReply(skipProtoReply);
	connect(&_connection, SIGNAL(setVideoMode(VideoMode)), this, SIGNAL(setVideoMode(VideoMode)));
}

ProtoConnectionWrapper::~ProtoConnectionWrapper()
{
}

void ProtoConnectionWrapper::receiveImage(const Image<ColorRgb> & image)
{
	_connection.setImage(image, _priority, _duration_ms);
}
