#pragma once

// util
#include <utils/Logger.h>
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/ColorRgba.h>
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
	explicit FlatBufferClient(QTcpSocket* socket, int timeout, QObject *parent = nullptr);

signals:
	///
	/// @brief forward register data to HyperionDaemon
	///
	void registerGlobalInput(int priority, hyperion::Components component, const QString& origin = "FlatBuffer", const QString& owner = "", unsigned smooth_cfg = 0);

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
	void setGlobalInputColor(int priority, const std::vector<ColorRgb> &ledColor, int timeout_ms, const QString& origin = "FlatBuffer" ,bool clearEffects = true);

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
	void registationRequired(int priority);

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
