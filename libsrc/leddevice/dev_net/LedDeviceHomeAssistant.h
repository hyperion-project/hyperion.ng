#ifndef LEDEVICEHOMEASSISTANT_H
#define LEDEVICEHOMEASSISTANT_H

// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderRestApi.h"

// Qt includes
#include <QString>
#include <QHostAddress>
#include <QNetworkAccessManager>

///
/// Implementation of the LedDevice interface for sending to
/// lights made available via the Home Assistant platform.
///
class LedDeviceHomeAssistant : LedDevice
{
public:
	///
	/// @brief Constructs LED-device for Home Assistant Lights
	///
	/// following code shows all configuration options
	/// @code
	/// "device" :
	/// {
	///     "type" : "homeassistant"
	///     "host" : "hostname or IP",
	///     "port"  : port
	///     "token": "bearer token",
	/// },
	///@endcode
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceHomeAssistant(const QJsonObject& deviceConfig);

	///
	/// @brief Destructor of the LED-device
	///
	~LedDeviceHomeAssistant() override;

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	static LedDevice* construct(const QJsonObject& deviceConfig);

	///
	/// @brief Discover Home Assistant lights available (for configuration).
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params) override;

	///
	/// @brief Get the Home Assistant light's resource properties
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP",
	///     "port"  : port
	///     "token" : "bearer token",
	///     "filter": "resource to query", root "/" is used, if empty
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to query device
	/// @return A JSON structure holding the device's properties
	///
	QJsonObject getProperties(const QJsonObject& params) override;

	///
	/// @brief Send an update to the Nanoleaf device to identify it.
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP",
	///     "port"  : port
	///     "token" : "bearer token",
	///     "entity_id": array of lightIds
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to address device
	///
	void identify(const QJsonObject& params) override;

protected:

	///
	/// @brief Initialise the Home Assistant light's configuration and network address details
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject& deviceConfig) override;

	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Writes the RGB-Color values to the Home Assistant light.
	///
	/// @param[in] ledValues The RGB-color
	/// @return Zero on success, else negative
	//////
	int write(const std::vector<ColorRgb>& ledValues) override;

	///
	/// @brief Power-/turn on the Home Assistant light.
	///
	/// @brief Store the device's original state.
	///
	bool powerOn() override;

	///
	/// @brief Power-/turn off the Home Assistant light.
	///
	/// @return True if success
	///
	bool powerOff() override;

private:

	///
	/// @brief Initialise the access to the REST-API wrapper
	///
	/// @return True, if success
	///
	bool openRestAPI();

	///
	/// @brief Get Nanoleaf device details and configuration
	///
	/// @return True, if Nanoleaf device capabilities fit configuration
	///
	bool initLedsConfiguration();

	///
	/// @brief Discover Home Assistant lights available (for configuration).
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonArray discoverSsdp() const;

	// ///
	// /// @brief Get number of panels that can be used as LEds.
	// ///
	// /// @return Number of usable LED panels
	// ///
	// int getHwLedCount(const QJsonObject& jsonLayout) const;

	QString      _hostName;
	QHostAddress _address;
	ProviderRestApi* _restApi;
	int	_apiPort;
	QString _bearerToken;

	/// List of the HA light entity_ids.
	QStringList _lightEntityIds;

	bool _isBrightnessOverwrite;
	bool _isFullBrightnessAtStart;
	int _brightness;
	bool _switchOffOnBlack;
	/// Transition time in seconds
	double _transitionTime;

};

#endif // LEDEVICEHOMEASSISTANT_H
