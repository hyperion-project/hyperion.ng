#pragma once

// stl includes
#include <string>

// Qt includes
#include <QByteArray>
#include <QTcpSocket>
#include <QStringList>
#include <QString>

// Hyperion includes
#include <hyperion/Hyperion.h>

//Utils includes
#include <utils/VideoMode.h>

// proto includes
#include "message.pb.h"
#include "protoserver/ProtoConnection.h"

///
/// The Connection object created by a ProtoServer when a new connection is establshed
///
class ProtoClientConnection : public QObject
{
	Q_OBJECT

public:
	///
	/// Constructor
	/// @param socket The Socket object for this connection
	/// @param hyperion The Hyperion server
	///
	ProtoClientConnection(QTcpSocket * socket);

	///
	/// Destructor
	///
	~ProtoClientConnection();

public slots:
	///
	/// Send video mode message to connected client
	///
	void setVideoMode(const VideoMode videoMode);

signals:
	///
	/// Signal which is emitted when the connection is being closed
	/// @param connection This connection object
	///
	void connectionClosed(ProtoClientConnection * connection);
	void newMessage(const proto::HyperionRequest * message);

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
	/// Handle an incoming Proto message
	///
	/// @param message the incoming message as string
	///
	void handleMessage(const proto::HyperionRequest &message);

	///
	/// Handle an incoming Proto Color message
	///
	/// @param message the incoming message
	///
	void handleColorCommand(const proto::ColorRequest & message);

	///
	/// Handle an incoming Proto Image message
	///
	/// @param message the incoming message
	///
	void handleImageCommand(const proto::ImageRequest & message);

	///
	/// Handle an incoming Proto Clear message
	///
	/// @param message the incoming message
	///
	void handleClearCommand(const proto::ClearRequest & message);

	///
	/// Handle an incoming Proto Clearall message
	///
	void handleClearallCommand();

	///
	/// Handle an incoming Proto message of unknown type
	///
	void handleNotImplemented();

	///
	/// Send a message to the connected client
	///
	/// @param message The Proto message to send
	///
	void sendMessage(const google::protobuf::Message &message);

	///
	/// Send a standard reply indicating success
	///
	void sendSuccessReply();

	///
	/// Send an error message back to the client
	///
	/// @param error String describing the error
	///
	void sendErrorReply(const std::string & error);

private:
	/// The TCP-Socket that is connected tot the Proto-client
	QTcpSocket * _socket;

	/// Link to Hyperion for writing led-values to a priority channel
	Hyperion * _hyperion;

	/// The buffer used for reading data from the socket
	QByteArray _receiveBuffer;

	int _priority;

	/// address of client
	QString _clientAddress;
};
