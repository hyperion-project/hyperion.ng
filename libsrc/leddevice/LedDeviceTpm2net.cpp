// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>
#include <sstream>

// Local-Hyperion includes
#include "LedDeviceTpm2net.h"


LedDeviceTpm2net::LedDeviceTpm2net(const Json::Value &deviceConfig)
	: LedDevice()
	, _socket(this)
{
	setConfig(deviceConfig);
}

LedDeviceTpm2net::~LedDeviceTpm2net()
{
}

bool LedDeviceTpm2net::setConfig(const Json::Value &deviceConfig)
{
	_host = QHostAddress(QString::fromStdString(deviceConfig.get("output", "127.0.0.1").asString()));
	_port = deviceConfig.get("port", 65506).asInt();

	return true;
}

LedDevice* LedDeviceTpm2net::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceTpm2net(deviceConfig);
}

int LedDeviceTpm2net::write(const std::vector<ColorRgb> & ledValues)
{
	if (_ledBuffer.size() == 0)
	{
		_ledBuffer.resize(7 + 3*ledValues.size());
		_ledBuffer[0] = 0x9c; // block-start byte TPM.NET
		_ledBuffer[1] = 0xDA;
		_ledBuffer[2] = ((3 * ledValues.size()) >> 8) & 0xFF; // frame size high byte
		_ledBuffer[3] = (3 * ledValues.size()) & 0xFF; // frame size low byte
		_ledBuffer[4] = 1; // packets number
		_ledBuffer[5] = 1; // Number of packets 
		_ledBuffer[(int)(7 + 3*ledValues.size()-1)] = 0x36; // block-end byte
	}

	// write data
	memcpy(6 + _ledBuffer.data(), ledValues.data() /*Max 1,490 bytes*/, ledValues.size() * 3);

	_socket.connectToHost(_host, _port);
	_socket.write((const char *)_ledBuffer.data());
	_socket.close();

	return 0;
}

int LedDeviceTpm2net::switchOff()
{
	memset(6 + _ledBuffer.data(), 0, _ledBuffer.size() - 5);
	return 0;
}
