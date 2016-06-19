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

// hyperion util includes
#include "hyperion/ImageProcessorFactory.h"
#include "hyperion/ImageProcessor.h"
#include "utils/ColorRgb.h"
#include "HyperionConfig.h"

// project includes
#include "UDPClientConnection.h"

UDPClientConnection::UDPClientConnection(QByteArray * datagram , const int priority, Hyperion * hyperion) :
	QObject(),
	_locale(QLocale::C),
	_datagram(datagram),
	_imageProcessor(ImageProcessorFactory::getInstance().newImageProcessor()),
	_hyperion(hyperion),
	_receiveBuffer(),
	_priority(priority),
	_ledColors(hyperion->getLedCount(), ColorRgb::BLACK)
{
	// initalize the locale. Start with the default C-locale
	_locale.setNumberOptions(QLocale::OmitGroupSeparator | QLocale::RejectGroupSeparator);

	// connect internal signals and slots
//	connect(_socket, SIGNAL(disconnected()), this, SLOT(socketClosed()));
//	connect(_socket, SIGNAL(readyRead()), this, SLOT(readData()));
}

UDPClientConnection::~UDPClientConnection()
{
	if (_priority < 255)
	{
		// clear the current channel
		_hyperion->clear(_priority);
		_priority = 255;
	}

	delete _datagram;
}

void UDPClientConnection::readData()
{
//	_receiveBuffer += _socket->readAll();
	_receiveBuffer = *_datagram;

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
			std::cout << "UDPLISTENER INFO: server drops messages (buffer full)" << std::endl;
			_receiveBuffer.clear();
		}

		// try too look up '\n' again
		bytes = _receiveBuffer.indexOf('\n') + 1;
	}
}

void UDPClientConnection::socketClosed()
{
	if (_priority < 255)
	{
		// clear the current channel
		_hyperion->clear(_priority);
		_priority = 255;
	}

	emit connectionClosed(this);
}

void UDPClientConnection::handleMessage(const QString & message)
{
	//std::cout << "udp message: " << message.toStdString() << std::endl;

	QStringList messageParts = message.split(" ", QString::SkipEmptyParts);

	if (messageParts.size() > 0)
	{
		if (messageParts[0] == "set" && messageParts.size() > 2)
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

							// send current color values to hyperion if this is the last led assuming leds values are send in order of id
							if ((ledIndex == _ledColors.size() -1) && _priority < 255)
							{
								_hyperion->setColors(_priority, _ledColors, -1);
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
					if (_priority < 255)
					{
						// clear the current channel
						_hyperion->clear(_priority);
					}

					_priority = prio;
					return;
				}
			}
		}
		else if (messageParts[0] == "sync")
		{
			// send current color values to hyperion
			if (_priority < 255)
			{
				_hyperion->setColors(_priority, _ledColors, -1);
			}
			return;
		}
	}

	std::cout << "UDPLISTENER INFO: unknown message: " << message.toStdString() << std::endl;
}

void UDPClientConnection::sendMessage(const std::string & message)
{
	//std::cout << "send udp message: " << message;
//	_socket->write(message.c_str(), message.size());
}

void UDPClientConnection::sendMessage(const char * message, int size)
{
	//std::cout << "send udp message: " << std::string(message, size);
//	_socket->write(message, size);
}

