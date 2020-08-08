// STL includes
#include <stdexcept>

// project includes
#include <hyperion/MessageForwarder.h>

// hyperion includes
#include <hyperion/Hyperion.h>

// utils includes
#include <utils/Logger.h>

// qt includes
#include <QTcpServer>
#include <QTcpSocket>

#include <flatbufserver/FlatBufferConnection.h>

MessageForwarder::MessageForwarder(Hyperion *hyperion)
	: QObject()
	, _hyperion(hyperion)
	, _log(Logger::getInstance("NETFORWARDER"))
	, _muxer(_hyperion->getMuxerInstance())
	, _forwarder_enabled(true)
	, _priority(140)
{
	// get settings updates
	connect(_hyperion, &Hyperion::settingsChanged, this, &MessageForwarder::handleSettingsUpdate);

	// component changes
	connect(_hyperion, &Hyperion::compStateChangeRequest, this, &MessageForwarder::handleCompStateChangeRequest);

	// connect with Muxer visible priority changes
	connect(_muxer, &PriorityMuxer::visiblePriorityChanged, this, &MessageForwarder::handlePriorityChanges);

	// init
	handleSettingsUpdate(settings::NETFORWARD, _hyperion->getSetting(settings::NETFORWARD));
}

MessageForwarder::~MessageForwarder()
{
	while (!_forwardClients.isEmpty())
		delete _forwardClients.takeFirst();
}

void MessageForwarder::handleSettingsUpdate(settings::type type, const QJsonDocument &config)
{
	if(type == settings::NETFORWARD)
	{
		// clear the current targets
		_jsonSlaves.clear();
		_flatSlaves.clear();
		while (!_forwardClients.isEmpty())
			delete _forwardClients.takeFirst();

		// build new one
		const QJsonObject &obj = config.object();
		if ( !obj["json"].isNull() )
		{
			const QJsonArray & addr = obj["json"].toArray();
			for (const auto& entry : addr)
			{
				addJsonSlave(entry.toString());
			}
		}

		if ( !obj["flat"].isNull() )
		{
			const QJsonArray & addr = obj["flat"].toArray();
			for (const auto& entry : addr)
			{
				addFlatbufferSlave(entry.toString());
			}
		}

		if (!_jsonSlaves.isEmpty() && obj["enable"].toBool() && _forwarder_enabled)
		{
			InfoIf(obj["enable"].toBool(true), _log, "Forward now to json targets '%s'", QSTRING_CSTR(_jsonSlaves.join(", ")));
			connect(_hyperion, &Hyperion::forwardJsonMessage, this, &MessageForwarder::forwardJsonMessage, Qt::UniqueConnection);
		} else if (_jsonSlaves.isEmpty() || ! obj["enable"].toBool() || !_forwarder_enabled)
			disconnect(_hyperion, &Hyperion::forwardJsonMessage, 0, 0);

		if (!_flatSlaves.isEmpty() && obj["enable"].toBool() && _forwarder_enabled)
		{
			InfoIf(obj["enable"].toBool(true), _log, "Forward now to flatbuffer targets '%s'", QSTRING_CSTR(_flatSlaves.join(", ")));
		}
		else if ( _flatSlaves.isEmpty() || ! obj["enable"].toBool() || !_forwarder_enabled)
		{
			disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, 0, 0);
			disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, 0, 0);
		}

		// update comp state
		_hyperion->setNewComponentState(hyperion::COMP_FORWARDER, obj["enable"].toBool(true));
	}
}

void MessageForwarder::handleCompStateChangeRequest(hyperion::Components component, bool enable)
{
	if (component == hyperion::COMP_FORWARDER && _forwarder_enabled != enable)
	{
		_forwarder_enabled = enable;
		handleSettingsUpdate(settings::NETFORWARD, _hyperion->getSetting(settings::NETFORWARD));
		Info(_log, "Forwarder change state to %s", (_forwarder_enabled ? "enabled" : "disabled"));
		_hyperion->setNewComponentState(component, _forwarder_enabled);
	}
}

void MessageForwarder::handlePriorityChanges(quint8 priority)
{
	const QJsonObject obj = _hyperion->getSetting(settings::NETFORWARD).object();
	if (priority != 0 && _forwarder_enabled && obj["enable"].toBool())
	{
		//_flatSlaves.clear();
		//while (!_forwardClients.isEmpty())
		//	delete _forwardClients.takeFirst();

		hyperion::Components activeCompId = _hyperion->getPriorityInfo(priority).componentId;
		if (activeCompId == hyperion::COMP_GRABBER || activeCompId == hyperion::COMP_V4L)
		{
			if ( !obj["flat"].isNull() )
			{
				const QJsonArray & addr = obj["flat"].toArray();
				for (const auto& entry : addr)
				{
					addFlatbufferSlave(entry.toString());
				}
			}

			switch(activeCompId)
			{
				case hyperion::COMP_GRABBER:
				{
					disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, 0, 0);
					connect(_hyperion, &Hyperion::forwardSystemProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
				}
				break;
				case hyperion::COMP_V4L:
				{
					disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, 0, 0);
					connect(_hyperion, &Hyperion::forwardV4lProtoMessage, this, &MessageForwarder::forwardFlatbufferMessage, Qt::UniqueConnection);
				}
				break;
				default:
				{
					disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, 0, 0);
					disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, 0, 0);
				}
			}
		}
		else
		{
			disconnect(_hyperion, &Hyperion::forwardSystemProtoMessage, 0, 0);
			disconnect(_hyperion, &Hyperion::forwardV4lProtoMessage, 0, 0);
		}
	}
}

