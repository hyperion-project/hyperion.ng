#include "LedDeviceWS281x.h"
#include <utils/SysInfo.h>

// Constants
namespace {
const bool verbose = false;
} //End of constants

LedDeviceWS281x::LedDeviceWS281x(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
{
}

LedDevice* LedDeviceWS281x::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceWS281x(deviceConfig);
}

bool LedDeviceWS281x::init(const QJsonObject &deviceConfig)
{

	// Initialise sub-class
	// Initialise sub-class
	if (!LedDevice::init(deviceConfig))
	{
		return false;
	}

	QString whiteAlgorithm = deviceConfig["whiteAlgorithm"].toString("white_off");

	_whiteAlgorithm	= RGBW::stringToWhiteAlgorithm(whiteAlgorithm);
	if (_whiteAlgorithm == RGBW::WhiteAlgorithm::INVALID)
	{
		this->setInError(QString ("unknown whiteAlgorithm: %1").arg(whiteAlgorithm));
		return false;
	}

	_channel = deviceConfig["pwmchannel"].toInt(0);
	if (_channel != 0 && _channel != 1)
	{
		this->setInError("WS281x: invalid PWM channel; must be 0 or 1.");
		return false;
	}

	memset(&_led_string, 0, sizeof(_led_string));
	_led_string.freq   = deviceConfig["freq"].toInt(800000UL);
	_led_string.dmanum = deviceConfig["dma"].toInt(5);
	_led_string.channel[_channel].gpionum    = deviceConfig["gpio"].toInt(18);
	_led_string.channel[_channel].count      = deviceConfig["leds"].toInt(256);
	_led_string.channel[_channel].invert     = deviceConfig["invert"].toInt(0);
	_led_string.channel[_channel].strip_type = (deviceConfig["rgbw"].toBool(false) ? SK6812_STRIP_GRBW : WS2811_STRIP_RGB);
	_led_string.channel[_channel].brightness = 255;

	_led_string.channel[!_channel].gpionum = 0;
	_led_string.channel[!_channel].invert = _led_string.channel[_channel].invert;
	_led_string.channel[!_channel].count = 0;
	_led_string.channel[!_channel].brightness = 0;
	_led_string.channel[!_channel].strip_type = WS2811_STRIP_RGB;

	Debug( _log, "ws281x strip type : %d", _led_string.channel[_channel].strip_type );

	return true;
}

int LedDeviceWS281x::open()
{
	_isDeviceReady = false;

	if (!SysInfo::isUserAdmin())
	{
		QString errortext = QString ("Hyperion must run with \"root\" privileges for this device. Current user is: \"%1\"").arg(SysInfo::userName());
		this->setInError( errortext );
		return -1;
	}

	// Try to open the LedDevice
	ws2811_return_t rc = ws2811_init(&_led_string);
	if ( rc != WS2811_SUCCESS )
	{
		QString errortext = QString ("Failed to open. Error message: %1").arg( ws2811_get_return_t_str(rc) );
		this->setInError( errortext );
		return -1;
	}

	// Everything is OK, device is ready
	_isDeviceReady = true;

	return 0;
}

int LedDeviceWS281x::close()
{
	_isDeviceReady = false;

	// LedDevice specific closing activities
	if ( isInitialised() )
	{
		ws2811_fini(&_led_string);
	}

	return 0;
}

// Send new values down the LED chain
int LedDeviceWS281x::write(const QVector<ColorRgb> &ledValues)
{
	int idx = 0;
	for (const ColorRgb& color : ledValues)
	{
		if (idx >= _led_string.channel[_channel].count)
		{
			break;
		}

		_temp_rgbw.red = color.red;
		_temp_rgbw.green = color.green;
		_temp_rgbw.blue = color.blue;
		_temp_rgbw.white = 0;

		if (_led_string.channel[_channel].strip_type == SK6812_STRIP_GRBW)
		{
			Rgb_to_Rgbw(color, &_temp_rgbw, _whiteAlgorithm);
		}

		_led_string.channel[_channel].leds[idx++] =
			((uint32_t)_temp_rgbw.white << 24) + ((uint32_t)_temp_rgbw.red << 16) + ((uint32_t)_temp_rgbw.green << 8) + _temp_rgbw.blue;

	}
	while (idx < _led_string.channel[_channel].count)
	{
		_led_string.channel[_channel].leds[idx++] = 0;
	}

	return ws2811_render(&_led_string) ? -1 : 0;
}

QJsonObject LedDeviceWS281x::discover(const QJsonObject& /*params*/)
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType);

	QJsonArray deviceList;

	//Indicate the general availability of the device, if hyperion is run under root
	devicesDiscovered.insert("isUserAdmin", SysInfo::isUserAdmin());

	devicesDiscovered.insert("devices", deviceList);

	DebugIf(verbose,_log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return devicesDiscovered;
}
