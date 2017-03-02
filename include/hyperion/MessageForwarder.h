#pragma once

// STL includes
#include <vector>
#include <map>
#include <cstdint>
#include <limits>

// QT includes
#include <QList>
#include <QStringList>
#include <QHostAddress>

// Utils includes
#include <utils/ColorRgb.h>
class MessageForwarder
{
public:

	struct JsonSlaveAddress {
		QHostAddress addr;
		quint16 port;
	};

	MessageForwarder();
	~MessageForwarder();
	
	void addJsonSlave(QString slave);
	void addProtoSlave(QString slave);

	bool protoForwardingEnabled();
	bool jsonForwardingEnabled();
	bool forwardingEnabled() { return jsonForwardingEnabled() || protoForwardingEnabled(); };
	QStringList getProtoSlaves();
	QList<MessageForwarder::JsonSlaveAddress> getJsonSlaves();

private:
	QStringList               _protoSlaves;
	QList<MessageForwarder::JsonSlaveAddress>   _jsonSlaves;
};
