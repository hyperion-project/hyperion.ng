#include "LedDeviceFadeCandy.h"

#include <QtEndian>

#include <chrono>

// https://docs.microsoft.com/en-us/windows/win32/winprog/windows-data-types#ssize-t
#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

// Constants
namespace {
constexpr std::chrono::milliseconds CONNECT_TIMEOUT{1000};

const int MAX_NUM_LEDS = 10000; // OPC can handle 21845 LEDs - in theory, fadecandy device should handle 10000 LEDs
const int OPC_SET_PIXELS = 0; // OPC command codes
const int OPC_SYS_EX = 255; // OPC command codes
const int OPC_HEADER_SIZE = 4; // OPC header size

// TCP elements
const char CONFIG_HOST[] = "host";
const char CONFIG_PORT[] = "port";

const char DEFAULT_HOST[] = "127.0.0.1";
const int STREAM_DEFAULT_PORT = 7890;

} //End of constants

LedDeviceFadeCandy::LedDeviceFadeCandy(const QJsonObject& deviceConfig)
	: LedDevice(deviceConfig)
	  , _client(nullptr)
	  , _hostName()
	  , _port(STREAM_DEFAULT_PORT)
{
}

LedDeviceFadeCandy::~LedDeviceFadeCandy()
{
	delete _client;
}

LedDevice* LedDeviceFadeCandy::construct(const QJsonObject& deviceConfig)
{
	return new LedDeviceFadeCandy(deviceConfig);
}

bool LedDeviceFadeCandy::init(const QJsonObject& deviceConfig)
{
	bool isInitOK {false};

	if (LedDevice::init(deviceConfig))
	{
		if (getLedCount() > MAX_NUM_LEDS)
		{
			QString errortext = QString("More LED configured than allowed (%1)").arg(MAX_NUM_LEDS);
			this->setInError(errortext);
			isInitOK = false;
		}
		else
		{
			_hostName = _devConfig[ CONFIG_HOST ].toString(DEFAULT_HOST);
			_port = deviceConfig[CONFIG_PORT].toInt(STREAM_DEFAULT_PORT);

			//If host not configured the init fails
			if (_hostName.isEmpty())
			{
				this->setInError("No target hostname nor IP defined");
			}
			else
			{
				_channel = deviceConfig["channel"].toInt(0);
				_gamma = deviceConfig["gamma"].toDouble(1.0);
				_noDither = !deviceConfig["dither"].toBool(false);
				_noInterp = !deviceConfig["interpolation"].toBool(false);
				_manualLED = deviceConfig["manualLed"].toBool(false);
				_ledOnOff = deviceConfig["ledOn"].toBool(false);
				_setFcConfig = deviceConfig["setFcConfig"].toBool(false);

				_whitePoint_r = 1.0;
				_whitePoint_g = 1.0;
				_whitePoint_b = 1.0;

				const QJsonArray whitePointConfig = deviceConfig["whitePoint"].toArray();
				if (!whitePointConfig.isEmpty() && whitePointConfig.size() == 3)
				{
					_whitePoint_r = whitePointConfig[0].toDouble() / 255.0;
					_whitePoint_g = whitePointConfig[1].toDouble() / 255.0;
					_whitePoint_b = whitePointConfig[2].toDouble() / 255.0;
				}

				_opc_data.resize(static_cast<int>(_ledRGBCount) + OPC_HEADER_SIZE);
				_opc_data[0] = static_cast<char>(_channel);
				_opc_data[1] = OPC_SET_PIXELS;
				qToBigEndian<quint16>(static_cast<quint16>(_ledRGBCount), _opc_data.data() + 2);

				isInitOK = true;
			}
		}
	}
	return isInitOK;
}

bool LedDeviceFadeCandy::initNetwork()
{
	bool isInitOK = true;

	if (_client == nullptr)
	{
		_client = new QTcpSocket(this);
	}
	return isInitOK;
}

