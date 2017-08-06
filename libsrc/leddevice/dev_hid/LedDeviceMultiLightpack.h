#pragma once

// stl includes
#include <vector>
#include <cstdint>
#include <QStringList>
#include <QString>

// libusb include
#include <libusb.h>

// Hyperion includes
#include <leddevice/LedDevice.h>
#include "LedDeviceLightpack.h"

///
/// LedDevice implementation for multiple lightpack devices
///
class LedDeviceMultiLightpack : public LedDevice
{
public:
	///
	/// Constructs specific LedDevice
	///
	LedDeviceMultiLightpack(const QJsonObject &);

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~LedDeviceMultiLightpack();

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Opens and configures the output device7
	///
	/// @return Zero on succes else negative
	///
	int open();

	///
	/// Switch the leds off
	///
	/// @return Zero on success else negative
	///
	virtual int switchOff();

private:
	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb>& ledValues);

	static QStringList getLightpackSerials();
	static QString getString(libusb_device * device, int stringDescriptorIndex);

	/// buffer for led data
	std::vector<LedDeviceLightpack *> _lightpacks;
};
