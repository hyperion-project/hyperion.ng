
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
