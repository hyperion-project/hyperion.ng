#include <hyperion/MessageForwarder.h>


MessageForwarder::MessageForwarder() :
_running(false)
{
}

MessageForwarder::~MessageForwarder()
{
}


void MessageForwarder::addJsonSlave(std::string slave)
{
	std::cout << slave << std::endl;
}

void MessageForwarder::addProtoSlave(std::string slave)
{
	_protoSlaves << QString(slave.c_str());
}

void MessageForwarder::sendMessage()
{
	if ( ! _running )
		return;

}

QStringList MessageForwarder::getProtoSlaves()
{
	return _protoSlaves;
}