void MessageForwarder::addJsonSlave(QString slave)
{
	QStringList parts = slave.split(":");
	if (parts.size() != 2)
	{
		Error(_log, "Unable to parse address (%s)",QSTRING_CSTR(slave));
		return;
	}

	bool ok;
	parts[1].toUShort(&ok);
	if (!ok)
	{
		Error(_log, "Unable to parse port number (%s)",QSTRING_CSTR(parts[1]));
		return;
	}

	// verify loop with jsonserver
	const QJsonObject &obj = _hyperion->getSetting(settings::JSONSERVER).object();
	if(QHostAddress(parts[0]) == QHostAddress::LocalHost && parts[1].toInt() == obj["port"].toInt())
	{
		Error(_log, "Loop between JsonServer and Forwarder! (%s)",QSTRING_CSTR(slave));
		return;
	}

	if (_forwarder_enabled && !_jsonSlaves.contains(slave))
		_jsonSlaves << slave;
}

void MessageForwarder::addFlatbufferSlave(QString slave)
{
	QStringList parts = slave.split(":");
	if (parts.size() != 2)
	{
		Error(_log, "Unable to parse address (%s)",QSTRING_CSTR(slave));
		return;
	}

	bool ok;
	parts[1].toUShort(&ok);
	if (!ok)
	{
		Error(_log, "Unable to parse port number (%s)",QSTRING_CSTR(parts[1]));
		return;
	}

	// verify loop with flatbufserver
	const QJsonObject &obj = _hyperion->getSetting(settings::FLATBUFSERVER).object();
	if(QHostAddress(parts[0]) == QHostAddress::LocalHost && parts[1].toInt() == obj["port"].toInt())
	{
		Error(_log, "Loop between Flatbuffer Server and Forwarder! (%s)",QSTRING_CSTR(slave));
		return;
	}

	if (_forwarder_enabled && !_flatSlaves.contains(slave))
	{
		_flatSlaves << slave;
		FlatBufferConnection* flatbuf = new FlatBufferConnection("Forwarder", slave.toLocal8Bit().constData(), _priority, false);
		_forwardClients << flatbuf;
	}
}

void MessageForwarder::forwardJsonMessage(const QJsonObject &message)
{
	if (_forwarder_enabled)
	{
		QTcpSocket client;
		for (int i=0; i<_jsonSlaves.size(); i++)
		{
			QStringList parts = _jsonSlaves.at(i).split(":");
			client.connectToHost(QHostAddress(parts[0]), parts[1].toUShort());
			if ( client.waitForConnected(500) )
			{
				sendJsonMessage(message,&client);
				client.close();
			}
		}
	}
}

void MessageForwarder::forwardFlatbufferMessage(const QString& name, const Image<ColorRgb> &image)
{
	if (_forwarder_enabled)
	{
		for (int i=0; i < _forwardClients.size(); i++)
			_forwardClients.at(i)->setImage(image);
	}
}

void MessageForwarder::sendJsonMessage(const QJsonObject &message, QTcpSocket *socket)
{
	// for hyperion classic compatibility
	QJsonObject jsonMessage = message;
	if (jsonMessage.contains("tan") && jsonMessage["tan"].isNull())
		jsonMessage["tan"] = 100;

	// serialize message
	QJsonDocument writer(jsonMessage);
	QByteArray serializedMessage = writer.toJson(QJsonDocument::Compact) + "\n";

	// write message
	socket->write(serializedMessage);
	if (!socket->waitForBytesWritten())
	{
		Debug(_log, "Error while writing data to host");
		return;
	}

	// read reply data
	QByteArray serializedReply;
	while (!serializedReply.contains('\n'))
	{
		// receive reply
		if (!socket->waitForReadyRead())
		{
			Debug(_log, "Error while writing data from host");
			return;
		}

		serializedReply += socket->readAll();
	}

	// parse reply data
	QJsonParseError error;
	QJsonDocument reply = QJsonDocument::fromJson(serializedReply ,&error);

	if (error.error != QJsonParseError::NoError)
	{
		Error(_log, "Error while parsing reply: invalid json");
		return;
	}
}
