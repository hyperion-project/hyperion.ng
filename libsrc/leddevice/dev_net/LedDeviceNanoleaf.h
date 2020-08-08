#ifndef LEDEVICENANOLEAF_H
#define LEDEVICENANOLEAF_H

// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderRestApi.h"
#include "ProviderUdp.h"

// Qt includes
#include <QString>
#include <QNetworkAccessManager>

///
/// Implementation of the LedDevice interface for sending to
/// Nanoleaf devices via network by using the 'external control' protocol.
///
class LedDeviceNanoleaf : public ProviderUdp
{
public:
	///
	/// @brief Constructs LED-device for Nanoleaf LightPanels (aka Aurora) or Canvas
	///
	/// following code shows all configuration options
	/// @code
	/// "device" :
	/// {
	///     "type" : "nanoleaf"
	///     "host" : "hostname or IP",
	///     "token": "Authentication Token",
	/// },
	///@endcode
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceNanoleaf(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LED-device
	///
	virtual ~LedDeviceNanoleaf() override;

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// @brief Discover Nanoleaf devices available (for configuration).
	///
	/// @return A JSON structure holding a list of devices found
	///
	virtual QJsonObject discover() override;

	///
	/// @brief Get the Nanoleaf device's resource properties
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP [:port]",
	///     "token" : "authentication token",
	///     "filter": "resource to query", root "/" is used, if empty
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to query device
	/// @return A JSON structure holding the device's properties
	///
	virtual QJsonObject getProperties(const QJsonObject& params) override;

	///
	/// @brief Send an update to the Nanoleaf device to identify it.
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP [:port]",
	///     "token" : "authentication token",
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to address device
	///
	virtual void identify(const QJsonObject& params) override;

protected:

	///
	/// @brief Initialise the Nanoleaf device's configuration and network address details
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
	virtual int open() override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	//////
	virtual int write(const std::vector<ColorRgb> & ledValues) override;

	///
	/// @brief Power-/turn on the Nanoleaf device.
	///
	/// @brief Store the device's original state.
	///
	virtual bool powerOn() override;

	///
	/// @brief Power-/turn off the Nanoleaf device.
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
	/// @param[in] authentication token
	///
	/// @return True, if success
	///
	bool initRestAPI(const QString &hostname, int port, const QString &token );

	///
	/// @brief Get Nanoleaf device details and configuration
	///
	/// @return True, if Nanoleaf device capabilities fit configuration
	///
	bool initLedsConfiguration();

	///
	/// @brief Change Nanoleaf device to External Control (UDP) mode
	///
	/// @return Response from device
	///@brief
	QJsonDocument changeToExternalControlMode();

	///
	/// @brief Get command to power Nanoleaf device on or off
	///
	/// @param isOn True, if to switch on device
	/// @return Command to switch device on/off
	///
	QString getOnOffRequest (bool isOn ) const;

	///
	/// @brief Convert vector to hex string
	///
	/// @param uint8_t vector
	/// @return vector as string of hex values
	std::string uint8_vector_to_hex_string( const std::vector<uint8_t>& buffer ) const;

	///REST-API wrapper
	ProviderRestApi* _restApi;

	QString _hostname;
	int  _apiPort;
	QString _authToken;

	bool _topDown;
	bool _leftRight;
	uint _startPos;
	uint _endPos;

	//Nanoleaf device details
	QString _deviceModel;
	QString _deviceFirmwareVersion;
	ushort _extControlVersion;

	/// The number of panels with LEDs
	uint _panelLedCount;

	/// Array of the panel ids.
	QVector<uint> _panelIds;

};

#endif // LEDEVICENANOLEAF_H
