
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>
#include <cerrno>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// Local Hyperion includes
#include "LedSpiDevice.h"
#include <utils/Logger.h>


LedSpiDevice::LedSpiDevice(const std::string& outputDevice, const unsigned baudrate, const int latchTime_ns,
				const int spiMode, const bool spiDataInvert) :
	mDeviceName(outputDevice),
	mBaudRate_Hz(baudrate),
	mLatchTime_ns(latchTime_ns),
	mFid(-1),
	mSpiMode(spiMode),
	mSpiDataInvert(spiDataInvert)
{
	memset(&spi, 0, sizeof(spi));
}

LedSpiDevice::~LedSpiDevice()
{
//	close(mFid);
}

int LedSpiDevice::open()
{
//printf ("mSpiDataInvert %d  mSpiMode %d\n",mSpiDataInvert,  mSpiMode);
	const int bitsPerWord = 8;

	mFid = ::open(mDeviceName.c_str(), O_RDWR);

	if (mFid < 0)
	{
		Error( _log, "Failed to open device (%s). Error message: %s", mDeviceName.c_str(),  strerror(errno) );
		return -1;
	}

	if (ioctl(mFid, SPI_IOC_WR_MODE, &mSpiMode) == -1 || ioctl(mFid, SPI_IOC_RD_MODE, &mSpiMode) == -1)
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

int LedSpiDevice::writeBytes(const unsigned size, const uint8_t * data)
{
	if (mFid < 0)
	{
		return -1;
	}

	spi.tx_buf = __u64(data);
	spi.len    = __u32(size);

	if (mSpiDataInvert) {
		uint8_t * newdata = (uint8_t *)malloc(size);
		for (unsigned i = 0; i<size; i++) {
			newdata[i] = data[i] ^ 0xff;
		}
		spi.tx_buf = __u64(newdata);
	}

	int retVal = ioctl(mFid, SPI_IOC_MESSAGE(1), &spi);

	if (retVal == 0 && mLatchTime_ns > 0)
	{
		// The 'latch' time for latching the shifted-value into the leds
		timespec latchTime;
		latchTime.tv_sec  = 0;
		latchTime.tv_nsec = mLatchTime_ns;

		// Sleep to latch the leds (only if write succesfull)
		nanosleep(&latchTime, NULL);
	}

	return retVal;
}
