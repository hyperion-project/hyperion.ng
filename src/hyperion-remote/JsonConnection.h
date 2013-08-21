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
	JsonConnection(const std::string & address, bool printJson);
	~JsonConnection();

	/// Set all leds to the specified color
	void setColor(QColor color, int priority, int duration);

	/// Set the leds according to the given image (assume the image is stretched to the display size)
	void setImage(QImage image, int priority, int duration);

	/// Retrieve a list of all occupied priority channels
	QString getServerInfo();

	/// Clear the given priority channel
	void clear(int priority);

	/// Clear all priority channels
	void clearAll();

	/// Set the color transform of the leds
	/// Note that providing a NULL will leave the settings on the server unchanged
	void setTransform(
			double * saturation,
			double * value,
			ColorTransformValues * threshold,
			ColorTransformValues * gamma,
			ColorTransformValues * blacklevel,
			ColorTransformValues * whitelevel);

private:
	/// Send a json command message and receive its reply
	Json::Value sendMessage(const Json::Value & message);

	bool parseReply(const Json::Value & reply);

private:
	bool _printJson;

	QTcpSocket _socket;
};
