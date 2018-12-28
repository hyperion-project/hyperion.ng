#pragma once

// Qt includes
#include <QString>
#include <QColor>
#include <QImage>
#include <QTcpSocket>
#include <QTimer>
#include <QMap>

// hyperion util
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/VideoMode.h>
#include <utils/Logger.h>

// flatbuffer FBS
#include "hyperion_reply_generated.h"
#include "hyperion_request_generated.h"

///
/// Connection class to setup an connection to the hyperion server and execute commands. Used from standalone capture binaries (x11/dispamnx/...)
///
class FlatBufferConnection : public QObject
{

	Q_OBJECT

public:
	///
	/// Constructor
	///
	/// @param address The address of the Hyperion server (for example "192.168.0.32:19444)
	///
	FlatBufferConnection(const QString & address);

	///
	/// Destructor
	///
	~FlatBufferConnection();

	/// Do not read reply messages from Hyperion if set to true
	void setSkipReply(bool skip);

	///
	/// Set all leds to the specified color
	///
	/// @param color The color
	/// @param priority The priority
	/// @param duration The duration in milliseconds
	///
	void setColor(const ColorRgb & color, int priority, int duration = 1);

	///
	/// Set the leds according to the given image (assume the image is stretched to the display size)
	///
	/// @param image The image
	/// @param priority The priority
	/// @param duration The duration in milliseconds
	///
	void setImage(const Image<ColorRgb> & image, int priority, int duration = -1);

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
	/// Send a command message and receive its reply
	///
	/// @param message The message to send
	///
	void sendMessage(const uint8_t* buffer, uint32_t size);

private slots:
	/// Try to connect to the Hyperion host
	void connectToHost();

	///
	/// Slot called when new data has arrived
	///
	void readData();

signals:

	///
	/// emits when a new videoMode was requested from flatbuf client
	///
	void setVideoMode(const VideoMode videoMode);

private:

	///
	/// Parse a reply message
	///
	/// @param reply The received reply
	///
	/// @return true if the reply indicates success
	///
	bool parseReply(const flatbuf::HyperionReply * reply);

private:
	/// The TCP-Socket with the connection to the server
	QTcpSocket _socket;

	/// Host address
	QString _host;

	/// Host port
	uint16_t _port;

	/// Skip receiving reply messages from Hyperion if set
	bool _skipReply;

	QTimer _timer;
	QAbstractSocket::SocketState  _prevSocketState;

	Logger * _log;
	flatbuffers::FlatBufferBuilder _builder;
};
