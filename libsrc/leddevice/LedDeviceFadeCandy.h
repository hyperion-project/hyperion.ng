#pragma once

// STL/Qt includes
#include <fstream>
#include <QObject>
#include <QTcpSocket>

// Leddevice includes
#include <leddevice/LedDevice.h>

///
/// Implementation of the LedDevice interface for sending to
/// fadecandy/opc-server via network by using the 'open pixel control' protocol.
///
class LedDeviceFadeCandy : public QObject, public LedDevice
{
	Q_OBJECT

public:
	///
	/// Constructs the LedDevice for fadecandy/opc server
	///
	/// @param host The ip address/host name of fadecandy/opc server
	/// @param port The port to use (fadecandy default is 7890)
	///
	LedDeviceFadeCandy(const std::string& host, const uint16_t port, const unsigned channel);

	///
	/// Destructor of the LedDevice; closes the tcp client
	///
	virtual ~LedDeviceFadeCandy();

	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	/// Switch the leds off
	virtual int switchOff();


private:
	QTcpSocket        _client;
	const std::string _host;
	const uint16_t    _port;
	const unsigned    _channel;
	QByteArray        _opc_data;

	/// try to establish connection to opc server, if not connected yet
	///
	/// @return true if connection is established
	///
	bool tryConnect();

	/// return the conenction state
	///
	/// @return True if connection established
	///
	bool isConnected();


	/// transfer current opc_data buffer to opc server
	///
	/// @return amount of transfered bytes. -1 error while transfering, -2 error while connecting
	///
	int transferData();
	
	int sendSysEx(uint8_t systemId, uint8_t commandId, QByteArray msg);

	void setGlobalColorCorrection(double gamma, double r=1.0, double g=1.0, double b=1.0);

	void setFirmwareConfig(bool noDither, bool noInterp, bool manualLED, bool ledOnOff);
	
};
