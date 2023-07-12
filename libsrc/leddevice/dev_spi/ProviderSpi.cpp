#include "HyperionConfig.h"
#include "ProviderSpi.h"

#ifdef ENABLE_SPIDEV
#include "providers/ProviderSpidev.h"
#endif


#ifdef ENABLE_FTDIDEV
#include "providers/ProviderFtdi.h"
#endif

// Local HyperHDR includes
#include <utils/Logger.h>


namespace SPIProvider {
    SpiImplementation deviceNameToSpiImplementation(const QString &deviceName) {
        if ((deviceName.startsWith("d:") || deviceName.startsWith("i:") || deviceName.startsWith("s:"))) {
            return SpiImplementation::FTDI;
        } else {
            return SpiImplementation::SPIDEV;
        }
    }

    BaseProvider *construct(const QJsonObject &deviceConfig) {
        QString deviceName = deviceConfig["output"].toString("unknown");
        switch (SPIProvider::deviceNameToSpiImplementation(deviceName)) {
#ifdef ENABLE_SPIDEV
            case SPIDEV:
                return dynamic_cast<BaseProvider *>(new ProviderSpidev(deviceConfig));
                break;
#endif
#ifdef ENABLE_FTDIDEV
            case FTDI:
                return dynamic_cast<BaseProvider *>(new ProviderFtdi(deviceConfig));
                break;
#endif
            default:
                break;
        }
        return new BaseProvider(deviceConfig);
    }

    QJsonArray discover(const QJsonObject &params) {
        QJsonArray deviceList;
#ifdef ENABLE_SPIDEV
        QJsonArray spidevDevices = (new ProviderSpidev(params))->discover(params);
        for (const auto &item: spidevDevices){
            deviceList += item;
        }
#endif

#ifdef ENABLE_FTDIDEV
        QJsonArray ftdiDevices = (new ProviderFtdi(params))->discover(params);
        for (const auto &item: ftdiDevices){
            deviceList += item;
        }
#endif
        return deviceList;
    }
}

ProviderSpi::ProviderSpi(const QJsonObject &deviceConfig)
        : LedDevice(deviceConfig),
          _spiProvider(nullptr) {
}

ProviderSpi::~ProviderSpi() {
}

bool ProviderSpi::init(const QJsonObject &deviceConfig) {
    bool isInitOK = false;

    // Initialise sub-class
    if (LedDevice::init(deviceConfig)) {
        _deviceName = deviceConfig["output"].toString(_deviceName);
        Debug(_log, "_deviceName %s", QSTRING_CSTR(_deviceName));
        _spiProvider = SPIProvider::construct(deviceConfig);
        isInitOK = true;
    }
    return isInitOK;
}

int ProviderSpi::open() {
    int retVal = -1;
    _isDeviceReady = true;
    if ((retVal = _spiProvider->open())) {
        _isDeviceReady = true;
    }
    return retVal;
}


int ProviderSpi::close() {
    // LedDevice specific closing activities
    int retVal = 0;
    _isDeviceReady = false;
    if (_spiProvider != nullptr) {
        retVal = _spiProvider->close();
    }

    return retVal;
}

int ProviderSpi::writeBytes(unsigned size, const uint8_t *data) {
    int retVal = 0;
    if ((retVal = _spiProvider->writeBytes(size, data)) < 0) {
        retVal = -1;
    }
    return retVal;
}

QJsonObject ProviderSpi::discover(const QJsonObject &params) {
    QJsonObject devicesDiscovered;
    QJsonArray deviceList = SPIProvider::discover(params);

    devicesDiscovered.insert("ledDeviceType", _activeDeviceType);
    devicesDiscovered.insert("devices", deviceList);

    Debug(_log, "SPI devices discovered: [%s]",
          QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

    return devicesDiscovered;
}
