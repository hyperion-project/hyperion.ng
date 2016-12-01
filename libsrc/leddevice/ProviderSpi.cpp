
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>
#include <cerrno>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// Local Hyperion includes
#include "ProviderSpi.h"
#include <utils/Logger.h>


ProviderSpi::ProviderSpi()
	: LedDevice()
	, _deviceName("/dev/spidev0.0")
	, _baudRate_Hz(1000000)
	, _latchTime_ns(0)
	, _fid(-1)
	, _spiMode(SPI_MODE_0)
	, _spiDataInvert(false)
{
	memset(&_spi, 0, sizeof(_spi));
}

ProviderSpi::~ProviderSpi()
{
//	close(_fid);
}

bool ProviderSpi::init(const QJsonObject &deviceConfig)
{
	LedDevice::init(deviceConfig);

	_deviceName    = deviceConfig["output"].toString(QString::fromStdString(_deviceName)).toStdString();
	_baudRate_Hz   = deviceConfig["rate"].toInt(_baudRate_Hz);
	_latchTime_ns  = deviceConfig["latchtime"].toInt(_latchTime_ns);
	_spiMode       = deviceConfig["spimode"].toInt(_spiMode);
	_spiDataInvert = deviceConfig["invert"].toBool(_spiDataInvert);
	
	return true;
}

int ProviderSpi::open()
{
	Debug(_log, "_baudRate_Hz %d,  _latchTime_ns %d", _baudRate_Hz, _latchTime_ns);
	Debug(_log, "_spiDataInvert %d,  _spiMode %d", _spiDataInvert, _spiMode);

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

int ProviderSpi::writeBytes(const unsigned size, const uint8_t * data)
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
	ErrorIf((retVal < 0), _log, "SPI failed to write. errno: %d, %s", errno,  strerror(errno) );

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
