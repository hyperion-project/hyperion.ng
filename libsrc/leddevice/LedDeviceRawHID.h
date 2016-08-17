#pragma once

// STL includes
#include <string>

// Qt includes
#include <QTimer>

// hyperion include
#include "LedHIDDevice.h"

///
/// Implementation of the LedDevice interface for writing to an RawHID led device.
///
class LedDeviceRawHID : public LedHIDDevice
{
	Q_OBJECT

public:
	///
	/// Constructs the LedDevice for attached RawHID device
	///
	LedDeviceRawHID(const unsigned short VendorId, const unsigned short ProductId, int delayAfterConnect_ms);

	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	/// Switch the leds off
	virtual int switchOff();

private slots:
	/// Write the last data to the leds again
	void rewriteLeds();

private:
	/// Timer object which makes sure that led data is written at a minimum rate
	/// The RawHID device will switch off when it does not receive data at least
	/// every 15 seconds
	QTimer _timer;
};
