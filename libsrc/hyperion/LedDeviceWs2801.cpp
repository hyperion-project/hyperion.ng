
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceWs2801.h"

LedDeviceWs2801::LedDeviceWs2801(const std::string& outputDevice, const unsigned baudrate) :
	LedSpiDevice(outputDevice, baudrate),
	mLedCount(0)
{
	latchTime.tv_sec  = 0;
	latchTime.tv_nsec = 500000;
}

int LedDeviceWs2801::write(const std::vector<RgbColor> &ledValues)
{
	mLedCount = ledValues.size();

	if (mFid < 0)
	{
		std::cerr << "Can not write to device which is open." << std::endl;
		return -1;
	}

	spi.tx_buf = (__u64)ledValues.data();
	spi.len    = ledValues.size() * sizeof(RgbColor);

	int retVal = ioctl(mFid, SPI_IOC_MESSAGE(1), &spi);

	if (retVal == 0)
	{
		// Sleep to latch the leds (only if write succesfull)
		nanosleep(&latchTime, NULL);
	}

	return retVal;
}

int LedDeviceWs2801::switchOff()
{
	return write(std::vector<RgbColor>(mLedCount, RgbColor::BLACK));
}
