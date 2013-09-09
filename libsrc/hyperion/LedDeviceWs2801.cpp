
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceWs2801.h"

LedDeviceWs2801::LedDeviceWs2801(const std::string& outputDevice,
								 const unsigned baudrate) :
	mDeviceName(outputDevice),
	mBaudRate_Hz(baudrate),
	mFid(-1)
{
	memset(&spi, 0, sizeof(spi));

	latchTime.tv_sec  = 0;
	latchTime.tv_nsec = 500000;

}

LedDeviceWs2801::~LedDeviceWs2801()
{
//	close(mFid);
}

int LedDeviceWs2801::open()
{
	const int bitsPerWord = 8;

	mFid = ::open(mDeviceName.c_str(), O_RDWR);

	if (mFid < 0)
	{
		std::cerr << "Failed to open device('" << mDeviceName << "') " << std::endl;
		return -1;
	}

	int mode = SPI_MODE_0;
	if (ioctl(mFid, SPI_IOC_WR_MODE, &mode) == -1 || ioctl(mFid, SPI_IOC_RD_MODE, &mode) == -1)
	{
		return -2;
	}

	if (ioctl(mFid, SPI_IOC_WR_BITS_PER_WORD, &bitsPerWord) == -1 || ioctl(mFid, SPI_IOC_RD_BITS_PER_WORD, &bitsPerWord) == -1)
	{
		return -4;
	}

	if (ioctl(mFid, SPI_IOC_WR_MAX_SPEED_HZ, &mBaudRate_Hz) == -1 || ioctl(mFid, SPI_IOC_RD_MAX_SPEED_HZ, &mBaudRate_Hz) == -1)
	{
		return -6;
	}

	return 0;
}

int LedDeviceWs2801::write(const std::vector<RgbColor> &ledValues)
{
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