int LedDeviceFadeCandy::open()
{
	int retval = -1;
	QString errortext;
	_isDeviceReady = false;

	if (initNetwork())
	{
	// Try to open the LedDevice
		if (!tryConnect())
		{
			errortext = QString("Failed to open device.");
			this->setInError(errortext);
		}
		else
		{
			// Everything is OK, device is ready
			_isDeviceReady = true;
			retval = 0;
		}
	}
	return retval;
}

int LedDeviceFadeCandy::close()
{
	int retval = 0;
	_isDeviceReady = false;

	// LedDevice specific closing activities
	if (_client != nullptr)
	{
		_client->close();
		// Everything is OK -> device is closed
	}
	return retval;
}

bool LedDeviceFadeCandy::isConnected() const
{
	bool connected = false;
	if (_client != nullptr)
	{
		connected = _client->state() == QAbstractSocket::ConnectedState;
	}
	return connected;
}

bool LedDeviceFadeCandy::tryConnect()
{
	if (_client != nullptr)
	{
		if (_client->state() == QAbstractSocket::UnconnectedState) {
			_client->connectToHost(_hostName, static_cast<quint16>(_port));
			if (_client->waitForConnected(CONNECT_TIMEOUT.count()))
			{
				Info(_log, "fadecandy/opc: connected to %s:%d on channel %d", QSTRING_CSTR(_hostName), _port, _channel);
				if (_setFcConfig)
				{
					sendFadeCandyConfiguration();
				}
			}
		}
	}
	return isConnected();
}

int LedDeviceFadeCandy::write(const std::vector<ColorRgb>& ledValues)
{
	uint idx = OPC_HEADER_SIZE;
	for (const ColorRgb& color : ledValues)
	{
		_opc_data[idx] = static_cast<char>(color.red);
		_opc_data[idx + 1] = static_cast<char>(color.green);
		_opc_data[idx + 2] = static_cast<char>(color.blue);
		idx += 3;
	}

	int retval = transferData() < 0 ? -1 : 0;
	return retval;
}

qint64 LedDeviceFadeCandy::transferData()
{
	if (isConnected() || tryConnect())
	{
		return _client->write(_opc_data);
	}
	return -2;
}

qint64 LedDeviceFadeCandy::sendSysEx(uint8_t systemId, uint8_t commandId, const QByteArray& msg)
{
	if (isConnected())
	{
		QByteArray sysExData;
		int data_size = msg.size() + 4;
		sysExData.resize(4 + OPC_HEADER_SIZE);

		sysExData[0] = 0;
		sysExData[1] = static_cast<char>(OPC_SYS_EX);

		qToBigEndian<quint16>(static_cast<quint16>(data_size), sysExData.data() + 2);
		qToBigEndian<quint16>(static_cast<quint16>(systemId), sysExData.data() + 4);
		qToBigEndian<quint16>(static_cast<quint16>(commandId), sysExData.data() + 6);

		sysExData += msg;

		return _client->write(sysExData, sysExData.size());
	}
	return -1;
}

void LedDeviceFadeCandy::sendFadeCandyConfiguration()
{
	Debug(_log, "send configuration to fadecandy");
	QString data = "{\"gamma\": " + QString::number(_gamma, 'g', 4) + ", \"whitepoint\": [" + QString::number(_whitePoint_r, 'g', 4) + ", " + QString::number(_whitePoint_g, 'g', 4) + ", " + QString::number(_whitePoint_b, 'g', 4) + "]}";
	sendSysEx(1, 1, data.toLocal8Bit());

	char firmware_data = static_cast<char>(static_cast<uint8_t>(_noDither) | (static_cast<uint8_t>(_noInterp) << 1) | (static_cast<uint8_t>(_manualLED) << 2) | (static_cast<uint8_t>(_ledOnOff) << 3));
	sendSysEx(1, 2, QByteArray(1, firmware_data));
}
