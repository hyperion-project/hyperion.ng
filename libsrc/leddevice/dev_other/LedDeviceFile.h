#pragma once

// STL includes
#include <fstream>
#include <chrono>

// Leddevice includes
#include <leddevice/LedDevice.h>

///
/// Implementation of the LedDevice that write the led-colors to an
/// ASCII-textfile('/home/pi/LedDevice.out')
///
class LedDeviceFile : public LedDevice
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	explicit LedDeviceFile(const QJsonObject &deviceConfig);

	///
	/// Destructor of this test-device
	///
	virtual ~LedDeviceFile() override;

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool init(const QJsonObject &deviceConfig) override;

public slots:
	///
	/// Closes the output device.
	/// Includes switching-off the device and stopping refreshes
	///
	virtual void close() override;
	
protected:
	///
	/// Opens and initiatialises the output device
	///
	/// @return Zero on succes (i.e. device is ready and enabled) else negative
	///
	virtual int open() override;

	///
	/// Writes the given led-color values to the output stream
	///
	/// @param ledValues The color-value per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues) override;

	/// The outputstream
	std::ofstream _ofs;

private:

	QString _fileName;
	/// Timestamp for the output record
	bool _printTimeStamp;
	/// Last write/output timestamp
	std::chrono::system_clock::time_point lastWriteTime = std::chrono::system_clock::now();

};
