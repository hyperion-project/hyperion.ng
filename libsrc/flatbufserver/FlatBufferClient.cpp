#include "FlatBufferClient.h"
#include <utils/PixelFormat.h>

// qt
#include <QTcpSocket>
#include <QTimer>
#include <QRgb>
#include <QtEndian>
#include <QHostAddress>

#include "hyperion_reply_generated.h"

// Constants
namespace {
const int FLATBUFFER_PRIORITY_MIN = 100;
const int FLATBUFFER_PRIORITY_MAX = 199;
const int FLATBUFFER_MAX_MSG_LENGTH = 10'000'000;
} //End of constants

FlatBufferClient::FlatBufferClient(QTcpSocket* socket, int timeout, QObject *parent)
	: QObject(parent)
	, _log(Logger::getInstance("FLATBUFSERVER"))
	, _socket(socket)
	, _clientAddress(socket->peerAddress().toString())
	, _timeoutTimer(nullptr)
	, _timeout(timeout * 1000)
	, _priority()
	, _processingMessage(false)
{
	_imageResampler.setPixelDecimation(1);
	
	// timer setup
	_timeoutTimer.reset(new QTimer(this));
	_timeoutTimer->setSingleShot(true);
	_timeoutTimer->setInterval(_timeout);
	connect(_timeoutTimer.get(), &QTimer::timeout, this, &FlatBufferClient::noDataReceived);

	// connect socket signals
	connect(_socket, &QTcpSocket::readyRead, this, &FlatBufferClient::readyRead);
	connect(_socket, &QTcpSocket::disconnected, this, &FlatBufferClient::disconnected);
}

void FlatBufferClient::setPixelDecimation(int decimator)
{
	_imageResampler.setPixelDecimation(decimator);
}

void FlatBufferClient::readyRead()
{
	if (_socket == nullptr) { return; }

	while (_socket->bytesAvailable() > 0)
	{
		_timeoutTimer->start();
		_receiveBuffer += _socket->readAll();
		processNextMessage();
	}
}

void FlatBufferClient::processNextMessage()
{
	if (_processingMessage) { return; } // Avoid re-entrancy

	// Wait for at least 4 bytes to read the message size
	if (_receiveBuffer.size() < 4) {
		return;
	}

	_processingMessage = true;

	uint32_t messageSize;
	memcpy(&messageSize, _receiveBuffer.constData(), sizeof(uint32_t));
	messageSize = qFromBigEndian(messageSize);

	// Validate message size
	if (messageSize == 0 || messageSize > FLATBUFFER_MAX_MSG_LENGTH)
	{
		Warning(_log, "Invalid message size: %d - dropping received data", messageSize);
		_receiveBuffer.clear();
		_processingMessage = false;
		return;
	}

	// Wait for full message
	if (_receiveBuffer.size() < static_cast<int>(messageSize + 4))
	{
		_processingMessage = false;
		return;
	}

	// Extract the message and remove it from the buffer
	_lastMessage = _receiveBuffer.mid(4, messageSize);
	_receiveBuffer.remove(0, messageSize + 4);

	const uint8_t* msgData = reinterpret_cast<const uint8_t*>(_lastMessage.constData());
	flatbuffers::Verifier verifier(msgData, messageSize);

	if (!hyperionnet::VerifyRequestBuffer(verifier)) {
		Error(_log, "Invalid FlatBuffer message received");
		sendErrorReply("Invalid FlatBuffer message received");
		_processingMessage = false;
		QMetaObject::invokeMethod(this, &FlatBufferClient::processNextMessage, Qt::QueuedConnection);
		return;
	}

	// Invoke message handling
	QMetaObject::invokeMethod(this, [this]() {
		const auto* msgData = reinterpret_cast<const uint8_t*>(_lastMessage.constData());
		handleMessage(hyperionnet::GetRequest(msgData));
		_processingMessage = false;
		QMetaObject::invokeMethod(this, &FlatBufferClient::processNextMessage, Qt::QueuedConnection);
	});
}

void FlatBufferClient::noDataReceived()
{
	Error(_log,"No data received for %dms - drop connection with client \"%s\"",_timeout, QSTRING_CSTR(QString("%1@%2").arg(_origin, _clientAddress)));
	forceClose();
}

void FlatBufferClient::forceClose()
{
	_socket->close();
}

