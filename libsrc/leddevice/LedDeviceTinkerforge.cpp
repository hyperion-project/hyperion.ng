
// STL includes
#include <cerrno>
#include <cstring>

// Local LedDevice includes
#include "LedDeviceTinkerforge.h"

static const unsigned MAX_NUM_LEDS = 320;
static const unsigned MAX_NUM_LEDS_SETTABLE = 16;

LedDeviceTinkerforge::LedDeviceTinkerforge(const std::string & host, uint16_t port, const std::string & uid, const unsigned interval) :
		LedDevice(),
		_host(host),
		_port(port),
		_uid(uid),
		_interval(interval),
		_ipConnection(nullptr),
		_ledStrip(nullptr),
		_colorChannelSize(0)
{
	// empty
}

LedDeviceTinkerforge::~LedDeviceTinkerforge()
{
	// Close the device (if it is opened)
	if (_ipConnection != nullptr && _ledStrip != nullptr)
	{
		switchOff();
	}

	// Clean up claimed resources
	delete _ipConnection;
	delete _ledStrip;
}

int LedDeviceTinkerforge::open()
{
	// Check if connection is already createds
	if (_ipConnection != nullptr)
	{
		std::cout << "Attempt to open existing connection; close before opening" << std::endl;
		return -1;
	}

	// Initialise a new connection
	_ipConnection = new IPConnection;
	ipcon_create(_ipConnection);

	int connectionStatus = ipcon_connect(_ipConnection, _host.c_str(), _port);
	if (connectionStatus < 0) 
	{
		std::cerr << "Attempt to connect to master brick (" << _host << ":" << _port << ") failed with status " << connectionStatus << std::endl;
		return -1;
	}

	// Create the 'LedStrip'
	_ledStrip = new LEDStrip;
	led_strip_create(_ledStrip, _uid.c_str(), _ipConnection);

	int frameStatus = led_strip_set_frame_duration(_ledStrip, _interval);
	if (frameStatus < 0) 
	{
		std::cerr << "Attempt to connect to led strip bricklet (led_strip_set_frame_duration()) failed with status " << frameStatus << std::endl;
		return -1;
	}

	return 0;
}

int LedDeviceTinkerforge::write(const std::vector<ColorRgb> &ledValues)
{
	unsigned nrLedValues = ledValues.size();

	if (nrLedValues > MAX_NUM_LEDS) 
	{
		std::cerr << "Invalid attempt to write led values. Not more than " << MAX_NUM_LEDS << " leds are allowed." << std::endl;
		return -1;
	}

	if (_colorChannelSize < nrLedValues)
	{
		_redChannel.resize(nrLedValues, uint8_t(0));
		_greenChannel.resize(nrLedValues, uint8_t(0));
		_blueChannel.resize(nrLedValues, uint8_t(0));
	}
	_colorChannelSize = nrLedValues;

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

int LedDeviceTinkerforge::switchOff()
{
	std::fill(_redChannel.begin(),   _redChannel.end(),   0);
	std::fill(_greenChannel.begin(), _greenChannel.end(), 0);
	std::fill(_blueChannel.begin(),  _blueChannel.end(),  0);

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
			std::cerr << "Setting led values failed with status " << status << std::endl;
			return status;
		}
	}

	return E_OK;
}
