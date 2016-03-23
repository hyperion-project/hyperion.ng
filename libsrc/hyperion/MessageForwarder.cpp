// STL includes
#include <stdexcept>

#include <hyperion/MessageForwarder.h>


MessageForwarder::MessageForwarder()
{
}

MessageForwarder::~MessageForwarder()
{
}


void MessageForwarder::addJsonSlave(std::string slave)
{
	QStringList parts = QString(slave.c_str()).split(":");
	if (parts.size() != 2)
		throw std::runtime_error(QString("HYPERION (forwarder) ERROR: Wrong address: unable to parse address (%1)").arg(slave.c_str()).toStdString());

	bool ok;
	quint16 port = parts[1].toUShort(&ok);
	if (!ok)
		throw std::runtime_error(QString("HYPERION (forwarder) ERROR: Wrong address: Unable to parse the port number (%1)").arg(parts[1]).toStdString());

	JsonSlaveAddress c;
	c.addr = QHostAddress(parts[0]);
	c.port = port;
	_jsonSlaves << c;
}

void MessageForwarder::addProtoSlave(std::string slave)
{
	_protoSlaves << QString(slave.c_str());
}

QStringList MessageForwarder::getProtoSlaves()
{
	return _protoSlaves;
}

QList<MessageForwarder::JsonSlaveAddress> MessageForwarder::getJsonSlaves()
{
	return _jsonSlaves;
}

bool MessageForwarder::protoForwardingEnabled()
{
	return ! _protoSlaves.empty();
}
