#pragma once

// STL includes0
#include <fstream>
#include <QObject>
#include <QtNetwork>
#include <QObject>
#include <QString>
#include <QTcpSocket>

// Leddevice includes
#include <leddevice/LedDevice.h>


///
/// Implementation of the LedDevice that write the led-colors to an
/// ASCII-textfile('/home/pi/LedDevice.out')
///
class LedDeviceFadeCandy : public QObject,public LedDevice
{
        Q_OBJECT
public:
	///
	/// Constructs the test-device, which opens an output stream to the file
	///
	LedDeviceFadeCandy(const std::string& host, const uint16_t port);

	///
	/// Destructor of this test-device
	///
	virtual ~LedDeviceFadeCandy();

	///
	/// Writes the given led-color values to the output stream
	///
	/// @param ledValues The color-value per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	/// Switch the leds off
	virtual int switchOff();

    bool tryConnect();
    bool isConnected();

//public slots:

private:
    QTcpSocket _client;
    const std::string _host;
    const uint16_t _port;
    QByteArray _opc_data;
};
