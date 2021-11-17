#ifndef LEDEVICERAZER_H
#define LEDEVICERAZER_H

// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderRestApi.h"

namespace Chroma
{
	typedef enum DeviceType
	{
		DEVICE_KEYBOARD = 0,
		DEVICE_MOUSE = 1,
		DEVICE_HEADSET = 2,
		DEVICE_MOUSEPAD = 3,
		DEVICE_KEYPAD = 4,
		DEVICE_CHROMALINK = 5,
		DEVICE_SYSTEM = 6,
		DEVICE_SPEAKERS = 7,
		DEVICE_INVALID
	} DEVICETYPE;

	typedef enum CustomEffectType
	{
		CHROMA_CUSTOM = 1,
		CHROMA_CUSTOM2 = 2,
	} CUSTOM_EFFECT_TYPE;

	const int MAX_ROW = 30;      //!< Maximum rows for custom effects.
	const int MAX_COLUMN = 30;   //!< Maximum columns for custom effects.
	const int MAX_LEDS = MAX_ROW * MAX_COLUMN;

	namespace Keyboard
	{
		const char TYPE_NAME[] = "keyboard";
		const int MAX_ROW = 6;
		const int MAX_COLUMN = 22;
		const CustomEffectType CUSTOM_EFFECT_TYPE = CHROMA_CUSTOM;
	}
	namespace Mouse
	{
		const char TYPE_NAME[] = "mouse";
		const int MAX_ROW = 9;
		const int MAX_COLUMN = 7;
		const CustomEffectType CUSTOM_EFFECT_TYPE = CHROMA_CUSTOM2;
	}
	namespace Headset
	{
		const char TYPE_NAME[] = "headset";
		const int MAX_ROW = 1;
		const int MAX_COLUMN = 5;
		const CustomEffectType CUSTOM_EFFECT_TYPE = CHROMA_CUSTOM;
	}
	namespace Mousepad
	{
		const char TYPE_NAME[] = "mousepad";
		const int MAX_ROW = 1;
		const int MAX_COLUMN = 15;
		const CustomEffectType CUSTOM_EFFECT_TYPE = CHROMA_CUSTOM;
	}
	namespace Keypad
	{
		const char TYPE_NAME[] = "keypad";
		const int MAX_ROW = 4;
		const int MAX_COLUMN = 5;
		const CustomEffectType CUSTOM_EFFECT_TYPE = CHROMA_CUSTOM;
	}
	namespace Chromalink
	{
		const char TYPE_NAME[] = "chromalink";
		const int MAX_ROW = 1;
		const int MAX_COLUMN = 5;
		const CustomEffectType CUSTOM_EFFECT_TYPE = CHROMA_CUSTOM;
	}

	const QStringList SupportedDevices{
		Keyboard::TYPE_NAME,
		Mouse::TYPE_NAME,
		Headset::TYPE_NAME,
		Mousepad::TYPE_NAME,
		Keypad::TYPE_NAME,
		Chromalink::TYPE_NAME
	};
}

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
	/// @brief Get a Razer device's resource properties
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "subType"  : "razer_device_type",
	/// }
	/// @endcode
	/// @param[in] params Parameters to query device
	/// @return A JSON structure holding the device's properties
	///
	QJsonObject getProperties(const QJsonObject& params) override;

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

	///
	/// @brief Update object with properties for a given device
	///
	/// @param[in] deviceType
	/// return True, if success
	///
	bool resolveDeviceProperties(const QString& deviceType);

	///REST-API wrapper
	ProviderRestApi* _restApi;

	QString _hostname;
	int		_apiPort;
	QUrl	_uri;

	QString _razerDeviceType;
	int _maxRow;
	int _maxColumn;
	int _maxLeds;
	Chroma::CustomEffectType _customEffectType;

	bool _isSingleColor;
};

#endif // LEDEVICERAZER_H
