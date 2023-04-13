// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>
#include <cerrno>
// Local Hyperion includes
#include "ProviderSpi.h"

#ifdef ENABLE_DEV_SPI
// Linux includes
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
// qt includes
#include <QDir>
#endif

#ifdef ENABLE_DEV_FTDI

#include <ftdi.h>
#include <libusb.h>
#include <utils/WaitTime.h>

#define FTDI_CHECK_RESULT(statement) if (statement) {setInError(ftdi_get_error_string(_ftdic)); return retVal;}

#define ANY_FTDI_VENDOR 0x0
#define ANY_FTDI_PRODUCT 0x0


namespace Pin {
    // enumerate the AD bus for convenience.
    enum bus_t {
        SK = 0x01, // ADBUS0, SPI data clock
        DO = 0x02, // ADBUS1, SPI data out
        CS = 0x08, // ADBUS3, SPI chip select, active low
    };
}

const unsigned char pinInitialState = Pin::CS;
// Use these pins as outputs
const unsigned char pinDirection = Pin::SK | Pin::DO | Pin::CS;
#endif


#include <utils/Logger.h>


// Constants
namespace {
    const bool verbose = false;
#ifdef ENABLE_DEV_SPI
    // SPI discovery service
    const char DISCOVERY_DIRECTORY[] = "/dev/";
    const char DISCOVERY_FILEPATTERN[] = "spidev*";
#endif

    const QString ImplementationSPIDEV = QString("spidev");
    const QString ImplementationFTDI = QString("ftdi");

} //End of constants

ProviderSpi::ProviderSpi(const QJsonObject &deviceConfig)
        : LedDevice(deviceConfig), _deviceName("/dev/spidev0.0"), _baudRate_Hz(1000000)
#ifdef ENABLE_DEV_SPI
        , _fid(-1)
        , _spiMode(SPI_MODE_0)
        , _spiDataInvert(false)
#endif
        , _spiImplementation(SPI_SPIDEV) {
#ifdef ENABLE_DEV_SPI
    memset(&_spi, 0, sizeof(_spi));
    _latchTime_ms = 1;
#endif
}

ProviderSpi::~ProviderSpi() {
}

bool ProviderSpi::init(const QJsonObject &deviceConfig) {
    bool isInitOK = false;

    // Initialise sub-class
    if (LedDevice::init(deviceConfig)) {
        _deviceName = deviceConfig["output"].toString(_deviceName);
        _baudRate_Hz = deviceConfig["rate"].toInt(_baudRate_Hz);
#ifdef ENABLE_DEV_SPI
        _spiMode       = deviceConfig["spimode"].toInt(_spiMode);
        _spiDataInvert = deviceConfig["invert"].toBool(_spiDataInvert);
        Debug(_log, "_spiDataInvert [%d], _spiMode [%d]", _spiDataInvert, _spiMode);
#endif
        bool isFtdiImplementation = (
                QString::compare(deviceConfig["implementation"].toString(ImplementationSPIDEV), ImplementationFTDI,
                                 Qt::CaseInsensitive) == 0);
        _spiImplementation = isFtdiImplementation ? SPI_FTDI : SPI_SPIDEV;

        Debug(_log, "_baudRate_Hz [%d], _latchTime_ms [%d]", _baudRate_Hz, _latchTime_ms);


        isInitOK = true;
    }
    return isInitOK;
}

int ProviderSpi::open() {
    int retVal = -1;
    QString errortext;
    _isDeviceReady = false;
    if (_spiImplementation == SPI_SPIDEV) {
#ifdef ENABLE_DEV_SPI
        const int bitsPerWord = 8;

        _fid = ::open(QSTRING_CSTR(_deviceName), O_RDWR);

        if (_fid < 0)
        {
            errortext = QString ("Failed to open device (%1). Error message: %2").arg(_deviceName, strerror(errno));
            retVal = -1;
        }
        else
        {
            if (ioctl(_fid, SPI_IOC_WR_MODE, &_spiMode) == -1 || ioctl(_fid, SPI_IOC_RD_MODE, &_spiMode) == -1)
            {
                retVal = -2;
            }
            else
            {
                if (ioctl(_fid, SPI_IOC_WR_BITS_PER_WORD, &bitsPerWord) == -1 || ioctl(_fid, SPI_IOC_RD_BITS_PER_WORD, &bitsPerWord) == -1)
                {
                    retVal = -4;
                }
                else
                {
                    if (ioctl(_fid, SPI_IOC_WR_MAX_SPEED_HZ, &_baudRate_Hz) == -1 || ioctl(_fid, SPI_IOC_RD_MAX_SPEED_HZ, &_baudRate_Hz) == -1)
                    {
                        retVal = -6;
                    }
                    else
                    {
                        // Everything OK -> enable device
                        _isDeviceReady = true;
                        retVal = 0;
                    }
                }
            }
            if ( retVal < 0 )
            {
                errortext = QString ("Failed to open device (%1). Error Code: %2").arg(_deviceName).arg(retVal);
            }
        }

        if ( retVal < 0 )
        {
            this->setInError( errortext );
        }
#endif
    } else if (_spiImplementation == SPI_FTDI) {
#ifdef ENABLE_DEV_FTDI

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

        _isDeviceReady = true;
#endif
    }
    return retVal;
}