void FlatBufferClient::disconnected()
{
	Debug(_log, "Disconnected client \"%s\"", QSTRING_CSTR(QString("%1@%2").arg(_origin, _clientAddress)));
	_socket->deleteLater();
	if (_priority != 0 && _priority >= FLATBUFFER_PRIORITY_MIN && _priority <= FLATBUFFER_PRIORITY_MAX)
	{
		emit clearGlobalInput(_priority);
	}

	emit clientDisconnected();
}

void FlatBufferClient::handleMessage(const hyperionnet::Request* req)
{
	if (req == nullptr) {
		sendErrorReply("Received null or invalid request.");
		return;
	}

	// Check command type and handle accordingly
	if (req->command_as_Color() != nullptr)
	{
		handleColorCommand(req->command_as_Color());
	}
	else if (req->command_as_Image() != nullptr)
	{
		handleImageCommand(req->command_as_Image());
	}
	else if (req->command_as_Clear() != nullptr)
	{
		handleClearCommand(req->command_as_Clear());
	}
	else if (req->command_as_Register() != nullptr)
	{
		handleRegisterCommand(req->command_as_Register());
	}
	else
	{
		sendErrorReply("Received invalid packet.");
	}
}
void FlatBufferClient::handleColorCommand(const hyperionnet::Color *colorReq)
{
	// extract parameters
	const int32_t rgbData = colorReq->data();
	std::vector<ColorRgb> const color{ ColorRgb{ uint8_t(qRed(rgbData)), uint8_t(qGreen(rgbData)), uint8_t(qBlue(rgbData)) } };

	// set output
	emit setGlobalInputColor(_priority, color, colorReq->duration());

	// send reply
	sendSuccessReply();
}

void FlatBufferClient::handleRegisterCommand(const hyperionnet::Register *regReq)
{
	if (regReq->priority() < FLATBUFFER_PRIORITY_MIN || regReq->priority() > FLATBUFFER_PRIORITY_MAX)
	{
		Error(_log, "Register request from client \"%s\" contains invalid priority %d. Valid priority for Flatbuffer connections is between %d and %d.", QSTRING_CSTR(_clientAddress), regReq->priority(), FLATBUFFER_PRIORITY_MIN, FLATBUFFER_PRIORITY_MAX);

		QString const errorText = QString("The priority %1 is not in the priority range between %2 and %3").arg(regReq->priority(), FLATBUFFER_PRIORITY_MIN, FLATBUFFER_PRIORITY_MAX);
		sendErrorReply(errorText);
		return;
	}

	_priority = regReq->priority();
	_origin = regReq->origin()->c_str();
	emit registerGlobalInput(_priority, hyperion::COMP_FLATBUFSERVER, QSTRING_CSTR(QString("%1@%2").arg(_origin, _clientAddress)));

	_builder.Clear();
	auto reply = hyperionnet::CreateReplyDirect(_builder, nullptr, -1, ((_priority != 0) ? _priority : -1));
	_builder.Finish(reply);

	sendMessage(_builder.GetBufferPointer(), _builder.GetSize());
}

void FlatBufferClient::handleImageCommand(const hyperionnet::Image *image)
{
	Image<ColorRgb> imageRGB;

	// extract parameters
	int const duration = image->duration();
	if (image->data_as_RawImage() != nullptr)
	{
		const auto* img = static_cast<const hyperionnet::RawImage*>(image->data_as_RawImage());

		hyperionnet::RawImageT rawImageNative;
		img->UnPackTo(&rawImageNative);

		const int width = rawImageNative.width;
		const int height = rawImageNative.height;

		if (width <= 0 || height <= 0 || rawImageNative.data.empty())
		{
			sendErrorReply("Invalid width and/or height or no raw image data provided");
			return;
		}

		// check consistency of the size of the received data
		int const bytesPerPixel = rawImageNative.data.size() / (width * height);
		if (bytesPerPixel != 3 && bytesPerPixel != 4)
		{
			sendErrorReply("Size of image data does not match with the width and height");
			return;
		}

		imageRGB.resize(width, height);
		processRawImage(rawImageNative, bytesPerPixel, _imageResampler, imageRGB);
	}
	else if (image->data_as_NV12Image() != nullptr)
	{
		const auto* img = static_cast<const hyperionnet::NV12Image*>(image->data_as_NV12Image());

		hyperionnet::NV12ImageT nv12ImageNative;
		img->UnPackTo(&nv12ImageNative);

		const int width = nv12ImageNative.width;
		const int height = nv12ImageNative.height;

		if (width <= 0 || height <= 0 || nv12ImageNative.data_y.empty() || nv12ImageNative.data_uv.empty())
		{
			sendErrorReply("Invalid width and/or height or no complete NV12 image data provided");
			return;
		}

		imageRGB.resize(width, height);
		processNV12Image(nv12ImageNative, _imageResampler, imageRGB);
	}
	else
	{
		sendErrorReply("No or unknown image data provided");
		return;
	}

	emit setGlobalInputImage(_priority, imageRGB, duration);
	emit setBufferImage("FlatBuffer", imageRGB);

	// send reply
	sendSuccessReply();
}

