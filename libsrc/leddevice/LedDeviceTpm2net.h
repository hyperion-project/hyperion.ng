#pragma once

// STL includes
#include <fstream>
#include <string>
#include <QUdpSocket>
#include <QStringList>
// Leddevice includes
#include <leddevice/LedDevice.h>

///
class LedDeviceTpm2net : public LedDevice
{
public:
	///
	/// Constructs the test-device, which opens an output stream to the file
	///
	LedDeviceTpm2net(const Json::Value &deviceConfig);

	virtual ~LedDeviceTpm2net();
	
	bool setConfig(const Json::Value &deviceConfig);

	static LedDevice* construct(const Json::Value &deviceConfig);

	///
	/// Writes the given led-color values to the output stream
	///
	/// @param ledValues The color-value per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	/// Switch the leds off
	virtual int switchOff();

private:
	/// The host of the master brick
	QHostAddress _host;

	/// The port of the master brick
	uint16_t _port;
	QUdpSocket _socket;
};
