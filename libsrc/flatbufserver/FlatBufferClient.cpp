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

Q_LOGGING_CATEGORY(flatbuffer_server_client_flow, "hyperion.flatbuffer.server.flow");
Q_LOGGING_CATEGORY(flatbuffer_server_client_cmd, "hyperion.flatbuffer.server.cmd");

// Constants
namespace {
const int FLATBUFFER_PRIORITY_MIN = 100;
const int FLATBUFFER_PRIORITY_MAX = 199;

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
	TRACK_SCOPE();
	_imageResampler.setPixelDecimation(1);

	// timer setup
	_timeoutTimer.reset(new QTimer());
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

	_timeoutTimer->start();
	_receiveBuffer += _socket->readAll();

	// check if we can read a header
	while(_receiveBuffer.size() >= 4)
	{
		// Directly read message size
		auto* raw = reinterpret_cast<const uint8_t*>(_receiveBuffer.constData());
		uint32_t const messageSize = (raw[0] << 24) | (raw[1] << 16) | (raw[2] << 8) | raw[3];

		// check if we can read a complete message
		if((uint32_t) _receiveBuffer.size() < messageSize + 4) { return; }

		// extract message without header and remove header + msg from buffer :: QByteArray::remove() does not return the removed data
		auto* msgData = reinterpret_cast<const uint8_t*>(_receiveBuffer.constData() + 4);
		_receiveBuffer.remove(0, messageSize + 4);

		flatbuffers::Verifier verifier(msgData, messageSize);

		if (!hyperionnet::VerifyRequestBuffer(verifier)) {
			Error(_log, "Invalid FlatBuffer message received");
			sendErrorReply("Invalid FlatBuffer message received");
			continue;
		}

		const auto *message = hyperionnet::GetRequest(msgData);
		handleMessage(message);
	}
}

void FlatBufferClient::noDataReceived()
{
	Error(_log,"No data received for %dms - drop connection with client \"%s\"",_timeout, QSTRING_CSTR(QString("%1@%2").arg(_origin, _clientAddress)));
	forceClose();
}

void FlatBufferClient::forceClose()
{
	qCDebug(flatbuffer_server_client_flow) << "Forcing close of client" << QString("%1@%2").arg(_origin, _clientAddress);
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
		qCDebug(flatbuffer_server_client_flow) << "Received null or invalid request from client" << QString("%1@%2").arg(_origin, _clientAddress);
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
		qCDebug(flatbuffer_server_client_flow) << "Received invalid packet from client" << QString("%1@%2").arg(_origin, _clientAddress);
		sendErrorReply("Received invalid packet.");
	}
}
void FlatBufferClient::handleColorCommand(const hyperionnet::Color *colorReq)
{
	qCDebug(flatbuffer_server_client_cmd) << "Received color command from client" << QString("%1@%2").arg(_origin, _clientAddress);

	// extract parameters
	const int32_t rgbData = colorReq->data();
	QVector<ColorRgb> const color{ ColorRgb{ uint8_t(qRed(rgbData)), uint8_t(qGreen(rgbData)), uint8_t(qBlue(rgbData)) } };

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

	qCDebug(flatbuffer_server_client_flow) << "Received register request from client" << QString("%1@%2").arg(_origin, _clientAddress) << "with priority" << _priority;

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
		const auto* img = image->data_as_RawImage();

		// Read image properties directly from FlatBuffer
		int32_t const width = img->width();
		int32_t const height = img->height();
		const auto* data = img->data();

		if (width <= 0 || height <= 0 || data == nullptr || data->empty())
		{
			qCDebug(flatbuffer_server_client_flow) << "Received RAW image command with invalid width and/or size or empty image by client" << QString("%1@%2").arg(_origin, _clientAddress);
			sendErrorReply("Invalid width and/or height or no raw image data provided");
			return;
		}

		// Check consistency of image data size
		auto dataSize = data->size();
		int const bytesPerPixel = dataSize / (width * height);
		if (bytesPerPixel != 3 && bytesPerPixel != 4)
		{
			qCDebug(flatbuffer_server_client_flow) << "Received RAW image command where the size of image data does not match with the width and height provided by client" << QString("%1@%2").arg(_origin, _clientAddress) << "";
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
		const auto* img = image->data_as_NV12Image();

			int32_t const width = img->width();
			int32_t const height = img->height();
			const auto* const data_y = img->data_y();
			const auto* data_uv = img->data_uv();

			if (width <= 0 || height <= 0 || data_y == nullptr || data_uv == nullptr ||
				data_y->empty() || data_uv->empty())
			{
				qCDebug(flatbuffer_server_client_flow) << "Received NV12 image command with invalid width and/or size or empty image by client" << QString("%1@%2").arg(_origin, _clientAddress);
				sendErrorReply("Invalid width and/or height or no complete NV12 image data provided");
				return;
			}

			// Combine Y and UV into one contiguous buffer (reuse class member buffer)
			size_t const y_size = data_y->size();
			size_t const uv_size = data_uv->size();

			size_t const required_size = y_size + uv_size;
			if (_combinedNv12Buffer.capacity() < required_size)
			{
				_combinedNv12Buffer.reserve(required_size);
			}
			std::memcpy(_combinedNv12Buffer.data(), data_y->data(), y_size);
			std::memcpy(_combinedNv12Buffer.data() + y_size, data_uv->data(), uv_size);

			// Determine stride for Y
			int32_t const stride_y = img->stride_y() > 0 ? img->stride_y() : width;

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
		qCDebug(flatbuffer_server_client_flow) << "Received image command with no or unknown image data by client" << QString("%1@%2").arg(_origin, _clientAddress);
		sendErrorReply("No or unknown image data provided");
		return;
	}

	qCDebug(flatbuffer_server_client_cmd) << "Received image from client" << QString("%1@%2").arg(_origin, _clientAddress)
										   << "with priority" << _priority
										   << "size" << _imageOutputBuffer.width() << "x" << _imageOutputBuffer.height()
										   << "and duration" << duration << "ms";

	emit setGlobalInputImage(_priority, _imageOutputBuffer, duration);
	emit setBufferImage("FlatBuffer", _imageOutputBuffer);

	// send reply
	sendSuccessReply();
}

