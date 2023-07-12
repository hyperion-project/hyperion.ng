#include "ProviderSpidev.h"

// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>
#include <cerrno>

// Linux includes
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <QDirIterator>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>


// Local HyperHDR includes
#include "utils/Logger.h"


ProviderSpidev::ProviderSpidev(const QJsonObject &deviceConfig)
: BaseProvider(deviceConfig),
          _fid(-1),
          _spiMode(SPI_MODE_0),
          _spiDataInvert(false) {
    memset(&_spi, 0, sizeof(_spi));

    _spiMode = deviceConfig["spimode"].toInt(_spiMode);
    _spiDataInvert = deviceConfig["invert"].toBool(_spiDataInvert);
}

ProviderSpidev::~ProviderSpidev() {
}

int ProviderSpidev::open() {
    int retVal = -1;
    QString errortext;
    const int bitsPerWord = 8;

    _fid = ::open(QSTRING_CSTR(_deviceName), O_RDWR);

    if (_fid < 0) {
        errortext = QString("Failed to open device (%1). Error message: %2").arg(_deviceName, strerror(errno));
        retVal = -1;
    } else {
        if (ioctl(_fid, SPI_IOC_WR_MODE, &_spiMode) == -1 || ioctl(_fid, SPI_IOC_RD_MODE, &_spiMode) == -1) {
            retVal = -2;
        } else {
            if (ioctl(_fid, SPI_IOC_WR_BITS_PER_WORD, &bitsPerWord) == -1 ||
                ioctl(_fid, SPI_IOC_RD_BITS_PER_WORD, &bitsPerWord) == -1) {
                retVal = -4;
            } else {
                if (ioctl(_fid, SPI_IOC_WR_MAX_SPEED_HZ, &_baudRate_Hz) == -1 ||
                    ioctl(_fid, SPI_IOC_RD_MAX_SPEED_HZ, &_baudRate_Hz) == -1) {
                    retVal = -6;
                } else {
                    // Everything OK -> enable device
                    retVal = 0;
                }
            }
        }
        if (retVal < 0) {
            errortext = QString("Failed to open device (%1). Error Code: %2").arg(_deviceName).arg(retVal);
        }
    }

    if (retVal < 0) {
//        this->setInError(errortext);
    }

    return retVal;
}


int ProviderSpidev::close() {
    // LedDevice specific closing activities
    int retVal = 0;

    // Test, if device requires closing
    if (_fid > -1) {
        // Close device
        if (::close(_fid) != 0) {
            Error(_log, "Failed to close device (%s). Error message: %s", QSTRING_CSTR(_deviceName), strerror(errno));
            retVal = -1;
        }
    }

    return retVal;
}

int ProviderSpidev::writeBytes(unsigned size, const uint8_t *data) {
    int retVal = 0;

    uint8_t *newdata = nullptr;

    if (_fid < 0) {
        return -1;
    }

    _spi.tx_buf = __u64(data);
    _spi.len = __u32(size);

    if (_spiDataInvert) {
        newdata = (uint8_t *) malloc(size);
        for (unsigned i = 0; i < size; i++) {
            newdata[i] = data[i] ^ 0xff;
        }
        _spi.tx_buf = __u64(newdata);
    }

    retVal = ioctl(_fid, SPI_IOC_MESSAGE(1), &_spi);
    ErrorIf((retVal < 0), _log, "SPI failed to write. errno: %d, %s", errno, strerror(errno));

    if (newdata != nullptr)
        free(newdata);

    return retVal;
}

QJsonArray ProviderSpidev::discover(const QJsonObject& /*params*/) {
    QJsonArray deviceList;
    QStringList files;
    QDirIterator it("/dev", QStringList() << "spidev*", QDir::System);

    while (it.hasNext())
        files << it.next();
    files.sort();

    for (const auto &path: files)
        deviceList.push_back(QJsonObject{
                {"value", path},
                {"name",  path}});

    Debug(_log, "SPI devices discovered: [%s]",
          QString(QJsonDocument(deviceList).toJson(QJsonDocument::Compact)).toUtf8().constData());

    return deviceList;
}
