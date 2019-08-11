#pragma once

// util
#include <utils/Logger.h>
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/Components.h>

// flatbuffer FBS
#include "hyperion_reply_generated.h"
#include "hyperion_request_generated.h"

class QTcpSocket;
class QTimer;

namespace flatbuf {
	class HyperionRequest;
}

///
/// @brief Socket (client) of FlatBufferServer
///
class FlatBufferClient : public QObject
{
    Q_OBJECT
public:
	///
	/// @brief Construct the client
	/// @param socket   The socket
	/// @param timeout  The timeout when a client is automatically disconnected and the priority unregistered
	/// @param parent   The parent
	///
	explicit FlatBufferClient(QTcpSocket* socket, const int &timeout, QObject *parent = nullptr);

signals:
	///
	/// @brief forward register data to HyperionDaemon
	///
	void registerGlobalInput(const int priority, const hyperion::Components& component, const QString& origin = "FlatBuffer", const QString& owner = "", unsigned smooth_cfg = 0);

	///
	/// @brief Forward clear command to HyperionDaemon
	///
	void clearGlobalInput(const int priority);

	///
	/// @brief Forward clearAll command to HyperionDaemon
	///
	void clearAllGlobalInput(bool forceClearAll=false);

	///
	/// @brief forward prepared image to HyperionDaemon
	///
	const bool setGlobalInputImage(const int priority, const Image<ColorRgb>& image, const int timeout_ms, const bool& clearEffect = false);

	///
	/// @brief Forward requested color
	///
	void setGlobalInputColor(const int priority, const ColorRgb &ledColor, const int timeout_ms, const QString& origin = "FlatBuffer" ,bool clearEffects = true);

	///
	/// @brief Emits whenever the client disconnected
	///
	void clientDisconnected();

public slots:
	///
	/// @brief Requests a registration from the client
	///
	void registationRequired(const int priority);

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
	/// @brief Handle the received message
	///
	void handleMessage(const hyperionnet::Request * req);

	///
	/// Register new priority
	///
	void handleRegisterCommand(const hyperionnet::Register *regReq);

	///
	/// @brief Hande Color message
	///
	void handleColorCommand(const hyperionnet::Color *colorReq);

	///
	/// Handle an incoming Image message
	///
	/// @param image the incoming image
	///
	void handleImageCommand(const hyperionnet::Image *image);

	///
	/// @brief Handle clear command
	///
	/// @param clear the incoming clear request
	///
	void handleClearCommand(const hyperionnet::Clear *clear);

	///
	/// Send handle not implemented
	///
	void handleNotImplemented();

	///
	/// Send a message to the connected client
	///
	void sendMessage();

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
	Logger *_log;
	QTcpSocket *_socket;
	const QString _clientAddress;
	QTimer *_timeoutTimer;
	int _timeout;
	int _priority;

	QByteArray _receiveBuffer;

	// Flatbuffers builder
	flatbuffers::FlatBufferBuilder _builder;
};
