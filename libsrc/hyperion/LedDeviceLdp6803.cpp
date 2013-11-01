// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceLdp6803.h"

LedDeviceLDP6803::LedDeviceLDP6803(const std::string& outputDevice, const unsigned baudrate) :
	LedSpiDevice(outputDevice, baudrate),
	mLedCount(0)
{
	latchTime.tv_sec = 0;
	latchTime.tv_nsec = 500000;
}

int LedDeviceLDP6803::write(const std::vector<RgbColor> &ledValues)
{
	mLedCount = ledValues.size();

	// Define buffer sizes based on number of leds
	// buffsize for actual buffer to be sent via SPI pins
	// tempbuffsize for RGB data processing.
	// buffsize = 4 zero bytes + 2 bytes per LED
	// tempbuffsize will hold RGB values, so 3 bytes per LED

	int buffsize = (mLedCount * 2) + 4;
	int tempbuffsize = mLedCount *3;
	int i,r,g,b,d,count;

	uint8_t m_buff[buffsize];
	const uint8_t *temp_buff;//[tempbuffsize];

	if (mFid < 0)
	{
			std::cerr << "Can not write to device which is open." << std::endl;
			return -1;
	}

	temp_buff = reinterpret_cast<const uint8_t*>(ledValues.data());

	// set first 4 bytes to zero
	m_buff[0]=0;
	m_buff[1]=0;
	m_buff[2]=0;
	m_buff[3]=0;

	// Set counter
	count=4;

	// Now process RGB values: 0-255 to be
	// converted to 0-31, with bits combined
	// to match hardware protocol

	for (i=0 ; i < tempbuffsize ; i+=3) {
		r = temp_buff[i] >> 3;
		g = temp_buff[i+1] >> 3;
		b = temp_buff[i+2] >> 3;

		d = (r * 1024) + (g * 32) + b + 32768;

		m_buff[count] = d >> 8;
		m_buff[count+1] = d & 0x00FF;

		count += 2;
	}

	spi.tx_buf = __u64(m_buff);
	spi.len    = buffsize;

	int retVal = ioctl(mFid, SPI_IOC_MESSAGE(1), &spi);

	if (retVal == 0)
	{
		// Sleep to latch the leds (only if write succesfull)
		nanosleep(&latchTime, NULL);
	}

	return retVal;
}

int LedDeviceLDP6803::switchOff()
{
	return write(std::vector<RgbColor>(mLedCount, RgbColor::BLACK));
}
