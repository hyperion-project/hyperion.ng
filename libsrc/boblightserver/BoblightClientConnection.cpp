// system includes
#include <stdexcept>
#include <cassert>
#include <iomanip>
#include <cstdio>
#include <cmath>

// stl includes
#include <iostream>
#include <sstream>
#include <iterator>
#include <locale>

// Qt includes
#include <QResource>
#include <QDateTime>
#include <QHostInfo>

// hyperion util includes
#include <hyperion/ImageProcessor.h>
#include "HyperionConfig.h"
#include <hyperion/Hyperion.h>
#include <utils/QStringUtils.h>

// project includes
#include "BoblightClientConnection.h"

BoblightClientConnection::BoblightClientConnection(Hyperion* hyperion, QTcpSocket *socket, int priority)
	: QObject()
	, _locale(QLocale::C)
	, _socket(socket)
	, _imageProcessor(hyperion->getImageProcessor())
	, _hyperion(hyperion)
	, _receiveBuffer()
	, _priority(priority)
	, _ledColors(hyperion->getLedCount(), ColorRgb::BLACK)
	, _log(Logger::getInstance("BOBLIGHT"))
	, _clientAddress(QHostInfo::fromName(socket->peerAddress().toString()).hostName())
{
	// initalize the locale. Start with the default C-locale
	_locale.setNumberOptions(QLocale::OmitGroupSeparator | QLocale::RejectGroupSeparator);

	// connect internal signals and slots
	connect(_socket, &QTcpSocket::disconnected, this, &BoblightClientConnection::socketClosed);
	connect(_socket, &QTcpSocket::readyRead, this, &BoblightClientConnection::readData);
}

BoblightClientConnection::~BoblightClientConnection()
{
	 // clear the current channel
	if (_priority != 0 && _priority >= 128 && _priority < 254)
		_hyperion->clear(_priority);

	delete _socket;
}

void BoblightClientConnection::readData()
{
	_receiveBuffer.append(_socket->readAll());

	int bytes = _receiveBuffer.indexOf('\n') + 1;
	while(bytes > 0)
	{
		// create message string (strip the newline)
		const QString message = readMessage(_receiveBuffer.data(), bytes);

		// handle trimmed message
		handleMessage(message);

		// remove message data from buffer
		_receiveBuffer.remove(0, bytes);

		// drop messages if the buffer is too full
		if (_receiveBuffer.size() > 100*1024)
		{
			Debug(_log, "server drops messages (buffer full)");
			_receiveBuffer.clear();
		}

		// try too look up '\n' again
		bytes = _receiveBuffer.indexOf('\n') + 1;
	}
}

QString BoblightClientConnection::readMessage(const char *data, const size_t size) const
{
	char *end = (char *)data + size - 1;

	// Trim left
	while (data < end && std::isspace(*data))
	{
		++data;
	}

	// Trim right
	while (end > data && std::isspace(*end))
	{
		--end;
	}

	// create message string (strip the newline)
	const int len = end - data + 1;
	const QString message =  QString::fromLatin1(data, len);

	//std::cout << bytes << ": \"" << message.toUtf8().constData() << "\"" << std::endl;

	return message;
}

void BoblightClientConnection::socketClosed()
{
	 // clear the current channel
	if (_priority >= 128 && _priority < 254)
		_hyperion->clear(_priority);

	emit connectionClosed(this);
}


