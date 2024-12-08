#include "FlatBufferClient.h"
#include <utils/PixelFormat.h>

// qt
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QRgb>

FlatBufferClient::FlatBufferClient(QTcpSocket* socket, int timeout, QObject *parent)
	: QObject(parent)
	, _log(Logger::getInstance("FLATBUFSERVER"))
	, _socket(socket)
	, _clientAddress("@"+socket->peerAddress().toString())
	, _timeoutTimer(new QTimer(this))
	, _timeout(timeout * 1000)
	, _priority()
{
	_imageResampler.setPixelDecimation(1);
	
	// timer setup
	_timeoutTimer->setSingleShot(true);
	_timeoutTimer->setInterval(_timeout);
	connect(_timeoutTimer, &QTimer::timeout, this, &FlatBufferClient::forceClose);

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
	_timeoutTimer->start();

	_receiveBuffer += _socket->readAll();

	// check if we can read a header
	while(_receiveBuffer.size() >= 4)
	{
		uint32_t messageSize =
			((_receiveBuffer[0]<<24) & 0xFF000000) |
			((_receiveBuffer[1]<<16) & 0x00FF0000) |
			((_receiveBuffer[2]<< 8) & 0x0000FF00) |
			((_receiveBuffer[3]    ) & 0x000000FF);

		// check if we can read a complete message
		if((uint32_t) _receiveBuffer.size() < messageSize + 4) return;

		// extract message only and remove header + msg from buffer :: QByteArray::remove() does not return the removed data
		const QByteArray msg = _receiveBuffer.mid(4, messageSize);
		_receiveBuffer.remove(0, messageSize + 4);

		const auto* msgData = reinterpret_cast<const uint8_t*>(msg.constData());
		flatbuffers::Verifier verifier(msgData, messageSize);

		if (hyperionnet::VerifyRequestBuffer(verifier))
		{
			auto message = hyperionnet::GetRequest(msgData);
			handleMessage(message);
			continue;
		}
		sendErrorReply("Unable to parse message");
	}
}

void FlatBufferClient::forceClose()
{
	_socket->close();
}

void FlatBufferClient::disconnected()
{
	Debug(_log, "Socket Closed");
	_socket->deleteLater();
	if (_priority != 0 && _priority >= 100 && _priority < 200)
		emit clearGlobalInput(_priority);

	emit clientDisconnected();
}

void FlatBufferClient::handleMessage(const hyperionnet::Request * req)
{
	const void* reqPtr;
	if ((reqPtr = req->command_as_Color()) != nullptr) {
		handleColorCommand(static_cast<const hyperionnet::Color*>(reqPtr));
	} else if ((reqPtr = req->command_as_Image()) != nullptr) {
		handleImageCommand(static_cast<const hyperionnet::Image*>(reqPtr));
	} else if ((reqPtr = req->command_as_Clear()) != nullptr) {
		handleClearCommand(static_cast<const hyperionnet::Clear*>(reqPtr));
	} else if ((reqPtr = req->command_as_Register()) != nullptr) {
		handleRegisterCommand(static_cast<const hyperionnet::Register*>(reqPtr));
	} else {
		sendErrorReply("Received invalid packet.");
	}
}

void FlatBufferClient::handleColorCommand(const hyperionnet::Color *colorReq)
{
	// extract parameters
	const int32_t rgbData = colorReq->data();
	std::vector<ColorRgb> color{ ColorRgb{ uint8_t(qRed(rgbData)), uint8_t(qGreen(rgbData)), uint8_t(qBlue(rgbData)) } };

	// set output
	emit setGlobalInputColor(_priority, color, colorReq->duration());

	// send reply
	sendSuccessReply();
}

void FlatBufferClient::registationRequired(int priority)
{
	if (_priority == priority)
	{
		auto reply = hyperionnet::CreateReplyDirect(_builder, nullptr, -1, -1);
		_builder.Finish(reply);

		// send reply
		sendMessage();

		_builder.Clear();
	}
}

