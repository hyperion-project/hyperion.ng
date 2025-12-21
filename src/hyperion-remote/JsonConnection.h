#pragma once

// Qt includes
#include <QColor>
#include <QImage>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonObject>
#include <QJsonArray>
#include <QScopedPointer>

//forward class decl
class Logger;

const int JSONAPI_DEFAULT_PORT = 19444;

///
/// Connection class to setup an connection to the hyperion server and execute commands
///
class JsonConnection : public QObject
{
	Q_OBJECT

public:
	/// 
	/// Constructor
	///
	/// @param address The hostname or IP-address of the Hyperion JSON-server (for example "192.168.0.32")
	/// @param address The port of the Hyperion JSON-server (default port = 19444)
	/// @param printJson Boolean indicating if the sent and received json is written to stdout
	///
	JsonConnection(const QHostAddress& address, bool printJson, quint16 port = JSONAPI_DEFAULT_PORT);
	JsonConnection(const QString& hostname, bool printJson, quint16 port = JSONAPI_DEFAULT_PORT);

	///
	/// Set all leds to the specified color
	///
	/// @param color The color
	/// @param priority The priority
	/// @param duration The duration in milliseconds
	///
	void setColor(const QList<QColor>& color, int priority, int duration);

	///
	/// Set the LEDs according to the given image (assume the image is stretched to the display size)
	///
	/// @param image The image
	/// @param priority The priority
	/// @param duration The duration in milliseconds
	/// @param name The image's filename
	///
	void setImage(const QImage &image, int priority, int duration, const QString& name = "");

#if defined(ENABLE_EFFECTENGINE)
	///
	/// Start the given effect
	///
	/// @param effectName The name of the effect
	/// @param effectArgs The arguments to use instead of the default ones
	/// @param priority The priority
	/// @param duration The duration in milliseconds
	///
	void setEffect(const QString & effectName, const QString &effectArgs, int priority, int duration);

	///
	/// Create a effect configuration file (.json)
	///
	/// @param effectName The name of the effect
	/// @param effectScript The name of the Python effect file
	/// @param effectArgs The arguments of the effect
	///
	void createEffect(const QString &effectName, const QString &effectScript, const QString & effectArgs);

	///
	/// Delete a effect configuration file (.json)
	///
	/// @param effectName The name of the effect
	///
	void deleteEffect(const QString &effectName);
#endif

	///
	/// Retrieve entire serverinfo as String
	///
	/// @return String with the server info
	///
	QString getServerInfoString();

	///
	/// Get the entire serverinfo
	///
	/// @return QJsonObject with the server info
	///
	QJsonObject getServerInfo();

	///
	/// Retrieve system info
	///
	/// @return String with the sys info
	///
	QString getSysInfo();

	///
	/// Suspend Hyperion. Stop all instances and components
	///
	void suspend();

	///
	/// Resume Hyperion. Start all instances and components
	///
	void resume();

	///
	/// Toggle between Suspend and Resume
	///
	void toggleSuspend();

	///
	///  Put Hyperion in Idle mode, i.e. all instances, components will be disabled besides the output processing (LED-devices, smoothing).
	///
	void idle();

	///
	/// Toggle between Idle and Working mode
	///
	void toggleIdle();

	///
	/// Restart Hyperion
	///
	void restart();

	///
	/// Clear the given priority channel
	///
	/// @param priority The priority
	///
	void clear(int priority);

	///
	/// Clear all priority channels
	///
	void clearAll();

	///
	/// Enable/Disable components during runtime
	///
	/// @param component The component [SMOOTHING, BLACKBORDER, FORWARDER, BOBLIGHTSERVER, GRABBER]
	/// @param state The state of the component [true | false]
	///
	void setComponentState(const QString & component, bool state);

	///
	/// Set current active priority channel and deactivate auto source switching
	///
	/// @param priority The priority
	///
	void setSource(int priority);

	///
	/// Enables auto source, if disabled prio by manual selecting input source
	///
	void setSourceAutoSelect();

	///
	/// Print the current loaded Hyperion configuration file
	///
	QString getConfig(const QString& type);

	///
	/// Write JSON Value(s) to the actual loaded configuration file
	///
	/// @param jsonString The JSON String(s) to write
	/// @param create Specifies whether the nonexistent json string to be created
	///
	void setConfig(const QString &jsonString);

	///
	/// Set the color adjustment of the leds
	///
	/// @note Note that providing a NULL will leave the settings on the server unchanged
	///
	/// @param adjustmentId The identifier of the correction to set
	/// @param redAdjustment The red channel adjustment values
	/// @param greenAdjustment The green channel adjustment values
	/// @param blueAdjustment The blue channel adjustment values
	/// @param gamma The gamma value
	/// @param backlightThreshold The threshold aka backlight
	/// @param brightness The threshold aka upper brightness limit

	void setAdjustment(
			const QString & adjustmentId,
			const QColor & redAdjustment,
			const QColor & greenAdjustment,
			const QColor & blueAdjustment,
			const QColor & cyanAdjustment,
			const QColor & magentaAdjustment,
			const QColor & yellowAdjustment,
			const QColor & blackAdjustment,
			const QColor & whiteAdjustment,
			const double *gammaR,
			const double *gammaG,
			const double *gammaB,
			const int    *backlightThreshold,
			const int    *backlightColored,
			const int    *brightness,
			const int    *brightnessC);

	///
	/// sets the image to leds mapping type
	///
	/// @param mappingType led mapping type
	void setLedMapping(const QString& mappingType);

	// sets video mode 3D/2D
	void setVideoMode(const QString& videoMode);

	// set the specified authorization token
	bool setToken(const QString& token);

	///
	/// Send a json message with a specific instance id
	/// @param instance The instance id
	///
	bool setInstance(int instance);

public slots:

	void close();

signals:

	void isReadyToSend();
	void isMessageReceived(const QJsonObject &resp);
	void isDisconnected();
	void errorOccured(const QString& error);

private slots:

	void onConnected();
	void onReadyRead();
	void onDisconnected();
	void onErrorOccured();

private:

	///
	/// @brief Try to connect to the Hyperion host
	///
	void connectToRemoteHost();

	///
	/// Send a json command message and receive its reply
	///
	/// @param message The message to send
	/// @param instanceIds A list of instanceIDs the command should be applied to
	///
	/// @return The returned reply
	///
	QJsonObject sendMessageSync(const QJsonObject& message, const QJsonArray& instanceIds = {});

	///
	/// Send a json command message
	///
	/// @param message The message to send
	/// @param instanceIds A list of instanceIDs the command should be applied to
	///
	void sendMessage(const QJsonObject& message, const QJsonArray& instanceIds = {});

	///
	/// Parse a reply message
	///
	/// @param reply The received reply
	///
	/// @return true if the reply indicates success
	///
	bool parseReply(const QJsonObject & reply);

	// Logger class
	QSharedPointer<Logger> _log;

	/// The TCP-Socket with the connection to the server
	QScopedPointer<QTcpSocket,QScopedPointerDeleteLater> _socket;

	/// Host address
	QString _hostname;

	/// Host port
	uint16_t _port;

	/// Flag for printing all send and received json-messages to the standard out
	bool _printJson;

	/// buffer for reply
	QByteArray _receiveBuffer;

};
