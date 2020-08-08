#pragma once

// Qt includes
#include <QColor>
#include <QImage>
#include <QTcpSocket>
#include <QJsonObject>

//forward class decl
class Logger;

///
/// Connection class to setup an connection to the hyperion server and execute commands
///
class JsonConnection
{
public:
	///
	/// Constructor
	///
	/// @param address The address of the Hyperion server (for example "192.168.0.32:19444)
	/// @param printJson Boolean indicating if the sent and received json is written to stdout
	///
	JsonConnection(const QString & address, bool printJson);

	///
	/// Destructor
	///
	~JsonConnection();

	///
	/// Set all leds to the specified color
	///
	/// @param color The color
	/// @param priority The priority
	/// @param duration The duration in milliseconds
	///
	void setColor(std::vector<QColor> color, int priority, int duration);

	///
	/// Set the leds according to the given image (assume the image is stretched to the display size)
	///
	/// @param image The image
	/// @param priority The priority
	/// @param duration The duration in milliseconds
	///
	void setImage(QImage &image, int priority, int duration);

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
	/// @param component The component [SMOOTHING, BLACKBORDER, FORWARDER, BOBLIGHT_SERVER, GRABBER]
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
	QString getConfig(std::string type);

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
		double *gammaR,
		double *gammaG,
		double *gammaB,
		int    *backlightThreshold,
		int    *backlightColored,
		int    *brightness,
		int    *brightnessC);

	///
	/// sets the image to leds mapping type
	///
	/// @param mappingType led mapping type
	void setLedMapping(QString mappingType);

	// sets video mode 3D/2D
	void setVideoMode(QString videoMode);

	// set the specified authorization token
	void setToken(const QString &token);

	///
	/// Send a json message with a specific instance id
	/// @param instance The instance id
	///
	void setInstance(int instance);


private:
	///
	/// Send a json command message and receive its reply
	///
	/// @param message The message to send
	///
	/// @return The returned reply
	///
	QJsonObject sendMessage(const QJsonObject & message);

	///
	/// Parse a reply message
	///
	/// @param reply The received reply
	///
	/// @return true if the reply indicates success
	///
	bool parseReply(const QJsonObject & reply);

	/// Flag for printing all send and received json-messages to the standard out
	bool _printJson;

	// Logger class
	Logger* _log;

	/// The TCP-Socket with the connection to the server
	QTcpSocket _socket;

};
