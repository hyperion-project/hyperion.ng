#include "ProviderFtdi.h"

#include <utils/WaitTime.h>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#define ANY_FTDI_VENDOR 0x0
#define ANY_FTDI_PRODUCT 0x0

#include <ftdi.h>
#include <libusb.h>
#include <QEventLoop>
#include <QTimer>

namespace Pin {
    // enumerate the AD bus for convenience.
    enum bus_t {
        SK = 0x01, // ADBUS0, SPI data clock
        DO = 0x02, // ADBUS1, SPI data out
        CS = 0x08, // ADBUS3, SPI chip select, active low
    };
}

#define FTDI_CHECK_RESULT(statement) if (statement) {Debug(_log, "FTDI ERROR: %s", ftdi_get_error_string(_ftdic)); return retVal;}

const unsigned char pinInitialState = Pin::CS;
// Use these pins as outputs
const unsigned char pinDirection = Pin::SK | Pin::DO | Pin::CS;

// Local HyperHDR includes
#include "utils/Logger.h"


ProviderFtdi::ProviderFtdi(const QJsonObject &deviceConfig): BaseProvider(deviceConfig),
        _ftdic(nullptr) {
    Debug(_log, "Initialise ProviderFtdi");
}

ProviderFtdi::~ProviderFtdi() {}



int ProviderFtdi::open() {
    int retVal = -1;
    _ftdic = ftdi_new();

    Debug(_log, "Opening FTDI device=%s", QSTRING_CSTR(_deviceName));

    FTDI_CHECK_RESULT((retVal = ftdi_usb_open_string(_ftdic, QSTRING_CSTR(_deviceName))) < 0);
    /* doing this disable resets things if they were in a bad state */
    FTDI_CHECK_RESULT((retVal = ftdi_disable_bitbang(_ftdic)) < 0);
    FTDI_CHECK_RESULT((retVal = ftdi_setflowctrl(_ftdic, SIO_DISABLE_FLOW_CTRL)) < 0);
    FTDI_CHECK_RESULT((retVal = ftdi_set_bitmode(_ftdic, 0x00, BITMODE_RESET)) < 0);
    FTDI_CHECK_RESULT((retVal = ftdi_set_bitmode(_ftdic, 0xff, BITMODE_MPSSE)) < 0);


    double reference_clock = 60e6;
    int divisor = (reference_clock / 2 / _baudRate_Hz) - 1;
    std::vector<uint8_t> buf = {
            DIS_DIV_5,
            TCK_DIVISOR,
            static_cast<unsigned char>(divisor),
            static_cast<unsigned char>(divisor >> 8),
            SET_BITS_LOW,          // opcode: set low bits (ADBUS[0-7]
            pinInitialState,    // argument: inital pin state
            pinDirection
    };

    FTDI_CHECK_RESULT((retVal = ftdi_write_data(_ftdic, buf.data(), buf.size())) != buf.size());
    Debug(_log, "Opened ftdi device=%s retVal=%d", QSTRING_CSTR(_deviceName), retVal);
    return retVal;
}


int ProviderFtdi::close() {
    int retVal = 0;
    if (_ftdic != nullptr) {
        Debug(_log, "Closing FTDI device");
//      Delay to give time to push color black from writeBlack() into the led,
//      otherwise frame transmission will be terminated half way through
        wait(30);
        ftdi_set_bitmode(_ftdic, 0x00, BITMODE_RESET);
        ftdi_usb_close(_ftdic);
        ftdi_free(_ftdic);
        _ftdic = nullptr;
    }
    return retVal;
}

int ProviderFtdi::writeBytes(unsigned size, const uint8_t *data) {
    int retVal = 0;

    int count_arg = size - 1;
    std::vector<uint8_t> buf = {
            SET_BITS_LOW,
            pinInitialState & ~Pin::CS,
            pinDirection,
            MPSSE_DO_WRITE | MPSSE_WRITE_NEG,
            static_cast<unsigned char>(count_arg),
            static_cast<unsigned char>(count_arg >> 8),
//            LED's data will be inserted here
            SET_BITS_LOW,
            pinInitialState | Pin::CS,
            pinDirection
    };
    // insert before last SET_BITS_LOW command
    // SET_BITS_LOW takes 2 arguments, so we're inserting data in -3 position from the end
    buf.insert(buf.end() - 3, &data[0], &data[size]);

    FTDI_CHECK_RESULT((retVal = (ftdi_write_data(_ftdic, buf.data(), buf.size())) != buf.size()));
    return retVal;
}

QJsonArray ProviderFtdi::discover(const QJsonObject & /*params*/) {
    QJsonArray deviceList;

    struct ftdi_device_list *devlist;
    struct ftdi_context *ftdic;

    ftdic = ftdi_new();

    if (ftdi_usb_find_all(ftdic, &devlist, ANY_FTDI_VENDOR, ANY_FTDI_PRODUCT) > 0)
    {
        QMap<QString, uint8_t> deviceIndexes;
        struct ftdi_device_list *curdev = devlist;
        while (curdev)
        {
            char manufacturer[128] = {0}, serial_string[128] = {0};
            ftdi_usb_get_strings(ftdic, curdev->dev, manufacturer, 128, NULL, 0, serial_string, 128);

            libusb_device_descriptor desc;
            libusb_get_device_descriptor(curdev->dev, &desc);

            QString vendorAndProduct = QString("0x%1:0x%2")
                    .arg(desc.idVendor, 4, 16, QChar{'0'})
                    .arg(desc.idProduct, 4, 16, QChar{'0'});

            QString serialNumber {serial_string};
            QString ftdiOpenString;
            if(!serialNumber.isEmpty())
            {
                ftdiOpenString = QString("s:%1:%2").arg(vendorAndProduct).arg(serialNumber);
            }
            else
            {
                uint8_t deviceIndex = deviceIndexes.value(vendorAndProduct, 0);
                ftdiOpenString = QString("i:%1:%2").arg(vendorAndProduct).arg(deviceIndex);
                deviceIndexes.insert(vendorAndProduct, deviceIndex + 1);
            }

            QString displayLabel = QString("%1 (%2)")
                    .arg(ftdiOpenString)
                    .arg(manufacturer);

            deviceList.push_back(QJsonObject{
                {"value", ftdiOpenString},
                {"name", displayLabel}
            });

            curdev = curdev->next;
        }
    }

    ftdi_list_free(&devlist);
    ftdi_free(ftdic);


    Debug(_log, "SPI devices discovered: [%s]",
          QString(QJsonDocument(deviceList).toJson(QJsonDocument::Compact)).toUtf8().constData());

    return deviceList;
}
