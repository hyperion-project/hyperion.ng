// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderFtdi.h"
#include <utils/WaitTime.h>

#include <ftdi.h>
#include <libusb.h>

#define ANY_FTDI_VENDOR 0x0
#define ANY_FTDI_PRODUCT 0x0

#define FTDI_CHECK_RESULT(statement) if (statement) {setInError(ftdi_get_error_string(_ftdic)); return rc;}

namespace Pin
{
// enumerate the AD bus for convenience.
enum bus_t
{
	SK = 0x01, // ADBUS0, SPI data clock
	DO = 0x02, // ADBUS1, SPI data out
	CS = 0x08, // ADBUS3, SPI chip select, active low
};
}

const uint8_t pinInitialState = Pin::CS;
// Use these pins as outputs
const uint8_t pinDirection = Pin::SK | Pin::DO | Pin::CS;

const QString ProviderFtdi::AUTO_SETTING = QString("auto");

ProviderFtdi::ProviderFtdi(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig),
	  _ftdic(nullptr),
	  _baudRate_Hz(1000000)
{
}

bool ProviderFtdi::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	if (LedDevice::init(deviceConfig))
	{
		_baudRate_Hz = deviceConfig["rate"].toInt(_baudRate_Hz);
		_deviceName = deviceConfig["output"].toString(AUTO_SETTING);

		Debug(_log, "_baudRate_Hz [%d]", _baudRate_Hz);
		Debug(_log, "_deviceName [%s]", QSTRING_CSTR(_deviceName));

		isInitOK = true;
	}
	return isInitOK;
}

int ProviderFtdi::open()
{
	int rc = 0;

	_ftdic = ftdi_new();

	if (ftdi_init(_ftdic) < 0)
	{
		_ftdic = nullptr;
		setInError("Could not initialize the ftdi library");
		return -1;
	}

	Debug(_log, "Opening FTDI device=%s", QSTRING_CSTR(_deviceName));

	FTDI_CHECK_RESULT((rc = ftdi_usb_open_string(_ftdic, QSTRING_CSTR(_deviceName))) < 0);
	/* doing this disable resets things if they were in a bad state */
	FTDI_CHECK_RESULT((rc = ftdi_disable_bitbang(_ftdic)) < 0);
	FTDI_CHECK_RESULT((rc = ftdi_setflowctrl(_ftdic, SIO_DISABLE_FLOW_CTRL)) < 0);
	FTDI_CHECK_RESULT((rc = ftdi_set_bitmode(_ftdic, 0x00, BITMODE_RESET)) < 0);
	FTDI_CHECK_RESULT((rc = ftdi_set_bitmode(_ftdic, 0xff, BITMODE_MPSSE)) < 0);

	double reference_clock = 60e6;
	int divisor = (reference_clock / 2 / _baudRate_Hz) - 1;
	std::vector<uint8_t> buf = {
		DIS_DIV_5,
		TCK_DIVISOR,
		static_cast<unsigned char>(divisor),
		static_cast<unsigned char>(divisor >> 8),
		SET_BITS_LOW,    // opcode: set low bits (ADBUS[0-7]
		pinInitialState, // argument: inital pin state
		pinDirection
	};

	FTDI_CHECK_RESULT((rc = ftdi_write_data(_ftdic, buf.data(), buf.size())) != buf.size());

	_isDeviceReady = true;
	return rc;
}

int ProviderFtdi::close()
{
	LedDevice::close();
	if (_ftdic != nullptr) {
		Debug(_log, "Closing FTDI device");
		// Delay to give time to push color black from writeBlack() into the led,
		// otherwise frame transmission will be terminated half way through
		wait(30);
		ftdi_set_bitmode(_ftdic, 0x00, BITMODE_RESET);
		ftdi_usb_close(_ftdic);
		ftdi_free(_ftdic);
		_ftdic = nullptr;
	}
	return 0;
}

void ProviderFtdi::setInError(const QString &errorMsg, bool isRecoverable)
{
	close();

	LedDevice::setInError(errorMsg, isRecoverable);
}

int ProviderFtdi::writeBytes(const qint64 size, const uint8_t *data)
{
	int rc;
	int count_arg = size - 1;
	std::vector<uint8_t> buf = {
		SET_BITS_LOW,
		pinInitialState & ~Pin::CS,
		pinDirection,
		MPSSE_DO_WRITE | MPSSE_WRITE_NEG,
		static_cast<unsigned char>(count_arg),
		static_cast<unsigned char>(count_arg >> 8),
		SET_BITS_LOW,
		pinInitialState | Pin::CS,
		pinDirection
	};
	// insert before last SET_BITS_LOW command
	// SET_BITS_LOW takes 2 arguments, so we're inserting data in -3 position from the end
	buf.insert(buf.end() - 3, &data[0], &data[size]);

	FTDI_CHECK_RESULT((rc = ftdi_write_data(_ftdic, buf.data(), buf.size())) != buf.size());
	return rc;
}

QJsonObject ProviderFtdi::discover(const QJsonObject & /*params*/)
{
	QJsonObject devicesDiscovered;
	QJsonArray deviceList;
	struct ftdi_device_list *devlist;
	struct ftdi_context *ftdic;

	ftdic = ftdi_new();

	if (ftdi_usb_find_all(ftdic, &devlist, ANY_FTDI_VENDOR, ANY_FTDI_PRODUCT) > 0)
	{
		struct ftdi_device_list *curdev = devlist;
		QMap<QString, uint8_t> deviceIndexes;

		while (curdev)
		{
			libusb_device_descriptor desc;
			int rc = libusb_get_device_descriptor(curdev->dev, &desc);
			if (rc == 0)
			{
				QString vendorIdentifier =  QString("0x%1").arg(desc.idVendor, 4, 16, QChar{'0'});
				QString productIdentifier = QString("0x%1").arg(desc.idProduct, 4, 16, QChar{'0'});
				QString vendorAndProduct = QString("%1:%2")
										   .arg(vendorIdentifier)
										   .arg(productIdentifier);
				uint8_t deviceIndex = deviceIndexes.value(vendorAndProduct, 0);

				char serial_string[128] = {0};
				char manufacturer_string[128] = {0};
				char description_string[128] = {0};
				ftdi_usb_get_strings2(ftdic, curdev->dev, manufacturer_string, 128, description_string, 128, serial_string, 128);

				QString serialNumber {serial_string};
				QString ftdiOpenString;
				if(!serialNumber.isEmpty())
				{
					ftdiOpenString = QString("s:%1:%2").arg(vendorAndProduct).arg(serialNumber);
				}
				else
				{
					ftdiOpenString = QString("i:%1:%2").arg(vendorAndProduct).arg(deviceIndex);
				}

				deviceList.push_back(QJsonObject{
										 {"ftdiOpenString", ftdiOpenString},
										 {"vendorIdentifier", vendorIdentifier},
										 {"productIdentifier", productIdentifier},
										 {"deviceIndex", deviceIndex},
										 {"serialNumber", serialNumber},
										 {"manufacturer", manufacturer_string},
										 {"description", description_string}
									 });
				deviceIndexes.insert(vendorAndProduct, deviceIndex + 1);
			}
			curdev = curdev->next;
		}
	}

	ftdi_list_free(&devlist);
	ftdi_free(ftdic);

	devicesDiscovered.insert("ledDeviceType", _activeDeviceType);
	devicesDiscovered.insert("devices", deviceList);

	Debug(_log, "FTDI devices discovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return devicesDiscovered;
}