void FlatBufferClient::handleClearCommand(const hyperionnet::Clear *clear)
{
	// extract parameters
	const int priority = clear->priority();

	// Check if we are clearing ourselves.
	if (priority == _priority)
	{
		_priority = -1;
	}

	emit clearGlobalInput(priority);

	sendSuccessReply();
}

void FlatBufferClient::handleNotImplemented()
{
	sendErrorReply("Command not implemented");
}

void FlatBufferClient::sendMessage(const uint8_t* data, size_t size)
{
	_timeoutTimer->start();
	const uint8_t header[4] = {
		uint8_t((size >> 24) & 0xFF),
		uint8_t((size >> 16) & 0xFF),
		uint8_t((size >>  8) & 0xFF),
		uint8_t((size	   ) & 0xFF)
	};

	// write message
	_socket->write(reinterpret_cast<const char*>(header), sizeof(header));
	_socket->write(reinterpret_cast<const char *>(data), size);
	_socket->flush();
}

void FlatBufferClient::sendSuccessReply()
{
	_builder.Clear();
	auto reply = hyperionnet::CreateReplyDirect(_builder, nullptr, -1, ((_priority != 0) ? _priority : -1));
	_builder.Finish(reply);

	sendMessage(_builder.GetBufferPointer(), _builder.GetSize());
}

void FlatBufferClient::sendErrorReply(const QString& error)
{
	_builder.Clear();
	auto reply = hyperionnet::CreateReplyDirect(_builder, QSTRING_CSTR(error));
	_builder.Finish(reply);

	sendMessage(_builder.GetBufferPointer(), _builder.GetSize());
}

inline void FlatBufferClient::processRawImage(const hyperionnet::RawImageT& raw_image, int bytesPerPixel, ImageResampler& resampler, Image<ColorRgb>& outputImage) {
	
	int const width = raw_image.width;
	int const height = raw_image.height;

	int const lineLength = width * bytesPerPixel;
	PixelFormat const pixelFormat = (bytesPerPixel == 4) ? PixelFormat::RGB32 : PixelFormat::RGB24;

	// Process the image
	resampler.processImage(
				raw_image.data.data(),	  // Raw RGB/RGBA buffer
				width,                    // Image width
				height,                   // Image height
				lineLength,               // Line length
				pixelFormat,              // Pixel format (RGB24/RGB32)
				outputImage               // Output image
				);
}

inline void FlatBufferClient::processNV12Image(const hyperionnet::NV12ImageT& nv12_image, ImageResampler& resampler, Image<ColorRgb>& outputImage) {
	// Combine data_y and data_uv into a single buffer
	int const width = nv12_image.width;
	int const height = nv12_image.height;

	size_t const y_size = nv12_image.data_y.size();
	size_t const uv_size = nv12_image.data_uv.size();
	std::vector<uint8_t> combined_buffer(y_size + uv_size);

	std::memcpy(combined_buffer.data(), nv12_image.data_y.data(), y_size);
	std::memcpy(combined_buffer.data() + y_size, nv12_image.data_uv.data(), uv_size);

	// Determine line length (stride_y)
	int const lineLength = nv12_image.stride_y > 0 ? nv12_image.stride_y : width;

	PixelFormat const pixelFormat = PixelFormat::NV12;

	// Process the image
	resampler.processImage(
				combined_buffer.data(),   // Combined NV12 buffer
				width,                    // Image width
				height,                   // Image height
				lineLength,               // Line length for Y plane
				pixelFormat,              // Pixel format (NV12)
				outputImage               // Output image
				);
}
