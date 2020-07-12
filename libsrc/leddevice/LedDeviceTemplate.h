#pragma once

// LedDevice includes
#include <leddevice/LedDevice.h>

///
/// Implementation of a LedDevice ...
/// ...
///
class LedDeviceTemplate : public LedDevice
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	explicit LedDeviceTemplate(const QJsonObject &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool init(const QJsonObject &deviceConfig) override;

public slots:
	///
	/// Closes the output device.
	/// Includes switching-off the device and stopping refreshes
	///
	virtual void close() override;

protected:
	///
	/// Opens and initiatialises the output device
	///
	/// @return Zero on succes (i.e. device is ready and enabled) else negative
	///
	virtual int open() override;

	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	//////
	virtual int write(const std::vector<ColorRgb> & ledValues) override;

private:


};
