#pragma once

// STL includes
#include <vector>
#include <map>
#include <cstdint>
#include <limits>

// QT includes
#include <QList>
#include <QStringList>

// Utils includes
#include <utils/ColorRgb.h>
class MessageForwarder
{
public:
	MessageForwarder();
	~MessageForwarder();
	
	void addJsonSlave(std::string slave);
	void addProtoSlave(std::string slave);
	void sendMessage();
	
	QStringList getProtoSlaves();

private:
	bool _running;

	QStringList _protoSlaves;
};
