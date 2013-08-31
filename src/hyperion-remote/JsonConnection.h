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

/// Connection class to setup an connection to the hyperion server and execute commands
class JsonConnection
{
public:
	/// @brief COnstructor
	/// @param address The address of the Hyperion server (for example "192.168.0.32:19444)
	/// @param printJson Boolean indicating if the sent and received json is written to stdout
	JsonConnection(const std::string & address, bool printJson);

	/// @brief Destructor
	~JsonConnection();

	/// @brief Set all leds to the specified color
	/// @param color The color
	/// @param priority The priority
	/// @param duration The duration in milliseconds
	void setColor(QColor color, int priority, int duration);

	/// @brief Set the leds according to the given image (assume the image is stretched to the display size)
	/// @param image The image
	/// @param priority The priority
	/// @param duration The duration in milliseconds
	void setImage(QImage image, int priority, int duration);

	/// @brief Retrieve a list of all occupied priority channels
	/// @return String with the server info
	QString getServerInfo();

	/// @brief Clear the given priority channel
	/// @param priority The priority
	void clear(int priority);

	/// @brief Clear all priority channels
	void clearAll();

	/// @brief Set the color transform of the leds
	/// @note Note that providing a NULL will leave the settings on the server unchanged
	/// @param saturation The HSV saturation gain
	/// @param value The HSV value gain
	/// @param threshold The threshold
	/// @param blacklevel The blacklevel
	/// @param whitelevel The whitelevel
	void setTransform(
			double * saturation,
			double * value,
			ColorTransformValues * threshold,
			ColorTransformValues * gamma,
			ColorTransformValues * blacklevel,
			ColorTransformValues * whitelevel);

private:
	/// @brief Send a json command message and receive its reply
	/// @param message The message to send
	/// @return The returned reply
	Json::Value sendMessage(const Json::Value & message);

	/// @brief Parse a reply message
	/// @param reply The received reply
	/// @return true if the reply indicates success
	bool parseReply(const Json::Value & reply);

private:
	bool _printJson;

	QTcpSocket _socket;
};
