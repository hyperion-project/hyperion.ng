
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


LedSpiDevice::LedSpiDevice(const std::string& outputDevice, const unsigned baudrate, const int latchTime_ns, const int spiMode, const bool spiDataInvert)
	: _deviceName(outputDevice)
	, _baudRate_Hz(baudrate)
	, _latchTime_ns(latchTime_ns)
	, _fid(-1)
	, _spiMode(spiMode)
	, _spiDataInvert(spiDataInvert)
{
	memset(&_spi, 0, sizeof(_spi));
	Debug(_log, "_spiDataInvert %d,  _spiMode %d", _spiDataInvert, _spiMode);
}

LedSpiDevice::~LedSpiDevice()
{
//	close(_fid);
}

int LedSpiDevice::open()
{
	const int bitsPerWord = 8;

	_fid = ::open(_deviceName.c_str(), O_RDWR);

	if (_fid < 0)
	{
		Error( _log, "Failed to open device (%s). Error message: %s", _deviceName.c_str(),  strerror(errno) );
		return -1;
	}

	if (ioctl(_fid, SPI_IOC_WR_MODE, &_spiMode) == -1 || ioctl(_fid, SPI_IOC_RD_MODE, &_spiMode) == -1)
	{
		return -2;
	}

	if (ioctl(_fid, SPI_IOC_WR_BITS_PER_WORD, &bitsPerWord) == -1 || ioctl(_fid, SPI_IOC_RD_BITS_PER_WORD, &bitsPerWord) == -1)
	{
		return -4;
	}

	if (ioctl(_fid, SPI_IOC_WR_MAX_SPEED_HZ, &_baudRate_Hz) == -1 || ioctl(_fid, SPI_IOC_RD_MAX_SPEED_HZ, &_baudRate_Hz) == -1)
	{
		return -6;
	}

	return 0;
}

int LedSpiDevice::writeBytes(const unsigned size, const uint8_t * data)
{
	if (_fid < 0)
	{
		return -1;
	}

	_spi.tx_buf = __u64(data);
	_spi.len    = __u32(size);

	if (_spiDataInvert)
	{
		uint8_t * newdata = (uint8_t *)malloc(size);
		for (unsigned i = 0; i<size; i++) {
			newdata[i] = data[i] ^ 0xff;
		}
		_spi.tx_buf = __u64(newdata);
	}

	int retVal = ioctl(_fid, SPI_IOC_MESSAGE(1), &_spi);

	if (retVal == 0 && _latchTime_ns > 0)
	{
		// The 'latch' time for latching the shifted-value into the leds
		timespec latchTime;
		latchTime.tv_sec  = 0;
		latchTime.tv_nsec = _latchTime_ns;

		// Sleep to latch the leds (only if write succesfull)
		nanosleep(&latchTime, NULL);
	}

	return retVal;
}
