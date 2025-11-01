#ifndef FLATBUFFERCLIENT_H
#define FLATBUFFERCLIENT_H

// util
#include <utils/Logger.h>
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/Components.h>
#include "utils/ImageResampler.h"

// flatbuffer FBS
#include "hyperion_request_generated.h"

#include <QScopedPointer>
#include <QTcpSocket>
#include <QTimer>

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

	void setPixelDecimation(int decimator);

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
	/// @brief close the socket and call disconnected()
	///
	void forceClose();

	void noDataReceived();

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

	bool processNextMessage();

	///
	/// Send a message to the connected client
	/// @param data to be send
	/// @param size
	///
	void sendMessage(const uint8_t* data, size_t size);

	///
	/// Send a standard reply indicating success
	///
	void sendSuccessReply();

	///
	/// Send an error message back to the client
	///
	/// @param error String describing the error
	///
	void sendErrorReply(const QString& error);

	void processRawImage(const uint8_t* buffer, int32_t width, int32_t height, int bytesPerPixel, const ImageResampler& resampler, Image<ColorRgb>& outputImage);
	void processNV12Image(const uint8_t* nv12_data, int32_t width, int32_t height, int32_t stride_y, const ImageResampler& resampler, Image<ColorRgb>& outputImage);

private:
	QSharedPointer<Logger> _log;
	QTcpSocket * _socket;
	QString _origin;
	const QString _clientAddress;
	QScopedPointer<QTimer, QScopedPointerDeleteLater> _timeoutTimer;
	int _timeout;
	int _priority;

	QByteArray _receiveBuffer;

	ImageResampler _imageResampler;
	Image<ColorRgb> _imageOutputBuffer;
	std::vector<uint8_t> _combinedNv12Buffer;

	// Flatbuffers builder
	flatbuffers::FlatBufferBuilder _builder;
	bool _processingMessage;
};

#endif // FLATBUFFERCLIENT_H
