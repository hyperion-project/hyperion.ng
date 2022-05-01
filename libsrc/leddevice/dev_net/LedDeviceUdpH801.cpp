#include "LedDeviceUdpH801.h"
#include <utils/NetUtils.h>

// Constants
namespace {

const char CONFIG_HOST[] = "host";
const char CONFIG_PORT[] = "port";

const ushort H801_DEFAULT_PORT = 30977;
const char H801_DEFAULT_HOST[] = "255.255.255.255";

} //End of constants

LedDeviceUdpH801::LedDeviceUdpH801(const QJsonObject &deviceConfig)
	: ProviderUdp(deviceConfig)
{
}

LedDevice* LedDeviceUdpH801::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpH801(deviceConfig);
}

bool LedDeviceUdpH801::init(const QJsonObject &deviceConfig)
{
	bool isInitOK {false};

	/* The H801 port is fixed */
	_latchTime_ms = 10;

	// Initialise sub-class
	if ( ProviderUdp::init(deviceConfig) )
	{
		_hostName = _devConfig[ CONFIG_HOST ].toString(H801_DEFAULT_HOST);
		_port = deviceConfig[CONFIG_PORT].toInt(H801_DEFAULT_PORT);

		_ids.clear();
		QJsonArray lArray = deviceConfig["lightIds"].toArray();
		for (int i = 0; i < lArray.size(); i++)
		{
			QString id = lArray[i].toString();
			_ids.push_back(id.toInt(nullptr, 16));
		}

		_message = QByteArray(_prefix_size + _colors + _id_size * _ids.size() + _suffix_size, 0x00);
		_message[0] = static_cast<char>(0xFB);
		_message[1] = static_cast<char>(0xEB);

		for (int i = 0; i < _ids.length(); i++) {
			_message[_prefix_size + _colors + i * _id_size + 0] = (_ids[i] >> 0x00) & 0xFF;
			_message[_prefix_size + _colors + i * _id_size + 1] = (_ids[i] >> 0x08) & 0xFF;
			_message[_prefix_size + _colors + i * _id_size + 2] = (_ids[i] >> 0x10) & 0xFF;
		}
		isInitOK = true;
	}
	return isInitOK;
}

int LedDeviceUdpH801::open()
{
	int retval = -1;
	_isDeviceReady = false;

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address))
	{
		if (ProviderUdp::open() == 0)
		{
			// Everything is OK, device is ready
			_isDeviceReady = true;
			retval = 0;
		}
	}
	return retval;
}

int LedDeviceUdpH801::write(const std::vector<ColorRgb> &ledValues)
{
	ColorRgb color = ledValues[0];
	_message[_prefix_size + 0] = color.red;
	_message[_prefix_size + 1] = color.green;
	_message[_prefix_size + 2] = color.blue;

	return writeBytes(_message.size(), reinterpret_cast<const uint8_t*>(_message.data()));
}
