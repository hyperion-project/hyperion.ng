#ifndef LEDEVICEFADECANDY_H
#define LEDEVICEFADECANDY_H

// STL/Qt includes
#include <QTcpSocket>
#include <QString>

// LedDevice includes
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
	/// @brief Constructs a LED-device for fadecandy/opc server
	///
	/// Following code shows all configuration options
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
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceFadeCandy(const QJsonObject& deviceConfig);

	///
	/// @brief Destructor of the LedDevice
	///
	~LedDeviceFadeCandy() override;

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	static LedDevice* construct(const QJsonObject& deviceConfig);

protected:

	///
	/// @brief Initialise the Nanoleaf device's configuration and network address details
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject& deviceConfig) override;

	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Closes the output device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	int close() override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb>& ledValues) override;

private:

	///
	/// @brief Initialise device's network details
	///
	/// @return True if success
	bool initNetwork();

	///
	/// @brief try to establish connection to opc server, if not connected yet
	///
	/// @return True, if connection is established
	///
	bool tryConnect();

	///
	/// @brief Return the connection state
	///
	/// @return True, if connection established
	///
	bool isConnected() const;

	///
	/// @brief Transfer current opc_data buffer to opc server
	///
	/// @return amount of transferred bytes. -1 error while transferring, -2 error while connecting
	///
	qint64 transferData();

	///
	/// @brief Send system exclusive commands
	///
	/// @param[in] systemId fadecandy device identifier (for standard fadecandy always: 1)
	/// @param[in] commandId id of command
	/// @param[in] msg the sysEx message
	/// @return amount bytes written, -1 if failed
	qint64 sendSysEx(uint8_t systemId, uint8_t commandId, const QByteArray& msg);

	///
	/// @brief Sends the configuration to fadecandy cserver
	///
	void sendFadeCandyConfiguration();

	QTcpSocket* _client;
	QString     _hostName;
	int    _port;
	int    _channel;
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
};

#endif // LEDEVICEFADECANDY_H
