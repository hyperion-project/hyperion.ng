#ifndef LEDDEVICEWLED_H
#define LEDDEVICEWLED_H

// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderRestApi.h"
#include "ProviderUdp.h"

///
/// Implementation of a WLED-device
/// ...
///
///
class LedDeviceWled : public ProviderUdp
{

public:
	///
	/// @brief Constructs a WLED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceWled(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the WLED-device
	///
	virtual ~LedDeviceWled() override;

	///
	/// @brief Constructs the WLED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// @brief Discover WLED devices available (for configuration).
	///
	/// @return A JSON structure holding a list of devices found
	///
	virtual QJsonObject discover() override;

	///
	/// @brief Get the WLED device's resource properties
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP [:port]",
	///     "filter": "resource to query", root "/" is used, if empty
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to query device
	/// @return A JSON structure holding the device's properties
	///
	virtual QJsonObject getProperties(const QJsonObject& params) override;

	///
	/// @brief Send an update to the WLED device to identify it.
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP [:port]",
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to address device
	///
	virtual void identify(const QJsonObject& params) override;

protected:

	///
	/// @brief Initialise the WLED device's configuration and network address details
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	virtual bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues) override;

	///
	/// @brief Power-/turn on the WLED device.
	///
	/// @brief Store the device's original state.
	///
	virtual bool powerOn() override;

	///
	/// @brief Power-/turn off the WLED device.
	///
	/// @return True if success
	///
	virtual bool powerOff() override;

private:

	///
	/// @brief Initialise the access to the REST-API wrapper
	///
	/// @param[in] host
	/// @param[in] port
	/// @return True, if success
	///
	bool initRestAPI(const QString &hostname, const int port );

	///
	/// @brief Get command to power WLED-device on or off
	///
	/// @param isOn True, if to switch on device
	/// @return Command to switch device on/off
	///
	QString getOnOffRequest (bool isOn ) const;

	///REST-API wrapper
	ProviderRestApi* _restApi;

	QString _hostname;
	int		_apiPort;
};

#endif // LEDDEVICEWLED_H
