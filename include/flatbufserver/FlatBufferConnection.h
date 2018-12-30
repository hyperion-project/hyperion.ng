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
/// Connection class to setup an connection to the hyperion server and execute commands.
///
class FlatBufferConnection : public QObject
{

	Q_OBJECT

public:
	///
	/// @brief Constructor
	/// @param address The address of the Hyperion server (for example "192.168.0.32:19444)
	/// @param skipReply  If true skip reply
	///
	FlatBufferConnection(const QString& origin, const QString & address, const int& priority, const bool& skipReply);

	///
	/// @brief Destructor
	///
	~FlatBufferConnection();

	/// @brief Do not read reply messages from Hyperion if set to true
	void setSkipReply(const bool& skip);

	///
	/// @brief Register a new priority with given origin
	/// @param origin  The user friendly origin string
	/// @param priority The priority to register
	///
	void setRegister(const QString& origin, int priority);

	///
	/// @brief Set all leds to the specified color
	/// @param color The color
	/// @param priority The priority
	/// @param duration The duration in milliseconds
	///
	void setColor(const ColorRgb & color, int priority, int duration = 1);

	///
	/// @brief Clear the given priority channel
	/// @param priority The priority
	///
	void clear(int priority);

	///
	/// @brief Clear all priority channels
	///
	void clearAll();

	///
	/// @brief Send a command message and receive its reply
	/// @param message The message to send
	///
	void sendMessage(const uint8_t* buffer, uint32_t size);

public slots:
	///
	/// @brief Set the leds according to the given image
	/// @param image The image
	///
	void setImage(const Image<ColorRgb> &image);

private slots:
	///
	/// @brief Try to connect to the Hyperion host
	///
	void connectToHost();

	///
	/// @brief Slot called when new data has arrived
	///
	void readData();

signals:

	///
	/// @brief emits when a new videoMode was requested from flatbuf client
	///
	void setVideoMode(const VideoMode videoMode);

private:

	///
	/// @brief Parse a reply message
	/// @param reply The received reply
	/// @return true if the reply indicates success
	///
	bool parseReply(const hyperionnet::Reply *reply);

private:
	/// The TCP-Socket with the connection to the server
	QTcpSocket _socket;

	QString _origin;
	int _priority;

	/// Host address
	QString _host;

	/// Host port
	uint16_t _port;

	/// buffer for reply
	QByteArray _receiveBuffer;

	QTimer _timer;
	QAbstractSocket::SocketState  _prevSocketState;

	Logger * _log;
	flatbuffers::FlatBufferBuilder _builder;

	bool _registered;
};
