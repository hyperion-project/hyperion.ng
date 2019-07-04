#pragma once

// STL/Qt includes
#include <QTcpSocket>
#include <QString>

// Leddevice includes
#include <leddevice/LedDevice.h>

///
/// Implementation of the LedDevice interface for sending to
/// fadecandy/opc-server via network by using the 'open pixel control' protocol.
///
class LedDeviceFadeCandy : public LedDevice
{
	Q_OBJECT

public:
	///
	/// Constructs the LedDevice for fadecandy/opc server
	///
	/// following code shows all config options
	/// @code
	/// "device" :
	/// {
	/// 	"name"          : "MyPi",
	/// 	"type"          : "fadecandy",
	/// 	"output"        : "localhost",
	/// 	"colorOrder"    : "rgb",
	/// 	"setFcConfig"   : false,
	/// 	"gamma"         : 1.0,
	/// 	"whitepoint"    : [1.0, 1.0, 1.0],
	/// 	"dither"        : false,
	/// 	"interpolation" : false,
	/// 	"manualLed"     : false,
	/// 	"ledOn"         : false
	/// }, 
	///@endcode
	///
	/// @param deviceConfig json config for fadecandy
	///
	LedDeviceFadeCandy(const QJsonObject &deviceConfig);

	///
	/// Destructor of the LedDevice; closes the tcp client
	///
	virtual ~LedDeviceFadeCandy();

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject &deviceConfig);

private:
	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb>& ledValues);

protected:
	QTcpSocket* _client;
	QString     _host;
	uint16_t    _port;
	unsigned    _channel;
	QByteArray  _opc_data;

	// fadecandy sysEx
	bool        _setFcConfig;
	double      _gamma;
	double      _whitePoint_r;
	double      _whitePoint_g;
	double      _whitePoint_b;
	bool        _noDither;
	bool        _noInterp;
	bool        _manualLED;
	bool        _ledOnOff;

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
	
	/// send system exclusive commands
	///
	/// @param systemId fadecandy device identifier (for standard fadecandy always: 1)
	/// @param commandId id of command
	/// @param msg the sysEx message
	/// @return amount bytes written, -1 if fail
	int sendSysEx(uint8_t systemId, uint8_t commandId, QByteArray msg);

	/// sends the configuration to fcserver
	void sendFadeCandyConfiguration();

};
