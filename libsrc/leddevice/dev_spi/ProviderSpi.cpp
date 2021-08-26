
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>
#include <cerrno>

// Linux includes
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

// Local Hyperion includes
#include "ProviderSpi.h"
#include <utils/Logger.h>

// qt includes
#include <QDir>

// Constants
namespace {
	const bool verbose = false;

	// SPI discovery service
	const char DISCOVERY_DIRECTORY[] = "/dev/";
	const char DISCOVERY_FILEPATTERN[] = "spidev*";

} //End of constants

ProviderSpi::ProviderSpi(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	, _deviceName("/dev/spidev0.0")
	, _baudRate_Hz(1000000)
	, _fid(-1)
	, _spiMode(SPI_MODE_0)
	, _spiDataInvert(false)
{
	memset(&_spi, 0, sizeof(_spi));
	_latchTime_ms = 1;
}

ProviderSpi::~ProviderSpi()
{
}

bool ProviderSpi::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( LedDevice::init(deviceConfig) )
	{
		_deviceName    = deviceConfig["output"].toString(_deviceName);
		_baudRate_Hz   = deviceConfig["rate"].toInt(_baudRate_Hz);
		_spiMode       = deviceConfig["spimode"].toInt(_spiMode);
		_spiDataInvert = deviceConfig["invert"].toBool(_spiDataInvert);

		Debug(_log, "_baudRate_Hz [%d], _latchTime_ms [%d]", _baudRate_Hz, _latchTime_ms);
		Debug(_log, "_spiDataInvert [%d], _spiMode [%d]", _spiDataInvert, _spiMode);

		isInitOK = true;
	}
	return isInitOK;
}

int ProviderSpi::open()
{
	int retval = -1;
	QString errortext;
	_isDeviceReady = false;

	const int bitsPerWord = 8;

	_fid = ::open(QSTRING_CSTR(_deviceName), O_RDWR);

	if (_fid < 0)
	{
		errortext = QString ("Failed to open device (%1). Error message: %2").arg(_deviceName, strerror(errno));
		retval = -1;
	}
	else
	{
		if (ioctl(_fid, SPI_IOC_WR_MODE, &_spiMode) == -1 || ioctl(_fid, SPI_IOC_RD_MODE, &_spiMode) == -1)
		{
			retval = -2;
		}
		else
		{
			if (ioctl(_fid, SPI_IOC_WR_BITS_PER_WORD, &bitsPerWord) == -1 || ioctl(_fid, SPI_IOC_RD_BITS_PER_WORD, &bitsPerWord) == -1)
			{
				retval = -4;
			}
			else
			{
				if (ioctl(_fid, SPI_IOC_WR_MAX_SPEED_HZ, &_baudRate_Hz) == -1 || ioctl(_fid, SPI_IOC_RD_MAX_SPEED_HZ, &_baudRate_Hz) == -1)
				{
					retval = -6;
				}
				else
				{
					// Everything OK -> enable device
					_isDeviceReady = true;
					retval = 0;
				}
			}
		}
		if ( retval < 0 )
		{
			errortext = QString ("Failed to open device (%1). Error Code: %2").arg(_deviceName).arg(retval);
		}
	}

	if ( retval < 0 )
	{
		this->setInError( errortext );
	}

	return retval;
}

int ProviderSpi::close()
{
	// LedDevice specific closing activities
	int retval = 0;
	_isDeviceReady = false;

	// Test, if device requires closing
	if ( _fid > -1 )
	{
		// Close device
		if ( ::close(_fid) != 0 )
		{
			Error( _log, "Failed to close device (%s). Error message: %s", QSTRING_CSTR(_deviceName),  strerror(errno) );
			retval = -1;
		}
	}
	return retval;
}

int ProviderSpi::writeBytes(unsigned size, const uint8_t * data)
{
	if (_fid < 0)
	{
		return -1;
	}

	uint8_t * newdata {nullptr};

	_spi.tx_buf = __u64(data);
	_spi.len    = __u32(size);

	if (_spiDataInvert)
	{
		newdata = static_cast<uint8_t *>(malloc(size));
		for (unsigned i = 0; i<size; i++) {
			newdata[i] = data[i] ^ 0xff;
		}
		_spi.tx_buf = __u64(newdata);
	}

	int retVal = ioctl(_fid, SPI_IOC_MESSAGE(1), &_spi);
	ErrorIf((retVal < 0), _log, "SPI failed to write. errno: %d, %s", errno,  strerror(errno) );

	free (newdata);

	return retVal;
}

QJsonObject ProviderSpi::discover(const QJsonObject& /*params*/)
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );

	QJsonArray deviceList;

	QDir deviceDirectory (DISCOVERY_DIRECTORY);
	QStringList deviceFilter(DISCOVERY_FILEPATTERN);
	deviceDirectory.setNameFilters(deviceFilter);
	deviceDirectory.setSorting(QDir::Name);
	QFileInfoList deviceFiles = deviceDirectory.entryInfoList(QDir::System);

	QFileInfoList::const_iterator deviceFileIterator;
	for (deviceFileIterator = deviceFiles.constBegin(); deviceFileIterator != deviceFiles.constEnd(); ++deviceFileIterator)
	{
		QJsonObject deviceInfo;
		deviceInfo.insert("deviceName", (*deviceFileIterator).fileName().remove(0,6));
		deviceInfo.insert("systemLocation", (*deviceFileIterator).absoluteFilePath());
		deviceList.append(deviceInfo);
	}
	devicesDiscovered.insert("devices", deviceList);

	DebugIf(verbose,_log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return devicesDiscovered;
}
