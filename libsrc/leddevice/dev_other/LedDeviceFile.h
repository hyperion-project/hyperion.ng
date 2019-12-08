#pragma once

// STL includes
#include <fstream>

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
	LedDeviceFile(const QJsonObject &deviceConfig);

	///
	/// Destructor of this test-device
	///
	virtual ~LedDeviceFile();

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool init(const QJsonObject &deviceConfig);
	
protected:
	///
	/// Opens and configures the output file
	///
	/// @return Zero on succes else negative
	///
	///
	virtual int open();
	///
	/// Writes the given led-color values to the output stream
	///
	/// @param ledValues The color-value per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	/// The outputstream
	std::ofstream _ofs;

private:

	QString _fileName;
	/// Timestamp for the output record
	bool _printTimeStamp;
};
