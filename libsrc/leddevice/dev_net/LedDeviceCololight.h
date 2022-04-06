#ifndef LEDEVICECOLOLIGHT_H
#define LEDEVICECOLOLIGHT_H

// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderUdp.h"

enum appID {
	TL1_CMD = 0x00,
	DIRECT_CONTROL = 0x01,
	TRANSMIT_FILE = 0x02,
	CLEAR_FILES = 0x03,
	WRITE_FILE = 0x04,
	READ_FILE = 0x05,
	MODIFY_SECU = 0x06
};

enum effect : uint32_t {
	SAVANNA = 0x04970400,
	SUNRISE = 0x01c10a00,
	UNICORNS = 0x049a0e00,
	PENSIEVE = 0x04c40600,
	THE_CIRCUS = 0x04810130,
	INSTASHARE = 0x03bc0190,
	EIGTHIES = 0x049a0000,
	CHERRY_BLOS = 0x04940800,
	RAINBOW = 0x05bd0690,
	TEST = 0x03af0af0,
	CHRISTMAS = 0x068b0900
};

enum verbs {
	GET = 0x03,
	SET = 0x04,
	SETEEPROM = 0x07,
	SETVAR = 0x0b
};

enum commandTypes {
	STATE_OFF = 0x80,
	STATE_ON = 0x81,
	BRIGTHNESS = 0xCF,
	SETCOLOR = 0xFF
};

enum idxTypes {
	BRIGTHNESS_CONTROL = 0x01,
	COLOR_CONTROL = 0x02,
	COLOR_DIRECT_CONTROL = 0x81,
	READ_INFO_FROM_STORAGE = 0x86
};

	enum bufferMode {
		MONOCROME = 0x01,
		LIGHTBEAD = 0x02,
		};

enum ledLayout {
	STRIP_LAYOUT,
	MODLUE_LAYOUT
};

enum modelType {
	STRIP,
	PLUS
};

const uint8_t PACKET_HEADER[] =
	{
		'S', 'Z',   // Tag "SZ"
		0x30, 0x30, // Version "00"
		0x00, 0x00, // AppID, 0x0000 = TL1 command mode
		0x00, 0x00, 0x00, 0x00 // Size
};

const uint8_t PACKET_SECU[] =
	{
		0x00, 0x00, 0x00, 0x00, // Dict
		0x00, 0x00, 0x00, 0x00, // Sum
		0x00, 0x00, 0x00, 0x00, // Salt
		0x00, 0x00, 0x00, 0x00 // SN
};

const uint8_t TL1_CMD_FIXED_PART[] =
	{
		0x00, 0x00, 0x00, 0x00, // DISTID
		0x00, 0x00, 0x00, 0x00, // SRCID
		0x00, // SECU
		0x00, // VERB
		0x00, // CTAG
		0x00 // LENGTH
};

///
/// Implementation of a Cololight LedDevice
///
class LedDeviceCololight : public ProviderUdp
{
public:

	///
	/// @brief Constructs a Cololight LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceCololight(const QJsonObject& deviceConfig);

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	///
	static LedDevice* construct(const QJsonObject& deviceConfig);

	///
	/// @brief Discover Cololight devices available (for configuration).
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params) override;

	///
	/// @brief Get a Cololight device's resource properties
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP",
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to query device
	/// @return A JSON structure holding the device's properties
	///
	QJsonObject getProperties(const QJsonObject& params) override;

	///
	/// @brief Send an update to the Cololight device to identify it.
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
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb>& ledValues) override;

	///
	/// @brief Power-/turn on the Cololight device.
	///
	/// @return True if success
	///
	bool powerOn() override;

	///
	/// @brief Power-/turn off the Cololight device.
	///
	/// @return True if success
	///
	bool powerOff() override;

private:

	bool initLedsConfiguration();
	void initDirectColorCmdTemplate();

	///
	/// @brief Read additional information from Cololight
	///
	/// @return True if success
	///
	bool getInfo();

	///
	/// @brief Set a Cololight effect
	///
	/// @param[in] effect from effect list
	///
	/// @return True if success
	///
	bool setEffect(const effect effect);

	///
	/// @brief Set a color
	///
	/// @param[in] color in RGB
	///
	/// @return True if success
	///
	bool setColor(const ColorRgb colorRgb);

	///
	/// @brief Set a color (or effect)
	///
	/// @param[in] color in four bytes (red, green, blue, mode)
	///
	/// @return True if success
	///
	bool setColor(const uint32_t color);

	///
	/// @brief Set colors per LED as per given list
	///
	/// @param[in] list of color per LED
	///
	/// @return True if success
	///
	bool setColor(const std::vector<ColorRgb>& ledValues);

	///
	/// @brief Set the Cololight device in TL1 command mode
	///
	/// @param[in] isOn, Enable TL1 command mode = true
	///
	/// @return True if success
	///
	bool setTL1CommandMode(bool isOn);

	///
	/// @brief Set the Cololight device's state (on/off) in TL1 mode
	///
	/// @param[in] isOn, on=true
	///
	/// @return True if success
	///
	bool setState(bool isOn);

	///
	/// @brief Set the Cololight device's state (on/off) in Direct Mode
	///
	/// @param[in] isOn, on=true
	///
	/// @return True if success
	///
	bool setStateDirect(bool isOn);

	///
	/// @brief Send a request to the Cololight device for execution
	///
	/// @param[in] appID
	/// @param[in] command
	///
	/// @return True if success
	///
	bool sendRequest(const appID appID, const QByteArray& command);

	///
	/// @brief Read response for a send request
	///
	/// @return True if success
	///
	bool readResponse();

	///
	/// @brief Read response for a send request
	///
	/// @param[out] response
	///
	/// @return True if success
	///
	bool readResponse(QByteArray& response);

	///
	/// @brief Discover Cololight devices available (for configuration).
	/// Cololight specific UDP Broadcast discovery
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonArray discover();

	// Cololight model, e.g. CololightPlus, CololightStrip
	int _modelType;

	// Defines how Cololight LED are organised (multiple light beads in a module or individual lights on a strip
	int _ledLayoutType;

	// Count of overall LEDs across all modules
	int _ledBeadCount;

	// Distance (in #modules) of the module farest away from the main controller
	int _distance;

	QByteArray _packetFixPart;
	QByteArray _DataPart;

	QByteArray _directColorCommandTemplate;

	quint32 _sequenceNumber;

	//Cololights discovered and their response message details
	QMultiMap<QString, QMap <QString, QString>> _services;
};

#endif // LEDEVICECOLOLIGHT_H