void FlatBufferClient::handleRegisterCommand(const hyperionnet::Register *regReq)
{
	if (regReq->priority() < 100 || regReq->priority() >= 200)
	{
		Error(_log, "Register request from client %s contains invalid priority %d. Valid priority for Flatbuffer connections is between 100 and 199.", QSTRING_CSTR(_clientAddress), regReq->priority());
		sendErrorReply("The priority " + std::to_string(regReq->priority()) + " is not in the priority range between 100 and 199.");
		return;
	}

	_priority = regReq->priority();
	emit registerGlobalInput(_priority, hyperion::COMP_FLATBUFSERVER, regReq->origin()->c_str()+_clientAddress);

	auto reply = hyperionnet::CreateReplyDirect(_builder, nullptr, -1, (_priority ? _priority : -1));
	_builder.Finish(reply);

	// send reply
	sendMessage();

	_builder.Clear();
}

void FlatBufferClient::handleImageCommand(const hyperionnet::Image *image)
{
	Image<ColorRgb> imageRGB;

	// extract parameters
	int duration = image->duration();
	const void* reqPtr;
	if ((reqPtr = image->data_as_RawImage()) != nullptr)
	{
		const auto* img = static_cast<const hyperionnet::RawImage*>(reqPtr);

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
		int bytesPerPixel = rawImageNative.data.size() / (width * height);
		if (bytesPerPixel != 3 && bytesPerPixel != 4)
		{
			sendErrorReply("Size of image data does not match with the width and height");
			return;
		}

		imageRGB.resize(width, height);
		processRawImage(rawImageNative, bytesPerPixel, _imageResampler, imageRGB);
	}
	else if ((reqPtr = image->data_as_NV12Image()) != nullptr)
	{
		const auto* img = static_cast<const hyperionnet::NV12Image*>(reqPtr);

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
	if (priority == _priority) {
		_priority = -1;
	}

	emit clearGlobalInput(priority);

	sendSuccessReply();
}

void FlatBufferClient::handleNotImplemented()
{
	sendErrorReply("Command not implemented");
}

void FlatBufferClient::sendMessage()
{
	auto size = _builder.GetSize();
	const uint8_t* buffer = _builder.GetBufferPointer();
	uint8_t sizeData[] = {uint8_t(size >> 24), uint8_t(size >> 16), uint8_t(size >> 8), uint8_t(size)};
	_socket->write((const char *) sizeData, sizeof(sizeData));
	_socket->write((const char *)buffer, size);
	_socket->flush();
}

void FlatBufferClient::sendSuccessReply()
{
	auto reply = hyperionnet::CreateReplyDirect(_builder);
	_builder.Finish(reply);

	// send reply
	sendMessage();

	_builder.Clear();
}

void FlatBufferClient::sendErrorReply(const std::string &error)
{
	// create reply
	auto reply = hyperionnet::CreateReplyDirect(_builder, error.c_str());
	_builder.Finish(reply);

	// send reply
	sendMessage();

	_builder.Clear();
}

inline void FlatBufferClient::processRawImage(const hyperionnet::RawImageT& raw_image, int bytesPerPixel, ImageResampler& resampler, Image<ColorRgb>& outputImage) {
	
	int width = raw_image.width;
	int height = raw_image.height;

	int lineLength = width * bytesPerPixel;
	PixelFormat pixelFormat = (bytesPerPixel == 4) ? PixelFormat::RGB32 : PixelFormat::RGB24;

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
	int width = nv12_image.width;
	int height = nv12_image.height;

	size_t y_size = nv12_image.data_y.size();
	size_t uv_size = nv12_image.data_uv.size();
	std::vector<uint8_t> combined_buffer(y_size + uv_size);

	std::memcpy(combined_buffer.data(), nv12_image.data_y.data(), y_size);
	std::memcpy(combined_buffer.data() + y_size, nv12_image.data_uv.data(), uv_size);

	// Determine line length (stride_y)
	int lineLength = nv12_image.stride_y > 0 ? nv12_image.stride_y : width;

	PixelFormat pixelFormat = PixelFormat::NV12;

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