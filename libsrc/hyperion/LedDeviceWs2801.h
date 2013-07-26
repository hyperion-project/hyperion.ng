#pragma once

// STL includes
#include <string>

// Linux-SPI includes
#include <linux/spi/spidev.h>

// hyperion incluse
#include <hyperion/LedDevice.h>

class LedDeviceWs2801 : public LedDevice
{
public:
	LedDeviceWs2801(const std::string& name,
					const std::string& outputDevice,
					const unsigned interval,
					const unsigned baudrate);

	virtual ~LedDeviceWs2801();

	int open();

	virtual int write(const std::vector<RgbColor> &ledValues);

private:
	const std::string mDeviceName;
	const int mBaudRate_Hz;

	int mFid;
	spi_ioc_transfer spi;
	timespec latchTime;
};
