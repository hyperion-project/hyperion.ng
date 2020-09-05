
// STL includes
#include <cerrno>
#include <cstring>

// Local LedDevice includes
#include "LedDeviceTinkerforge.h"

// Constants
namespace {

const unsigned MAX_NUM_LEDS = 320;
const unsigned MAX_NUM_LEDS_SETTABLE = 16;
const uint16_t DEFAULT_PORT = 4223;

} //End of constants

LedDeviceTinkerforge::LedDeviceTinkerforge(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	  ,_port(DEFAULT_PORT)
	  ,_ipConnection(nullptr)
	  ,_ledStrip(nullptr)
	  ,_colorChannelSize(0)
{
}

LedDeviceTinkerforge::~LedDeviceTinkerforge()
{
	// Clean up claimed resources
	delete _ipConnection;
	delete _ledStrip;
}

LedDevice* LedDeviceTinkerforge::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceTinkerforge(deviceConfig);
}

bool LedDeviceTinkerforge::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( LedDevice::init(deviceConfig) )
	{
		_host     = deviceConfig["output"].toString("127.0.0.1");
		_port     = deviceConfig["port"].toInt(DEFAULT_PORT);
		_uid      = deviceConfig["uid"].toString();
		_interval = deviceConfig["rate"].toInt();

		if ((unsigned)_ledCount > MAX_NUM_LEDS)
		{
			QString errortext = QString ("Initialization error. Not more than %1 LEDs are allowed.").arg(MAX_NUM_LEDS);
			this->setInError(errortext);
			isInitOK = false;
	}
		else
		{
			if (_colorChannelSize < (unsigned)_ledCount)
			{
				_redChannel.resize(_ledCount, uint8_t(0));
				_greenChannel.resize(_ledCount, uint8_t(0));
				_blueChannel.resize(_ledCount, uint8_t(0));
			}
			_colorChannelSize = _ledCount;
			isInitOK = true;
		}
	}
	return isInitOK;
}

int LedDeviceTinkerforge::open()
{
	int retval = -1;
	_isDeviceReady = false;

	// Check if connection is already created
	if (_ipConnection != nullptr)
	{
		Error(_log, "Attempt to open existing connection; close before opening");
	}
	else
	{
		// Initialise a new connection
		_ipConnection = new IPConnection;
		ipcon_create(_ipConnection);

		int connectionStatus = ipcon_connect(_ipConnection, QSTRING_CSTR(_host), _port);
		if (connectionStatus < 0)
		{
			Error(_log, "Attempt to connect to master brick (%s:%d) failed with status %d", QSTRING_CSTR(_host), _port, connectionStatus);
		}
		else
		{
			// Create the 'LedStrip'
			_ledStrip = new LEDStrip;
			led_strip_create(_ledStrip, QSTRING_CSTR(_uid), _ipConnection);

			int frameStatus = led_strip_set_frame_duration(_ledStrip, _interval);
			if (frameStatus < 0)
			{
				Error(_log,"Attempt to connect to led strip bricklet (led_strip_set_frame_duration()) failed with status %d", frameStatus);
			}
			else
			{
				// Everything is OK, device is ready
				_isDeviceReady = true;
				retval = 0;
			}
		}
	}
	// On error/exceptions, set LedDevice in error
	if ( retval < 0 )
	{
		this->setInError( "Error opening device!" );
	}
	return retval;
}

int LedDeviceTinkerforge::write(const std::vector<ColorRgb> &ledValues)
{
	auto redIt   = _redChannel.begin();
	auto greenIt = _greenChannel.begin();
	auto blueIt  = _blueChannel.begin();

	for (const ColorRgb &ledValue : ledValues)
	{
		*redIt = ledValue.red;
		++redIt;
		*greenIt = ledValue.green;
		++greenIt;
		*blueIt = ledValue.blue;
		++blueIt;
	}

	return transferLedData(_ledStrip, 0, _colorChannelSize, _redChannel.data(), _greenChannel.data(), _blueChannel.data());
}

int LedDeviceTinkerforge::transferLedData(LEDStrip *ledStrip, unsigned index, unsigned length, uint8_t *redChannel, uint8_t *greenChannel, uint8_t *blueChannel)
{
	if (length == 0 || index >= length || length > MAX_NUM_LEDS)
	{
		return E_INVALID_PARAMETER;
	}

	uint8_t reds[MAX_NUM_LEDS_SETTABLE];
	uint8_t greens[MAX_NUM_LEDS_SETTABLE];
	uint8_t blues[MAX_NUM_LEDS_SETTABLE];

	for (unsigned i=index; i<length; i+=MAX_NUM_LEDS_SETTABLE)
	{
		const unsigned copyLength = (i + MAX_NUM_LEDS_SETTABLE > length) ? length - i : MAX_NUM_LEDS_SETTABLE;
		memcpy(reds,   redChannel   + i,   copyLength);
		memcpy(greens, greenChannel + i, copyLength);
		memcpy(blues,  blueChannel  + i,  copyLength);

		const int status = led_strip_set_rgb_values(ledStrip, i, copyLength, reds, greens, blues);
		if (status != E_OK)
		{
			Warning(_log, "Setting led values failed with status %d", status);
			return status;
		}
	}

	return E_OK;
}
