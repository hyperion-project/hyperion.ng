#include <ssdp/SSDPHandler.h>

#include "SSDPDescription.h"
#include <hyperion/Hyperion.h>
#include <HyperionConfig.h>
#include <db/MetaTable.h>

#include <QNetworkInterface>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#include <QRandomGenerator>
#endif

const char HTTP_SERVICE_TYPE[] = "http";
const char HTTPS_SERVICE_TYPE[] = "https";
const char JSONAPI_SERVICE_TYPE[] = "jsonapi";
const char FLATBUFFER_SERVICE_TYPE[] = "flatbuffer";
const char PROTOBUFFER_SERVICE_TYPE[] = "protobuffer";

static const QString SSDP_IDENTIFIER("urn:hyperion-project.org:device:basic:1");

SSDPHandler::SSDPHandler(QObject* parent)
	: SSDPServer(parent)
	, _httpPort(8090)
	, _httpsPort(8092)
	, _jsonApiPort(19444)
	, _flatBufferPort(19400)
	, _protoBufferPort(19445)
{
	Debug(_log, "SSDP info service created");
}

SSDPHandler::~SSDPHandler()
{
}

void SSDPHandler::initServer()
{
	MetaTable metaTable;
	setUuid(metaTable.getUUID());
	// announce targets
	_deviceList.push_back("upnp:rootdevice");
	_deviceList.push_back("uuid:" + getUuid());
	_deviceList.push_back(SSDP_IDENTIFIER);

	// prep server
	SSDPServer::initServer();

	// listen for mSearchRequestes
	connect(this, &SSDPServer::msearchRequestReceived, this, &SSDPHandler::handleMSearchRequest);

	// get localAddress from interface
	if (!getLocalAddress().isEmpty())
	{
		_localAddress = getLocalAddress();
	}
}

void SSDPHandler::stop()
{
	sendAnnounceList(false);
	SSDPServer::stop();
	Info(_log, "SSDP info service stopped");
}

void SSDPHandler::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	bool isSsdpInfoUpdated {false};
	const QJsonObject& obj = config.object();

	switch (type) {
	case settings::FLATBUFSERVER:
	{
		int port = (obj["port"].toInt());
		if ( port != _flatBufferPort)
		{
			_flatBufferPort = port;
			isSsdpInfoUpdated = true;
		}
	}
	break;
	case settings::PROTOSERVER:
	{
		int port = (obj["port"].toInt());
		if ( port != _protoBufferPort)
		{
			_protoBufferPort = port;
			isSsdpInfoUpdated = true;
		}
	}
	break;
	case settings::JSONSERVER:
	{
		int port = (obj["port"].toInt());
		if ( port != _jsonApiPort)
		{
			_jsonApiPort = port;
			isSsdpInfoUpdated = true;
		}
	}
	break;
	case settings::WEBSERVER:
	{
		int httpPort = (obj["port"].toInt());
		int httpsPort = (obj["sslPort"].toInt());

		if ( httpPort != _httpPort || httpsPort != _httpPort)
		{
			_httpPort = httpPort;
			setDescriptionAddress(getDescriptionAddress());

			_httpsPort = httpsPort;
			isSsdpInfoUpdated = true;
		}
	}
	break;
	case settings::GENERAL:
	{
		QString name = (obj["name"].toString());
		if ( name != _name)
		{
			_name = name;
			isSsdpInfoUpdated = true;
		}
	}
	break;
	default:
	break;
	}

	if (isSsdpInfoUpdated)
	{
		emit descriptionUpdated(getDescription());
	}
}

void SSDPHandler::onPortChanged(const QString& serviceType, quint16 servicePort)
{
	if (serviceType == HTTP_SERVICE_TYPE)
	{
		_httpPort = servicePort;
	}
	else if (serviceType == HTTPS_SERVICE_TYPE)
	{
		_httpsPort = servicePort;
	}
	else if (serviceType == JSONAPI_SERVICE_TYPE)
	{
		_jsonApiPort = servicePort;
	}
	else if (serviceType == FLATBUFFER_SERVICE_TYPE)
	{
		_flatBufferPort = servicePort;
	}
	else if (serviceType == PROTOBUFFER_SERVICE_TYPE)
	{
		_protoBufferPort = servicePort;
	}
	emit descriptionUpdated(getDescription());
}