void FlatBufferClient::handleClearCommand(const hyperionnet::Clear *clear)
{
	// extract parameters
	const int priority = clear->priority();

	qCDebug(flatbuffer_server_client_cmd) << "Received clear command from client" << QString("%1@%2").arg(_origin, _clientAddress)  << "with priority" << priority;

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
	qCDebug(flatbuffer_server_client_flow) << "Received not implemented command from client" << QString("%1@%2").arg(_origin, _clientAddress);
	sendErrorReply("Command not implemented");
}

void FlatBufferClient::registationRequired(int priority)
{
	qCDebug(flatbuffer_server_client_flow) << "Registration required for priority" << priority;
	if (_priority == priority)
	{
		_builder.Clear();
		auto reply = hyperionnet::CreateReplyDirect(_builder, nullptr, -1,  -1);
		_builder.Finish(reply);

		sendMessage(_builder.GetBufferPointer(), _builder.GetSize());
	}
}

void FlatBufferClient::sendMessage(const uint8_t* data, size_t size)
{
	_timeoutTimer->start();
	const uint8_t header[4] = {
		uint8_t((size >> 24) & 0xFF),
		uint8_t((size >> 16) & 0xFF),
		uint8_t((size >>  8) & 0xFF),
		uint8_t( size	     & 0xFF)
	};

	// write message
	_socket->write(reinterpret_cast<const char*>(header), sizeof(header));
	_socket->write(reinterpret_cast<const char *>(data), static_cast<qint64>(size));
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
											  int32_t width,
											  int32_t height,
											  int bytesPerPixel,
											  const ImageResampler& resampler,
											  Image<ColorRgb>& outputImage) const
{
	const size_t lineLength = static_cast<size_t>(width) * bytesPerPixel;
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
											   int32_t width,
											   int32_t height,
											   int32_t stride_y,
											   const ImageResampler& resampler,
											   Image<ColorRgb>& outputImage) const
{
	PixelFormat const pixelFormat = PixelFormat::NV12;

	resampler.processImage(
		nv12_data, // Combined NV12 buffer
		width,
		height,
		stride_y,
		pixelFormat,
		outputImage
	);
}
