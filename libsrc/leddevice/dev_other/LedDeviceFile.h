#ifndef LEDEVICEFILE_H
#define LEDEVICEFILE_H

// LedDevice includes
#include <leddevice/LedDevice.h>

// Qt includes
#include <QFile>
#include <QDateTime>

///
/// Implementation of the LedDevice that write the LED-colors to an
/// ASCII-textfile primarily for testing purposes
///
class LedDeviceFile : public LedDevice
{
public:

	///
	/// @brief Constructs a file output LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceFile(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LedDevice
	///
	~LedDeviceFile() override;

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	static LedDevice* construct(const QJsonObject &deviceConfig);

protected:

	///
	/// @brief Initialise the device's configuration
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
	int write(const std::vector<ColorRgb> & ledValues) override;

	/// The outputstream
	QFile* _file;

private:

	void initFile(const QString &filename);

	QString _fileName;
	/// Timestamp for the output record
	bool _printTimeStamp;

};

#endif // LEDEVICEFILE_H