int ProviderSpi::close() {
    // LedDevice specific closing activities
    int retVal = 0;
    _isDeviceReady = false;
    if (_spiImplementation == SPI_SPIDEV) {
#ifdef ENABLE_DEV_SPI
        // Test, if device requires closing
        if ( _fid > -1 )
        {
            // Close device
            if ( ::close(_fid) != 0 )
            {
                Error( _log, "Failed to close device (%s). Error message: %s", QSTRING_CSTR(_deviceName),  strerror(errno) );
                retVal = -1;
            }
        }
#endif
    } else if (_spiImplementation == SPI_FTDI) {
#ifdef ENABLE_DEV_FTDI
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
#endif
    }
    return retVal;
}

int ProviderSpi::writeBytes(unsigned size, const uint8_t *data) {
    int retVal = 0;
    if (_spiImplementation == SPI_SPIDEV) {
#ifdef ENABLE_DEV_SPI
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

        retVal = ioctl(_fid, SPI_IOC_MESSAGE(1), &_spi);
        ErrorIf((retVal < 0), _log, "SPI failed to write. errno: %d, %s", errno,  strerror(errno) );

        free (newdata);
#endif
    } else if (_spiImplementation == SPI_FTDI) {
#ifdef ENABLE_DEV_FTDI
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

        FTDI_CHECK_RESULT((retVal = ftdi_write_data(_ftdic, buf.data(), buf.size())) != buf.size());
#endif
    }
    return retVal;
}

QJsonObject ProviderSpi::discover(const QJsonObject & /*params*/) {
    QJsonObject devicesDiscovered;
    devicesDiscovered.insert("ledDeviceType", _activeDeviceType);
    QJsonArray deviceList;

#ifdef ENABLE_DEV_SPI
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
        deviceInfo.insert("implementation", ImplementationSPIDEV);
        deviceList.append(deviceInfo);
    }
#endif
#ifdef ENABLE_DEV_FTDI
    struct ftdi_device_list *devlist;
    struct ftdi_context *ftdic;

    ftdic = ftdi_new();

    if (ftdi_usb_find_all(ftdic, &devlist, ANY_FTDI_VENDOR, ANY_FTDI_PRODUCT) > 0) {
        struct ftdi_device_list *curdev = devlist;
        QMap<QString, uint8_t> deviceIndexes;

        while (curdev) {
            libusb_device_descriptor desc;
            int rc = libusb_get_device_descriptor(curdev->dev, &desc);
            if (rc == 0) {
                QString vendorIdentifier = QString("0x%1").arg(desc.idVendor, 4, 16, QChar{'0'});
                QString productIdentifier = QString("0x%1").arg(desc.idProduct, 4, 16, QChar{'0'});
                QString vendorAndProduct = QString("%1:%2")
                        .arg(vendorIdentifier)
                        .arg(productIdentifier);
                uint8_t deviceIndex = deviceIndexes.value(vendorAndProduct, 0);

                char serial_string[128] = {0};
                char manufacturer_string[128] = {0};
                char description_string[128] = {0};
                ftdi_usb_get_strings2(ftdic, curdev->dev, manufacturer_string, 128, description_string, 128,
                                      serial_string, 128);

                QString serialNumber{serial_string};
                QString ftdiOpenString;
                if (!serialNumber.isEmpty()) {
                    ftdiOpenString = QString("s:%1:%2").arg(vendorAndProduct).arg(serialNumber);
                } else {
                    ftdiOpenString = QString("i:%1:%2").arg(vendorAndProduct).arg(deviceIndex);
                }

                deviceList.push_back(QJsonObject{
                        {"ftdiOpenString",    ftdiOpenString},
                        {"vendorIdentifier",  vendorIdentifier},
                        {"productIdentifier", productIdentifier},
                        {"deviceIndex",       deviceIndex},
                        {"serialNumber",      serialNumber},
                        {"manufacturer",      manufacturer_string},
                        {"description",       description_string},
                        {"deviceName",        description_string},
                        {"systemLocation",    ftdiOpenString},
                        {"implementation",    ImplementationFTDI},
                });
                deviceIndexes.insert(vendorAndProduct, deviceIndex + 1);
            }
            curdev = curdev->next;
        }
    }

    ftdi_list_free(&devlist);
    ftdi_free(ftdic);
#endif
    devicesDiscovered.insert("devices", deviceList);
    DebugIf(verbose, _log, "devicesDiscovered: [%s]",
            QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

    return devicesDiscovered;
}