void BoblightClientConnection::handleMessage(const QString & message)
{
	//std::cout << "boblight message: " << message.toStdString() << std::endl;
	const QVector<QStringRef> messageParts = QStringUtils::splitRef(message, ' ', QStringUtils::SplitBehavior::SkipEmptyParts);
	if (messageParts.size() > 0)
	{
		if (messageParts[0] == "hello")
		{
			sendMessage("hello\n");
			return;
		}
		else if (messageParts[0] == "ping")
		{
			sendMessage("ping 1\n");
			return;
		}
		else if (messageParts[0] == "get" && messageParts.size() > 1)
		{
			if (messageParts[1] == "version")
			{
				sendMessage("version 5\n");
				return;
			}
			else if (messageParts[1] == "lights")
			{
				sendLightMessage();
				return;
			}
		}
		else if (messageParts[0] == "set" && messageParts.size() > 2)
		{
			if (messageParts.size() > 3 && messageParts[1] == "light")
			{
				bool rc;
				const unsigned ledIndex = parseUInt(messageParts[2], &rc);
				if (rc && ledIndex < _ledColors.size())
				{
					if (messageParts[3] == "rgb" && messageParts.size() == 7)
					{
						// custom parseByte accepts both ',' and '.' as decimal separator
						// no need to replace decimal comma with decimal point

						bool rc1, rc2, rc3;
						const uint8_t red = parseByte(messageParts[4], &rc1);
						const uint8_t green = parseByte(messageParts[5], &rc2);
						const uint8_t blue = parseByte(messageParts[6], &rc3);

						if (rc1 && rc2 && rc3)
						{
							ColorRgb & rgb =  _ledColors[ledIndex];
							rgb.red = red;
							rgb.green = green;
							rgb.blue = blue;

							if (_priority == 0 || _priority < 128 || _priority >= 254)
								return;

							// send current color values to hyperion if this is the last led assuming leds values are send in order of id
							if (ledIndex == _ledColors.size() -1)
							{
								_hyperion->setInput(_priority, _ledColors);
							}

							return;
						}
					}
					else if(messageParts[3] == "speed" ||
						      messageParts[3] == "interpolation" ||
						      messageParts[3] == "use" ||
						      messageParts[3] == "singlechange")
					{
						// these message are ignored by Hyperion
						return;
					}
				}
			}
			else if (messageParts.size() == 3 && messageParts[1] == "priority")
			{
				bool rc;
				const int prio = static_cast<int>(parseUInt(messageParts[2], &rc));
				if (rc && prio != _priority)
				{
					if (_priority != 0 && _hyperion->getPriorityInfo(_priority).componentId == hyperion::COMP_BOBLIGHTSERVER)
						_hyperion->clear(_priority);

					if (prio < 128 || prio >= 254)
					{
						_priority = 128;
						while (_hyperion->getActivePriorities().contains(_priority))
						{
							_priority += 1;
						}

						// warn against invalid priority
						Warning(_log, "The priority %i is not in the priority range between 128 and 253. Priority %i is used instead.", prio, _priority);
						// register new priority (previously modified)
						_hyperion->registerInput(_priority, hyperion::COMP_BOBLIGHTSERVER, QString("Boblight@%1").arg(_socket->peerAddress().toString()));
					}
					else
					{
						// register new priority
						_hyperion->registerInput(prio, hyperion::COMP_BOBLIGHTSERVER, QString("Boblight@%1").arg(_socket->peerAddress().toString()));
						_priority = prio;
					}

					return;
				}
			}
		}
		else if (messageParts[0] == "sync")
		{
			if ( _priority >= 128 && _priority < 254)
				_hyperion->setInput(_priority, _ledColors); // send current color values to hyperion

			return;
		}
	}

	Debug(_log, "unknown boblight message: %s", QSTRING_CSTR(message));
}

/// Float values 10 to the power of -p for p in 0 .. 8.
const float ipows[] = {
	1,
	1.0f / 10.0f,
	1.0f / 100.0f,
	1.0f / 1000.0f,
	1.0f / 10000.0f,
	1.0f / 100000.0f,
	1.0f / 1000000.0f,
	1.0f / 10000000.0f,
	1.0f / 100000000.0f};

