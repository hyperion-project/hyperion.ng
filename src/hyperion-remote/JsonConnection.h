#pragma once

// stl includes
#include <string>

// Qt includes
#include <QColor>
#include <QImage>
#include <QTcpSocket>
#include <QMap>

// jsoncpp includes
#include <json/json.h>

// hyperion-remote includes
#include "ColorTransformValues.h"
#include "ColorCorrectionValues.h"
#include "ColorAdjustmentValues.h"

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
	JsonConnection(const std::string & address, bool printJson);

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
	void setImage(QImage image, int priority, int duration);

	///
	/// Start the given effect
	///
	/// @param effect The name of the effect
	/// @param effectArgs The arguments to use instead of the default ones
	/// @param priority The priority
	/// @param duration The duration in milliseconds
	///
	void setEffect(const std::string & effectName, const std::string &effectArgs, int priority, int duration);

	///
	/// Retrieve a list of all occupied priority channels
	///
	/// @return String with the server info
	///
	QString getServerInfo();

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
	/// @param component The component [SMOOTHING, BLACKBORDER, KODICHECKER, FORWARDER, UDPLISTENER, BOBLIGHT_SERVER, GRABBER]
	/// @param state The state of the component [true | false]
	///
	void setComponentState(const std::string & component, const bool state);

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
	QString getConfigFile();

	///
	/// Write JSON Value(s) to the actual loaded configuration file
	///
	/// @param jsonString The JSON String(s) to write
	/// @param create Specifies whether the nonexistent json string to be created
	///
	void setConfigFile(const std::string & jsonString, bool create);

	///
	/// Set the color transform of the leds
	///
	/// @note Note that providing a NULL will leave the settings on the server unchanged
	///
	/// @param transformId The identifier of the transform to set
	/// @param saturation The HSV saturation gain
	/// @param value The HSV value gain
	/// @param saturationL The HSL saturation gain
	/// @param luminance The HSL luminance gain
	/// @param luminanceMin The HSL luminance minimum
	/// @param threshold The threshold
	/// @param gamma The gamma value
	/// @param blacklevel The blacklevel
	/// @param whitelevel The whitelevel
	///
	void setTransform(
			std::string * transformId,
			double * saturation,
			double * value,
			double * saturationL,
			double * luminance,
			double * luminanceMin,
			ColorTransformValues * threshold,
			ColorTransformValues * gamma,
			ColorTransformValues * blacklevel,
			ColorTransformValues * whitelevel);
	
	///
	/// Set the color correction of the leds
	///
	/// @note Note that providing a NULL will leave the settings on the server unchanged
	///
	/// @param correctionId The identifier of the correction to set
	/// @param correction The correction values
	void setCorrection(
			std::string * correctionId,
			ColorCorrectionValues * correction);

	///
	/// Set the color temperature of the leds
	///
	/// @note Note that providing a NULL will leave the settings on the server unchanged
	///
	/// @param temperatureId The identifier of the correction to set
	/// @param temperature The temperature correction values
	void setTemperature(
			std::string * temperatureId,
			ColorCorrectionValues * temperature);

	///
	/// Set the color adjustment of the leds
	///
	/// @note Note that providing a NULL will leave the settings on the server unchanged
	///
	/// @param adjustmentId The identifier of the correction to set
	/// @param redAdjustment The red channel adjustment values
	/// @param greenAdjustment The green channel adjustment values
	/// @param blueAdjustment The blue channel adjustment values
	void setAdjustment(
			std::string * adjustmentId,
			ColorAdjustmentValues * redAdjustment,
			ColorAdjustmentValues * greenAdjustment,
			ColorAdjustmentValues * blueAdjustment);

private:
	///
	/// Send a json command message and receive its reply
	///
	/// @param message The message to send
	///
	/// @return The returned reply
	///
	Json::Value sendMessage(const Json::Value & message);

	///
	/// Parse a reply message
	///
	/// @param reply The received reply
	///
	/// @return true if the reply indicates success
	///
	bool parseReply(const Json::Value & reply);

private:
	/// Flag for printing all send and received json-messages to the standard out
	bool _printJson;

	/// The TCP-Socket with the connection to the server
	QTcpSocket _socket;
};
