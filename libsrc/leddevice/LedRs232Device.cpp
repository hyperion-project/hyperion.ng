
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Local Hyperion includes
#include "LedRs232Device.h"


LedRs232Device::LedRs232Device(const std::string& outputDevice, const unsigned baudrate) :
	mDeviceName(outputDevice),
	mBaudRate_Hz(baudrate),
	_rs232Port()
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
		_rs232Port.setPort(mDeviceName);
		_rs232Port.setBaudrate(mBaudRate_Hz);
		_rs232Port.open();
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
	if (!_rs232Port.isOpen())
	{
		return -1;
	}

//	for (int i = 0; i < 20; ++i)
//		std::cout << std::hex << (int)data[i] << " ";
//	std::cout << std::endl;

	try
	{
		_rs232Port.write(data, size);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Unable to write to RS232 device (" << e.what() << ")" << std::endl;
		return -1;
	}

	return 0;
}