void SSDPHandler::onStateChange(bool state)
{
	if (state)
	{
		if (start())
		{
			sendAnnounceList(true);
		}
	}
	else
	{
		stop();
	}
}

QString SSDPHandler::getLocalAddress() const
{
	// get the first valid IPv4 address. This is probably not that one we actually want to announce
	for (const auto& address : QNetworkInterface::allAddresses())
	{
		// is valid when, no loopback, IPv4
		if (!address.isLoopback() && address.protocol() == QAbstractSocket::IPv4Protocol)
		{
			return address.toString();
		}
	}
	return QString();
}

void SSDPHandler::handleMSearchRequest(const QString& target, const QString& mx, const QString address, quint16 port)
{
	const auto respond = [=]() {
		// when searched for all devices / root devices / basic device
		if (target == "ssdp:all")
			sendMSearchResponse(SSDP_IDENTIFIER, address, port);
		else if (target == "upnp:rootdevice" || target == "urn:schemas-upnp-org:device:basic:1" || target == SSDP_IDENTIFIER)
			sendMSearchResponse(target, address, port);
	};

	bool ok = false;
	int maxDelay = mx.toInt(&ok);
	if (ok)
	{
		/* Pick a random delay between 0 and MX seconds */
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
		int randomDelay = QRandomGenerator::global()->generate() % (maxDelay * 1000);
#else
		int randomDelay = qrand() % (maxDelay * 1000);
#endif
		QTimer::singleShot(randomDelay, respond);
	}
	else
	{
		/* MX Header is not valid.
		 * Send response without delay */
		respond();
	}
}

QString SSDPHandler::getDescriptionAddress() const
{
	return getBaseAddress() + "description.xml";
}

QString SSDPHandler::getBaseAddress() const
{
	return QString("http://%1:%2/").arg(_localAddress).arg(_httpPort);
}

QString SSDPHandler::getDescription() const
{
	/// %1 base url                   http://192.168.0.177:8090/
	/// %2 friendly name              Hyperion (192.168.0.177)
	/// %3 modelNumber                2.0.0
	/// %4 serialNumber / UDN (H ID)  Fjsa723dD0....
	/// %5 json port                  19444
	/// %6 ssl server port            8092
	/// %7 protobuf port              19445
	/// %8 flatbuf port               19400

	return SSDP_DESCRIPTION.arg(
		getBaseAddress(),
		QString("Hyperion (%1)").arg(_localAddress),
		QString(HYPERION_VERSION),
		getUuid(),
		QString::number(_jsonApiPort),
		QString::number(_httpsPort),
		QString::number(_protoBufferPort),
		QString::number(_flatBufferPort)
	);
}

QString SSDPHandler::getInfo() const
{
	QString hyperionServicesInfo;
	hyperionServicesInfo.append(QString("HYPERION-NAME: %1\r\n").arg(_name));
	hyperionServicesInfo.append(QString("HYPERION-JSS-PORT: %1\r\n").arg(_jsonApiPort));
#if defined(ENABLE_FLATBUF_SERVER)
	hyperionServicesInfo.append(QString("HYPERION-FBS-PORT: %1\r\n").arg(_flatBufferPort));
#endif
#if defined(ENABLE_PROTOBUF_SERVER)
	hyperionServicesInfo.append(QString("HYPERION-PBS-PORT: %1\r\n").arg(_protoBufferPort));
#endif
	return hyperionServicesInfo;
}

void SSDPHandler::sendAnnounceList(bool alive)
{
	for (const auto& entry : _deviceList) {
		alive ? SSDPServer::sendAlive(entry) : SSDPServer::sendByeBye(entry);
	}
}
