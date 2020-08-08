// system includes
#include <stdexcept>
#include <cassert>
#include <iomanip>
#include <cstdio>

// stl includes
#include <iostream>
#include <sstream>
#include <iterator>

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
	_receiveBuffer += _socket->readAll();

	int bytes = _receiveBuffer.indexOf('\n') + 1;
	while(bytes > 0)
	{
		// create message string (strip the newline)
		QString message = QString::fromLatin1(_receiveBuffer.data(), bytes-1);
		// remove message data from buffer
		_receiveBuffer = _receiveBuffer.mid(bytes);

		// handle trimmed message
		handleMessage(message.trimmed());

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
	QStringList messageParts = QStringUtils::split(message," ",QStringUtils::SplitBehavior::SkipEmptyParts);
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
				unsigned ledIndex = messageParts[2].toUInt(&rc);
				if (rc && ledIndex < _ledColors.size())
				{
					if (messageParts[3] == "rgb" && messageParts.size() == 7)
					{
						// replace decimal comma with decimal point
						messageParts[4].replace(',', '.');
						messageParts[5].replace(',', '.');
						messageParts[6].replace(',', '.');

						bool rc1, rc2, rc3;
						uint8_t red = qMax(0, qMin(255, int(255 * messageParts[4].toFloat(&rc1))));

						// check for correct locale should not be needed anymore - please check!
						if (!rc1)
						{
							// maybe a locale issue. switch to a locale with a comma instead of a dot as decimal seperator (or vice versa)
							_locale = QLocale((_locale.decimalPoint() == QChar('.')) ? QLocale::Dutch : QLocale::C);
							_locale.setNumberOptions(QLocale::OmitGroupSeparator | QLocale::RejectGroupSeparator);

							// try again
							red = qMax(0, qMin(255, int(255 * messageParts[4].toFloat(&rc1))));
						}

						uint8_t green = qMax(0, qMin(255, int(255 * messageParts[5].toFloat(&rc2))));
						uint8_t blue  = qMax(0, qMin(255, int(255 * messageParts[6].toFloat(&rc3))));

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
				int prio = messageParts[2].toInt(&rc);
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
