#include "LedDeviceFadeCandy.h"

static const signed   MAX_NUM_LEDS    = 10000; // OPC can handle 21845 leds - in theory, fadecandy device should handle 10000 leds
static const unsigned OPC_SET_PIXELS  = 0;     // OPC command codes
static const unsigned OPC_SYS_EX      = 255;     // OPC command codes
static const unsigned OPC_HEADER_SIZE = 4;     // OPC header size

LedDeviceFadeCandy::LedDeviceFadeCandy(const QJsonObject &deviceConfig)
	: LedDevice()
	, _client(nullptr)
{
	_deviceReady = init(deviceConfig);
	_client = new QTcpSocket(this);
}

LedDeviceFadeCandy::~LedDeviceFadeCandy()
{
	_client->close();
}

LedDevice* LedDeviceFadeCandy::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceFadeCandy(deviceConfig);
}

bool LedDeviceFadeCandy::init(const QJsonObject &deviceConfig)
{
	LedDevice::init(deviceConfig);

	if (_ledCount > MAX_NUM_LEDS)
	{
		Error(_log, "fadecandy/opc: Invalid attempt to write led values. Not more than %d leds are allowed.", MAX_NUM_LEDS);
		return false;
	}

	_host        = deviceConfig["output"].toString("127.0.0.1");
	_port        = deviceConfig["port"].toInt(7890);
	_channel     = deviceConfig["channel"].toInt(0);
	_gamma       = deviceConfig["gamma"].toDouble(1.0);
	_noDither    = ! deviceConfig["dither"].toBool(false);
	_noInterp    = ! deviceConfig["interpolation"].toBool(false);
	_manualLED   = deviceConfig["manualLed"].toBool(false);
	_ledOnOff    = deviceConfig["ledOn"].toBool(false);
	_setFcConfig = deviceConfig["setFcConfig"].toBool(false);

	_whitePoint_r = 1.0;
	_whitePoint_g = 1.0;
	_whitePoint_b = 1.0;

	const QJsonArray whitePointConfig = deviceConfig["whitePoint"].toArray();
	if ( !whitePointConfig.isEmpty() && whitePointConfig.size() == 3 )
	{
		_whitePoint_r = whitePointConfig[0].toDouble() / 255.0;
		_whitePoint_g = whitePointConfig[1].toDouble() / 255.0;
		_whitePoint_b = whitePointConfig[2].toDouble() / 255.0;
	}

	_opc_data.resize( _ledRGBCount + OPC_HEADER_SIZE );
	_opc_data[0] = _channel;
	_opc_data[1] = OPC_SET_PIXELS;
	_opc_data[2] = _ledRGBCount >> 8;
	_opc_data[3] = _ledRGBCount & 0xff;

	return true;
}

bool LedDeviceFadeCandy::isConnected()
{
	return _client->state() == QAbstractSocket::ConnectedState;
}

bool LedDeviceFadeCandy::tryConnect()
{
	if (  _client->state() == QAbstractSocket::UnconnectedState ) {
		_client->connectToHost( _host, _port);
		if ( _client->waitForConnected(1000) )
		{
			Info(_log,"fadecandy/opc: connected to %s:%i on channel %i", QSTRING_CSTR(_host), _port, _channel);
			if (_setFcConfig)
			{
				sendFadeCandyConfiguration();
			}
		}
	}

	return isConnected();
}

int LedDeviceFadeCandy::write( const std::vector<ColorRgb> & ledValues )
{
	uint idx = OPC_HEADER_SIZE;
	for (const ColorRgb& color : ledValues)
	{
		_opc_data[idx  ] = unsigned( color.red   );
		_opc_data[idx+1] = unsigned( color.green );
		_opc_data[idx+2] = unsigned( color.blue  );
		idx += 3;
	}

	return ( transferData()<0 ? -1 : 0 );
}

int LedDeviceFadeCandy::transferData()
{
	if (LedDevice::enabled())
		if ( isConnected() || tryConnect() )
			return _client->write( _opc_data, _opc_data.size() );

	return -2;
}

int LedDeviceFadeCandy::sendSysEx(uint8_t systemId, uint8_t commandId, QByteArray msg)
{
	if ( isConnected() )
	{
		QByteArray sysExData;
		ssize_t data_size = msg.size() + 4;
		sysExData.resize( 4 + OPC_HEADER_SIZE );

		sysExData[0] = 0;
		sysExData[1] = OPC_SYS_EX;
		sysExData[2] = data_size >>8;
		sysExData[3] = data_size &0xff;
		sysExData[4] = systemId >>8;
		sysExData[5] = systemId &0xff;
		sysExData[6] = commandId >>8;
		sysExData[7] = commandId &0xff;

		sysExData += msg;

		return _client->write( sysExData, sysExData.size() );
	}
	return -1;
}

void LedDeviceFadeCandy::sendFadeCandyConfiguration()
{
	Debug(_log, "send configuration to fadecandy");
	QString data = "{\"gamma\": "+QString::number(_gamma,'g',4)+", \"whitepoint\": ["+QString::number(_whitePoint_r,'g',4)+", "+QString::number(_whitePoint_g,'g',4)+", "+QString::number(_whitePoint_b,'g',4)+"]}";
	sendSysEx(1, 1, data.toLocal8Bit() );

	char firmware_data = ((uint8_t)_noDither | ((uint8_t)_noInterp << 1) | ((uint8_t)_manualLED << 2) | ((uint8_t)_ledOnOff << 3) );
	sendSysEx(1, 2, QByteArray(1,firmware_data) );
}
