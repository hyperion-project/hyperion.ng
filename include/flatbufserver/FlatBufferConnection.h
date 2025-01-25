#pragma once

// Qt includes
#include <QString>
#include <QColor>
#include <QImage>
#include <QTcpSocket>
#include <QTimer>
#include <QMap>
#include <QHostAddress>

// hyperion util
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/VideoMode.h>
#include <utils/Logger.h>

#include <flatbuffers/flatbuffers.h>

const int FLATBUFFER_DEFAULT_PORT = 19400;

namespace hyperionnet
{
struct Reply;
}

///
/// Connection class to setup an connection to the hyperion server and execute commands.
///
class FlatBufferConnection : public QObject
{

	Q_OBJECT

public:
	///
	/// @brief Constructor
	/// @param host The hostname or IP-address of the Hyperion Flatbuffer server (for example "192.168.0.32")
	/// @param port The port of the Hyperion Flatpuffer server (default is 19400)
	/// @param skipReply  If true skip reply
	///
	FlatBufferConnection(const QString& origin, const QString& host, int priority, bool skipReply, quint16 port = FLATBUFFER_DEFAULT_PORT);

	///
	/// @brief Destructor
	///
	~FlatBufferConnection() override;

	/// @brief Do not read reply messages from Hyperion if set to true
	void setSkipReply(bool skip);

	///
	/// @brief Set all leds to the specified color
	/// @param color The color
	/// @param priority The priority
	/// @param duration The duration in milliseconds
	///
	void setColor(const ColorRgb & color, int duration = 1);

	///
	/// @brief Clear the given priority channel
	/// @param priority The priority
	///
	void clearPriority(int priority);

	///
	/// @brief Clear all priority channels
	///
	void clearAllPriorities();

	///
	/// @brief Check, if client is already registered
	/// @return True, if reistered
	///
	bool isClientRegistered();

public slots:
	///
	/// @brief Set the leds according to the given image
	/// @param image The image
	///
	void setImage(const Image<ColorRgb> &image);
	void setImage(const QByteArray& imageData, int width, int height, int duration = -1);

signals:

	void isReadyToSend();
	void isDisconnected();
	void errorOccured(const QString& error);

private slots:
	///
	/// @brief Try to connect to the Hyperion host
	///
	void connectToRemoteHost();

	///
	/// @brief Slot called when new data has arrived
	///
	void readData();

	void onConnected();
	void onDisconnected();


signals:

	///
	/// @brief emits when a new videoMode was requested from flatbuf client
	///
	void setVideoMode(VideoMode videoMode);

private:

	///
	/// @brief Register a new priority with given origin
	/// @param origin  The user friendly origin string
	/// @param priority The priority to register
	///
	void registerClient(const QString& origin, int priority);

	void sendMessage(const uint8_t* data, size_t size);

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
	Logger * _log;

	flatbuffers::FlatBufferBuilder _builder;
	bool _isRegistered;
};
