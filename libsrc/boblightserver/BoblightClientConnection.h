#pragma once

// stl includes
#include <string>

// Qt includes
#include <QByteArray>
#include <QTcpSocket>
#include <QLocale>

// Hyperion includes
#include <hyperion/Hyperion.h>

class ImageProcessor;

///
/// The Connection object created by \a BoblightServer when a new connection is establshed
///
class BoblightClientConnection : public QObject
{
	Q_OBJECT

public:
	///
	/// Constructor
	/// @param socket The Socket object for this connection
	/// @param hyperion The Hyperion server
	///
	BoblightClientConnection(QTcpSocket * socket, Hyperion * hyperion);

	///
	/// Destructor
	///
	~BoblightClientConnection();

signals:
	///
	/// Signal which is emitted when the connection is being closed
	/// @param connection This connection object
	///
	void connectionClosed(BoblightClientConnection * connection);

private slots:
	///
	/// Slot called when new data has arrived
	///
	void readData();

	///
	/// Slot called when this connection is being closed
	///
	void socketClosed();

private:
	///
	/// Handle an incoming boblight message
	///
	/// @param message the incoming message as string
	///
	void handleMessage(const QString &message);

	///
	/// Send a message to the connected client
	///
	/// @param message The boblight message to send
	///
	void sendMessage(const std::string &message);

	///
	/// Send a message to the connected client
	///
	/// @param message The boblight message to send
	/// @param size The size of the message
	///
	void sendMessage(const char * message, int size);

	///
	/// Send a lights message the to connected client
	///
	void sendLightMessage();

private:
	/// Locale used for parsing floating point values
	QLocale _locale;

	/// The TCP-Socket that is connected tot the boblight-client
	QTcpSocket * _socket;

	/// The processor for translating images to led-values
	ImageProcessor * _imageProcessor;

	/// Link to Hyperion for writing led-values to a priority channel
	Hyperion * _hyperion;

	/// The buffer used for reading data from the socket
	QByteArray _receiveBuffer;

	/// The priority used by this connection
	int _priority;

	/// The latest led color data
	std::vector<ColorRgb> _ledColors;
};
