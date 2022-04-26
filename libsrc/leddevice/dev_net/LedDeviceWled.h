#ifndef LEDDEVICEWLED_H
#define LEDDEVICEWLED_H

// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderRestApi.h"
#include "LedDeviceUdpDdp.h"
#include "LedDeviceUdpRaw.h"

///
/// Implementation of a WLED-device
///
class LedDeviceWled : public LedDeviceUdpDdp, LedDeviceUdpRaw
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
	~LedDeviceWled() override;

	///
	/// @brief Constructs the WLED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// @brief Discover WLED devices available (for configuration).
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params) override;

	///
	/// @brief Get the WLED device's resource properties
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP",
	///     "filter": "resource to query", root "/" is used, if empty
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to query device
	/// @return A JSON structure holding the device's properties
	///
	QJsonObject getProperties(const QJsonObject& params) override;

	///
	/// @brief Send an update to the WLED device to identify it.
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP",
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to address device
	///
	void identify(const QJsonObject& params) override;

protected:

	///
	/// @brief Initialise the WLED device's configuration and network address details
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Closes the UDP device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	int close() override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> & ledValues) override;

	///
	/// @brief Power-/turn on the WLED device.
	///
	/// @return True if success
	///
	bool powerOn() override;

	///
	/// @brief Power-/turn off the WLED device.
	///
	/// @return True if success
	///
	bool powerOff() override;

	///
	/// @brief Store the device's original state.
	///
	/// Save the device's state before hyperion color streaming starts allowing to restore state during switchOff().
	///
	/// @return True if success
	///
	bool storeState() override;

	///
	/// @brief Restore the device's original state.
	///
	/// Restore the device's state as before hyperion color streaming started.
	/// This includes the on/off state of the device.
	///
	/// @return True, if success
	///
	bool restoreState() override;

private:

	///
	/// @brief Initialise the access to the REST-API wrapper
	///
	/// @return True, if success
	///
	bool openRestAPI();

	///
	/// @brief Get command to power WLED-device on or off
	///
	/// @param isOn True, if to switch on device
	/// @return Command to switch device on/off
	///
	QString getOnOffRequest (bool isOn ) const;

	QString getBrightnessRequest (int bri ) const;
	QString getEffectRequest(int effect, int speed=128) const;
	QString getLorRequest(int lor) const;
	QString getUdpnRequest(bool send, bool recv) const;

	bool sendStateUpdateRequest(const QString &request);

	QString resolveAddress (const QString& hostName);

	///REST-API wrapper
	ProviderRestApi* _restApi;

	QString _hostAddress;
	int		_apiPort;

	QJsonObject _originalStateProperties;

	bool _isBrightnessOverwrite;
	int _brightness;

	bool _isSyncOverwrite;
	bool _originalStateUdpnSend;
	bool _originalStateUdpnRecv;

	bool _isStreamDDP;
};

#endif // LEDDEVICEWLED_H
