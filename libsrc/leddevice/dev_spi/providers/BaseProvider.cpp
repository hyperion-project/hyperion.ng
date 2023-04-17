#include "BaseProvider.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

BaseProvider::BaseProvider(const QJsonObject& deviceConfig):
    _deviceName("/dev/spidev0.0"),
    _baudRate_Hz(1000000),
    _activeDeviceType("UNSPECIFIED")
{
    _activeDeviceType = deviceConfig["type"].toString(_activeDeviceType).toLower();
    _log = Logger::getInstance("PROVIDER_" + _activeDeviceType.toUpper());

    _deviceName = deviceConfig["output"].toString(_deviceName);
    _baudRate_Hz = deviceConfig["rate"].toInt(_baudRate_Hz);

    Debug(_log, "_deviceName [%s], _baudRate_Hz [%d]", QSTRING_CSTR(_deviceName), _baudRate_Hz);
}

BaseProvider::~BaseProvider(){}

int BaseProvider::open() {
    Debug(_log, "Open");
    return 0;
}
int BaseProvider::writeBytes(unsigned size, const uint8_t *data) {
    return 0;
}

int BaseProvider::close() {
    Debug(_log, "Close");
    return 0;
}

QJsonArray BaseProvider::discover(const QJsonObject& /*params*/)
{
    QJsonArray deviceList;
    return deviceList;
}