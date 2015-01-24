
// STL includes
#include <cstring>
#include <iostream>

// Qt includes
#include <QTimer>

// Serial includes
#include <serial/serial.h>

// Local Hyperion includes
#include "LedRs232Device.h"

LedRs232Device::LedRs232Device(const std::string& outputDevice, const unsigned baudrate, int delayAfterConnect_ms) :
	_deviceName(outputDevice),
	_baudRate_Hz(baudrate),
	_delayAfterConnect_ms(delayAfterConnect_ms),
	_rs232Port(),
	_blockedForDelay(false)
{
	// empty
}

LedRs232Device::~LedRs232Device()
{
	if (_rs232Port.isOpen())
	{
		_rs232Port.close();
	}
}

int LedRs232Device::open()
{
	try
	{
		std::cout << "Opening UART: " << _deviceName << std::endl;
		_rs232Port.setPort(_deviceName);
		_rs232Port.setBaudrate(_baudRate_Hz);
		_rs232Port.open();

		if (_delayAfterConnect_ms > 0)
		{
			_blockedForDelay = true;
			QTimer::singleShot(_delayAfterConnect_ms, this, SLOT(unblockAfterDelay()));
			std::cout << "Device blocked for " << _delayAfterConnect_ms << " ms" << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Unable to open RS232 device (" << e.what() << ")" << std::endl;
		return -1;
	}

	return 0;
}

int LedRs232Device::writeBytes(const unsigned size, const uint8_t * data)
{
	if (_blockedForDelay)
	{
		return 0;
	}

	if (!_rs232Port.isOpen())
	{
		return -1;
	}

//	for (int i = 0; i < 20; ++i)
//		std::cout << std::hex << (int)data[i] << " ";
//	std::cout << std::endl;

	try
	{
		_rs232Port.flushOutput();
		_rs232Port.write(data, size);
		_rs232Port.flush();
	}
	catch (const serial::SerialException & serialExc)
	{
		// TODO[TvdZ]: Maybe we should limit the frequency of this error report somehow
		std::cerr << "Serial exception caught while writing to device: " << serialExc.what() << std::endl;
		std::cout << "Attempting to re-open the device." << std::endl;

		// First make sure the device is properly closed
		try
		{
			_rs232Port.close();
		}
		catch (const std::exception & e) {}

		// Attempt to open the device and write the data
		try
		{
			_rs232Port.open();
			_rs232Port.write(data, size);
			_rs232Port.flush();
		}
		catch (const std::exception & e)
		{
			// We failed again, this not good, do nothing maybe in the next loop we have more success
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Unable to write to RS232 device (" << e.what() << ")" << std::endl;
		return -1;
	}

	return 0;
}

void LedRs232Device::unblockAfterDelay()
{
	std::cout << "Device unblocked" << std::endl;
	_blockedForDelay = false;
}
