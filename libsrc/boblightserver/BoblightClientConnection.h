#pragma once

// Qt includes
#include <QByteArray>
#include <QTcpSocket>
#include <QLocale>
#include <QString>

// utils includes
#include <utils/Logger.h>
#include <utils/ColorRgb.h>

/// Whether to parse floats with an eye on performance
#define FAST_FLOAT_PARSE

class ImageProcessor;
class Hyperion;

///
/// The Connection object created by \a BoblightServer when a new connection is established
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
	BoblightClientConnection(Hyperion* hyperion, QTcpSocket * socket, int priority);

	///
	/// Destructor
	///
	~BoblightClientConnection() override;

	///
	/// Get the Boblight client's IP-address
	///
	/// @returns IP-address as QString
	///
	QString getClientAddress() { return _clientAddress; }

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
	void sendMessage(const QByteArray &message);

	///
	/// Send a lights message the to connected client
	///
	void sendLightMessage();

	///
	/// Interpret the float value "0.0" to "1.0" of the QString byte values 0 .. 255
	///
	/// @param s the string to parse
	/// @param ok whether the result is ok
	/// @return the parsed byte value in range 0 to 255, or 0
	///
	uint8_t parseByte(const QString& s, bool *ok = nullptr) const;

	///
	/// Parse the given QString as unsigned int value.
	///
	/// @param s the string to parse
	/// @param ok whether the result is ok
	/// @return the parsed unsigned int value
	///
	unsigned parseUInt(const QString& s, bool *ok = nullptr) const;

	///
	/// Parse the given QString as float value, e.g. the 16-bit (wide char) QString "1" shall represent 1, "0.5" is 0.5 and so on.
	///
	/// @param s the string to parse
	/// @param ok whether the result is ok
	/// @return the parsed float value, or 0
	///
	float parseFloat(const QString& s, bool *ok = nullptr) const;

	///
	/// Read an incoming boblight message as QString
	///
	/// @param data the char data buffer of the incoming message
	/// @param size the length of the buffer buffer
	/// @returns the incoming boblight message as QString
	///
	QString readMessage(const char *data, const size_t size) const;

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

	/// logger instance
	Logger * _log;

	/// address of client
	QString _clientAddress;
};