float BoblightClientConnection::parseFloat(const QStringRef& s, bool *ok) const
{
	// We parse radix 10
	const char MIN_DIGIT = '0';
	const char MAX_DIGIT = '9';
	const char SEP_POINT = '.';
	const char SEP_COMMA = ',';
	const int NUM_POWS = 9;

	/// The maximum number of characters we want to process
	const int MAX_LEN = 18; // Chosen randomly

	/// The index of the current character
	int q = 0;

	/// The integer part of the number
	int64_t n = 0;

	auto it = s.begin();

#define STEP ((it != s.end()) && (q++ < MAX_LEN))

	// parse the integer-part
	while (it->unicode() >= MIN_DIGIT && it->unicode() <= MAX_DIGIT && STEP)
	{
		n = (n * 10) + (it->unicode() - MIN_DIGIT);
		++it;
	}

	/// The resulting float value
	float f = static_cast<float>(n);

	// parse decimal part
	if ((it->unicode() == SEP_POINT || it->unicode() == SEP_COMMA) && STEP)
	{
		/// The decimal part of the number
		int64_t d = 0;

		/// The exponent for the scale-factor 10 to the power -e
		int e = 0;

		++it;
		while (it->unicode() >= MIN_DIGIT && it->unicode() <= MAX_DIGIT && STEP)
		{
			d = (d * 10) + (it->unicode() - MIN_DIGIT);
			++e;
			++it;
		}

		const float h = static_cast<float>(d);

		// We want to use pre-calculated power whenever possible
		if (e < NUM_POWS)
		{
			f += h * ipows[e];
		}
		else
		{
			f += h / std::pow(10.0f, e);
		}
	}

	if (q >= MAX_LEN || q < s.length())
	{
		if (ok)
		{
			//std::cout << "FAIL L " << q << ": " << s.toUtf8().constData() << std::endl;
			*ok = false;
		}
		return 0;
	}

	if (ok)
	{
		//std::cout << "OK " << d << ": " << s.toUtf8().constData() << std::endl;
		*ok = true;
	}

	return f;
}

unsigned BoblightClientConnection::parseUInt(const QStringRef& s, bool *ok) const
{
	// We parse radix 10
	const char MIN_DIGIT = '0';
	const char MAX_DIGIT = '9';

	/// The maximum number of characters we want to process
	const int MAX_LEN = 10;

	/// The index of the current character
	int q = 0;

	/// The integer part of the number
	int n = 0;

	auto it = s.begin();

	// parse the integer-part
	while (it->unicode() >= MIN_DIGIT && it->unicode() <= MAX_DIGIT && ((it != s.end()) && (q++ < MAX_LEN)))
	{
		n = (n * 10) + (it->unicode() - MIN_DIGIT);
		++it;
	}

	if (ok)
	{
		*ok = !(q >= MAX_LEN || q < s.length());
	}

	return n;
}

uint8_t BoblightClientConnection::parseByte(const QStringRef& s, bool *ok) const
{
	const int LO = 0;
	const int HI = 255;

#if defined(FAST_FLOAT_PARSE)
	const float d = parseFloat(s, ok);
#else
	const float d = s.toFloat(ok);
#endif

	// Clamp to byte range 0 to 255
	return static_cast<uint8_t>(qBound(LO, int(HI * d), HI)); // qBound args are in order min, value, max; see: https://doc.qt.io/qt-5/qtglobal.html#qBound
}

void BoblightClientConnection::sendLightMessage()
{
	char buffer[256];

	int n = snprintf(buffer, sizeof(buffer), "lights %d\n", _hyperion->getLedCount());
	sendMessage(QByteArray(buffer, n));

	double h0, h1, v0, v1;
	for (unsigned i = 0; i < _hyperion->getLedCount(); ++i)
	{
		_imageProcessor->getScanParameters(i, h0, h1, v0, v1);
		n = snprintf(buffer, sizeof(buffer), "light %03d scan %f %f %f %f\n", i, 100*v0, 100*v1, 100*h0, 100*h1);
		sendMessage(QByteArray(buffer, n));
	}
}
