
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// Local Hyperion includes
#include "LedSpiDevice.h"


LedSpiDevice::LedSpiDevice(const std::string& outputDevice, const unsigned baudrate) :
	mDeviceName(outputDevice),
	mBaudRate_Hz(baudrate),
	mFid(-1)
{
	memset(&spi, 0, sizeof(spi));
}

LedSpiDevice::~LedSpiDevice()
{
//	close(mFid);
}

int LedSpiDevice::open()
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

int LedSpiDevice::latch(const unsigned len, const char * vec, const int latchTime_ns)
{

	if (mFid < 0)
	{
		return -1;
	}

	spi.tx_buf = __u64(vec);
	spi.len    = __u32(len);

	int retVal = ioctl(mFid, SPI_IOC_MESSAGE(1), &spi);

	if (retVal == 0 && latchTime_ns > 0)
	{
		// The 'latch' time for latching the shifted-value into the leds
		timespec latchTime;
		latchTime.tv_sec  = 0;
		latchTime.tv_nsec = latchTime_ns;

		// Sleep to latch the leds (only if write succesfull)
		nanosleep(&latchTime, NULL);
	}

	return retVal;

}
