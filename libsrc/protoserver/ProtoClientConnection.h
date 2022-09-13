#pragma once

// protobuffer PROTO
// protobuf defines an Error() function itself, so undef it here
#undef Error
#include "message.pb.h"

// util
#include <utils/Logger.h>
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/ColorRgba.h>
#include <utils/Components.h>

class QTcpSocket;
class QTimer;

namespace proto {
	class HyperionRequest;
}

///
/// The Connection object created by a ProtoServer when a new connection is established
///
class ProtoClientConnection : public QObject
{
	Q_OBJECT

public:
	///
	/// @brief Construct the client
	/// @param socket   The socket
	/// @param timeout  The timeout when a client is automatically disconnected and the priority unregistered
	/// @param parent   The parent
	///
	explicit ProtoClientConnection(QTcpSocket* socket, int timeout, QObject *parent);

signals:
	///
	/// @brief forward register data to HyperionDaemon
	///
	void registerGlobalInput(int priority, hyperion::Components component, const QString& origin = "ProtoBuffer", const QString& owner = "", unsigned smooth_cfg = 0);

	///
	/// @brief Forward clear command to HyperionDaemon
	///
	void clearGlobalInput(int priority, bool forceClearAll=false);

	///
	/// @brief forward prepared image to HyperionDaemon
	///
	bool setGlobalInputImage(int priority, const Image<ColorRgb>& image, int timeout_ms, bool clearEffect = false);

	///
	/// @brief Forward requested color
	///
	void setGlobalInputColor(int priority, const std::vector<ColorRgb> &ledColor, int timeout_ms, const QString& origin = "ProtoBuffer" ,bool clearEffects = true);

	///
	/// @brief Emit the final processed image
	///
	void setBufferImage(const QString& name, const Image<ColorRgb>& image);

	///
	/// @brief Emits whenever the client disconnected
	///
	void clientDisconnected();

public slots:
	///
	/// @brief Requests a registration from the client
	///
	void registationRequired(int priority) { if (_priority == priority) _priority = -1; };

	///
	/// @brief close the socket and call disconnected()
	///
	void forceClose();

private slots:
	///
	/// @brief Is called whenever the socket got new data to read
	///
	void readyRead();

	///
	/// @brief Is called when the socket closed the connection, also requests thread exit
	///
	void disconnected();

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
	Logger*_log;

	/// The TCP-Socket that is connected tot the Proto-client
	QTcpSocket* _socket;

	/// address of client
	const QString _clientAddress;

	QTimer*_timeoutTimer;
	int _timeout;
	int _priority;

	/// The buffer used for reading data from the socket
	QByteArray _receiveBuffer;
};
