#ifndef LEDEVICERAZER_H
#define LEDEVICERAZER_H

// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderRestApi.h"

///
/// Implementation of a Razer Chroma LedDevice
/// Supported Razer Chroma device types: Keyboard, Mouse, Headset, Mousepad, Keypad, Chromalink
///
class LedDeviceRazer : public LedDevice
{
public:

	///
	/// @brief Constructs a specific LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceRazer(const QJsonObject& deviceConfig);

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	///
	static LedDevice* construct(const QJsonObject& deviceConfig);

	///
	/// @brief Destructor of the LED-device
	///
	~LedDeviceRazer() override;

protected:

	///
	/// @brief Initialise the device's configuration
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
	/// @brief Closes the output device.
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
	int write(const std::vector<ColorRgb>& ledValues) override;

protected slots:

	///
	/// @brief Write the last data to the LEDs again.
	///
	/// @return Zero on success else negative
	///
	int rewriteLEDs() override;

private:

	///
	/// @brief Initialise the access to the REST-API wrapper
	///
	/// @param[in] host
	/// @param[in] port
	/// @return True, if success
	///
	bool initRestAPI(const QString& hostname, int port);

	///
	/// @brief Check, if Chroma SDK API response failed
	///
	/// @param[in] http response, incl. the response by Chroma SDK in JSON-format
	/// return True, API call failed
	///
	bool checkApiError(const httpResponse& response);

	///REST-API wrapper
	ProviderRestApi* _restApi;

	QString _hostname;
	int		_apiPort;
	QUrl	_uri;

	QString _razerDeviceType;
};

#endif // LEDEVICERAZER_H
