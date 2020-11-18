#ifndef LEDEVICEFACTORY_H
#define LEDEVICEFACTORY_H

class LedDevice;
class QJsonObject;

///
/// The LedDeviceFactory is responsible for constructing 'LedDevices'
///
class LedDeviceFactory
{
public:
	///
	/// Constructs a LedDevice based on the given configuration
	///
	/// @param deviceConfig The configuration of the led-device
	///
	/// @return The constructed LedDevice or nullptr if configuration is invalid. The ownership of
	/// the constructed LedDevice is transferred to the caller
	///
	static LedDevice * construct(const QJsonObject & deviceConfig);
};

#endif // LEDEVICEFACTORY_H
