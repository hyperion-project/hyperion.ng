// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

#include <QHostInfo>

// hyperion local includes
#include "LedDeviceUdpH801.h"

LedDeviceUdpH801::LedDeviceUdpH801(const Json::Value &deviceConfig)
	: ProviderUdp(deviceConfig)
{
	setConfig(deviceConfig);
}

bool LedDeviceUdpH801::setConfig(const Json::Value &deviceConfig)
{
	/* The H801 port is fixed */
	ProviderUdp::setConfig(deviceConfig, 30977, "255.255.255.255");
	/* 10ms seems to be a safe default for the wait time */
	_LatchTime_ns = deviceConfig.get("latchtime", 10000000).asInt();

	_ids.clear();
	for (Json::Value::ArrayIndex i = 0; i < deviceConfig["lightIds"].size(); i++) {
		QString id(deviceConfig["lightIds"][i].asCString());
		_ids.push_back(id.toInt(nullptr, 16));
	}

	_message = QByteArray(_prefix_size + _colors + _id_size * _ids.size() + _suffix_size, 0x00);
	_message[0] = 0xFB;
	_message[1] = 0xEB;

	for (int i = 0; i < _ids.length(); i++) {
		_message[_prefix_size + _colors + i * _id_size + 0] = (_ids[i] >> 0x00) & 0xFF;
		_message[_prefix_size + _colors + i * _id_size + 1] = (_ids[i] >> 0x08) & 0xFF;
		_message[_prefix_size + _colors + i * _id_size + 2] = (_ids[i] >> 0x10) & 0xFF;
	}

	Debug(_log, "H801 using %s:%d", _address.toString().toStdString().c_str(), _port);

	return true;
}

LedDevice* LedDeviceUdpH801::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceUdpH801(deviceConfig);
}

int LedDeviceUdpH801::write(const std::vector<ColorRgb> &ledValues)
{
	ColorRgb color = ledValues[0];
	_message[_prefix_size + 0] = color.red;
	_message[_prefix_size + 1] = color.green;
	_message[_prefix_size + 2] = color.blue;

	writeBytes(_message.size(), reinterpret_cast<const uint8_t*>(_message.data()));
}

int LedDeviceUdpH801::switchOff()
{
	return write(std::vector<ColorRgb>(_ledCount, ColorRgb{0, 0, 0}));
}

