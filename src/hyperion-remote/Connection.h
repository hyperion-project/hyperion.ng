#pragma once

// stl includes
#include <string>

// Qt includes
#include <QColor>
#include <QImage>
#include <QTcpSocket>

// jsoncpp includes
#include <json/json.h>

// hyperion-remote includes
#include "ColorTransformValues.h"

/// Connection class to setup an connection to the hyperion server and execute commands
class Connection
{
public:
	Connection(const std::string & address, bool printJson);
	~Connection();

	/// Set all leds to the specified color
	bool setColor(QColor color, int priority, int duration);

	/// Set the leds according to the given image (assume the image is stretched to the display size)
	bool setImage(QImage image, int priority, int duration);

	/// Retrieve a list of all occupied priority channels
	bool listPriorities();

	/// Clear the given priority channel
	bool clear(int priority);

	/// Clear all priority channels
	bool clearAll();

	/// Set the color transform of the leds
	/// Note that providing a NULL will leave the settings on the server unchanged
	bool setTransform(ColorTransformValues * threshold, ColorTransformValues * gamma, ColorTransformValues * blacklevel, ColorTransformValues * whitelevel);

private:
	/// Send a json command message and receive its reply
	Json::Value sendMessage(const Json::Value & message);

private:
	bool _printJson;

	QTcpSocket _socket;
};
