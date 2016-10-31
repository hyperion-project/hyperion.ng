#include "LedDeviceUdpH801.h"

LedDeviceUdpH801::LedDeviceUdpH801(const QJsonObject &deviceConfig)
	: ProviderUdp()
{
	_deviceReady = init(deviceConfig);
}

bool LedDeviceUdpH801::init(const QJsonObject &deviceConfig)
{
	/* The H801 port is fixed */
	_LatchTime_ns = 10000000;
	_port = 30977;
	ProviderUdp::init(deviceConfig, "255.255.255.255");

	_ids.clear();
	QJsonArray lArray = deviceConfig["lightIds"].toArray();
	for (int i = 0; i < lArray.size(); i++)
	{
		QString id = lArray[i].toString();
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

LedDevice* LedDeviceUdpH801::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpH801(deviceConfig);
}

int LedDeviceUdpH801::write(const std::vector<ColorRgb> &ledValues)
{
	ColorRgb color = ledValues[0];
	_message[_prefix_size + 0] = color.red;
	_message[_prefix_size + 1] = color.green;
	_message[_prefix_size + 2] = color.blue;

	return writeBytes(_message.size(), reinterpret_cast<const uint8_t*>(_message.data()));
}
