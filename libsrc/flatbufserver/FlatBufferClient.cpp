#include "FlatBufferClient.h"
#include <utils/PixelFormat.h>
#include <utils/ColorRgba.h>

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

bool FlatBufferClient::processNextMessageInline()
{
	if (_processingMessage) { return false; } // Avoid re-entrancy

	// Wait for at least 4 bytes to read the message size
	if (_receiveBuffer.size() < 4) {
		return false;
	}

	_processingMessage = true;

	// Directly read message size (no memcpy)
	const uint8_t* raw = reinterpret_cast<const uint8_t*>(_receiveBuffer.constData());
	uint32_t messageSize = (raw[0] << 24) | (raw[1] << 16) | (raw[2] << 8) | raw[3];

	// Validate message size
	if (messageSize == 0 || messageSize > FLATBUFFER_MAX_MSG_LENGTH)
	{
		Warning(_log, "Invalid message size: %d - dropping received data", messageSize);
		_processingMessage = false;
		return true;
	}

	// Wait for full message
	if (_receiveBuffer.size() < static_cast<int>(messageSize + 4))
	{
		_processingMessage = false;
		return false;
	}

	// Extract the message and remove it from the buffer (no copying)
	const uint8_t* msgData = reinterpret_cast<const uint8_t*>(_receiveBuffer.constData() + 4);
	flatbuffers::Verifier verifier(msgData, messageSize);

	if (!hyperionnet::VerifyRequestBuffer(verifier)) {
		Error(_log, "Invalid FlatBuffer message received");
		sendErrorReply("Invalid FlatBuffer message received");
		_processingMessage = false;

		// Clear the buffer in case of an invalid message
		_receiveBuffer.clear();
		return true;
	}

	// Invoke message handling
	QMetaObject::invokeMethod(this, [this, msgData, messageSize]() {
		handleMessage(hyperionnet::GetRequest(msgData));
		_processingMessage = false;

		// Remove the processed message from the buffer (header + body)
		_receiveBuffer.remove(0, messageSize + 4);  // Clear the processed message + header

		// Continue processing the next message
		processNextMessage();
	});

	return true;
}

void FlatBufferClient::processNextMessage()
{
	// Run the message processing inline until the buffer is empty or we can't process further
	while (processNextMessageInline()) {
		// Keep processing as long as we can
	}
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
	// extract parameters
	int const duration = image->duration();

	if (image->data_as_RawImage() != nullptr)
	{
		const auto* img = static_cast<const hyperionnet::RawImage*>(image->data_as_RawImage());

		// Read image properties directly from FlatBuffer
		const int width = img->width();
		const int height = img->height();
		const auto* data = img->data();

		if (width <= 0 || height <= 0 || data == nullptr || data->size() == 0)
		{
			sendErrorReply("Invalid width and/or height or no raw image data provided");
			return;
		}

		// Check consistency of image data size
		const int dataSize = data->size();
		const int bytesPerPixel = dataSize / (width * height);
		if (bytesPerPixel != 3 && bytesPerPixel != 4)
		{
			sendErrorReply("Size of image data does not match with the width and height");
			return;
		}

		// Only resize if needed (reuse memory)
		if (_imageOutputBuffer.width() != width || _imageOutputBuffer.height() != height)
		{
			_imageOutputBuffer.resize(width, height);
		}

		processRawImage(data->data(), width, height, bytesPerPixel, _imageResampler, _imageOutputBuffer);
	}
	else if (image->data_as_NV12Image() != nullptr)
	{
		const auto* img = static_cast<const hyperionnet::NV12Image*>(image->data_as_NV12Image());

			const int width = img->width();
			const int height = img->height();
			const auto* data_y = img->data_y();
			const auto* data_uv = img->data_uv();

			if (width <= 0 || height <= 0 || data_y == nullptr || data_uv == nullptr ||
				data_y->size() == 0 || data_uv->size() == 0)
			{
				sendErrorReply("Invalid width and/or height or no complete NV12 image data provided");
				return;
			}

			// Combine Y and UV into one contiguous buffer (reuse class member buffer)
			const size_t y_size = data_y->size();
			const size_t uv_size = data_uv->size();

			size_t required_size = y_size + uv_size;
			if (_combinedNv12Buffer.capacity() < required_size)
			{
				_combinedNv12Buffer.reserve(required_size);
			}
			std::memcpy(_combinedNv12Buffer.data(), data_y->data(), y_size);
			std::memcpy(_combinedNv12Buffer.data() + y_size, data_uv->data(), uv_size);

			// Determine stride for Y
			const int stride_y = img->stride_y() > 0 ? img->stride_y() : width;

			// Resize only when needed
			if (_imageOutputBuffer.width() != width || _imageOutputBuffer.height() != height)
			{
				_imageOutputBuffer.resize(width, height);
			}

			// Process image
			processNV12Image(_combinedNv12Buffer.data(), width, height, stride_y, _imageResampler, _imageOutputBuffer);
	}
	else
	{
		sendErrorReply("No or unknown image data provided");
		return;
	}

	emit setGlobalInputImage(_priority, _imageOutputBuffer, duration);
	emit setBufferImage("FlatBuffer", _imageOutputBuffer);

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

inline void FlatBufferClient::processRawImage(const uint8_t* buffer,
											  int width,
											  int height,
											  int bytesPerPixel,
											  ImageResampler& resampler,
											  Image<ColorRgb>& outputImage)
{
	int const lineLength = width * bytesPerPixel;
	PixelFormat const pixelFormat = (bytesPerPixel == 4) ? PixelFormat::RGB32 : PixelFormat::RGB24;

	resampler.processImage(
		buffer, // Raw buffer
		width,
		height,
		lineLength,
		pixelFormat,
		outputImage
	);
}

inline void FlatBufferClient::processNV12Image(const uint8_t* nv12_data,
											   int width,
											   int height,
											   int stride_y,
											   ImageResampler& resampler,
											   Image<ColorRgb>& outputImage)
{
	PixelFormat pixelFormat = PixelFormat::NV12;

	resampler.processImage(
		nv12_data, // Combined NV12 buffer
		width,
		height,
		stride_y,
		pixelFormat,
		outputImage
	);
}
