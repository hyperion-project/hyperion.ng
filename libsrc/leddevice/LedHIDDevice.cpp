
// STL includes
#include <cstring>
#include <iostream>

// Qt includes
#include <QTimer>

// Local Hyperion includes
#include "LedHIDDevice.h"

LedHIDDevice::LedHIDDevice(const unsigned short VendorId, const unsigned short ProductId, int delayAfterConnect_ms, const bool useFeature) :
	_VendorId(VendorId),
	_ProductId(ProductId),
	_useFeature(useFeature),
	_deviceHandle(nullptr),
	_delayAfterConnect_ms(delayAfterConnect_ms),
	_blockedForDelay(false)
{
	// empty
}

LedHIDDevice::~LedHIDDevice()
{
	if (_deviceHandle != nullptr)
	{
		hid_close(_deviceHandle);
		_deviceHandle = nullptr;
	}

	hid_exit();
}

int LedHIDDevice::open()
{
	// Initialize the usb context
	int error = hid_init();
	if (error != 0)
	{
		std::cerr << "Error while initializing the hidapi context" << std::endl;
		return -1;
	}
	std::cout << "Hidapi initialized" << std::endl;

	// Open the device
	printf("Opening device: VID %04hx PID %04hx\n", _VendorId, _ProductId);
	_deviceHandle = hid_open(_VendorId, _ProductId, nullptr);
		
	if (_deviceHandle == nullptr)
	{
		// Failed to open the device
		std::cerr << "Failed to open HID device. Maybe your PID/VID setting is wrong? Make sure to add a udev rule/use sudo." << std::endl;
		
		// http://www.signal11.us/oss/hidapi/
		/*
		std::cout << "Showing a list of all available HID devices:" << std::endl;
		auto devs = hid_enumerate(0x00, 0x00);
		auto cur_dev = devs;	
		while (cur_dev) {
			printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls",
				cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
			printf("\n");
			printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
			printf("  Product:      %ls\n", cur_dev->product_string);
			printf("\n");
			cur_dev = cur_dev->next;
		}
		hid_free_enumeration(devs);
		*/
		
		return -1;
	}
	else{
		std::cout << "Opened HID device successful" << std::endl;
	}
	
	// Wait after device got opened if enabled
	if (_delayAfterConnect_ms > 0)
	{
		_blockedForDelay = true;
		QTimer::singleShot(_delayAfterConnect_ms, this, SLOT(unblockAfterDelay()));
		std::cout << "Device blocked for " << _delayAfterConnect_ms << " ms" << std::endl;
	}

	return 0;
}


int LedHIDDevice::writeBytes(const unsigned size, const uint8_t * data)
{
	if (_blockedForDelay) {
		return 0;
	}

	if (_deviceHandle == nullptr)
	{
		// try to reopen
		auto status = open();
		if(status < 0){
			// Try again in 3 seconds
			int seconds = 3000;
			_blockedForDelay = true;
			QTimer::singleShot(seconds, this, SLOT(unblockAfterDelay()));
			std::cout << "Device blocked for " << seconds << " ms" << std::endl;
		}
		// Return here, to not write led data if the device should be blocked after connect
		return status;
	}

	// Prepend report ID to the buffer
	uint8_t ledData[size + 1];
	ledData[0] = 0; // Report ID
	memcpy(ledData + 1, data, size_t(size));

	// Send data via feature or out report
	int ret;
	if(_useFeature){
		ret = hid_send_feature_report(_deviceHandle, ledData, size + 1);
	}
	else{
		ret = hid_write(_deviceHandle, ledData, size + 1);
	}
	
	// Handle first error
	if(ret < 0){
		std::cerr << "Failed to write to HID device." << std::endl;

		// Try again
		if(_useFeature){
			ret = hid_send_feature_report(_deviceHandle, ledData, size + 1);
		}
		else{
			ret = hid_write(_deviceHandle, ledData, size + 1);
		}

		// Writing failed again, device might have disconnected
		if(ret < 0){
			std::cerr << "Failed to write to HID device." << std::endl;
		
			hid_close(_deviceHandle);
			_deviceHandle = nullptr;
		}
	}
	
	return ret;
}

void LedHIDDevice::unblockAfterDelay()
{
	std::cout << "Device unblocked" << std::endl;
	_blockedForDelay = false;
}
