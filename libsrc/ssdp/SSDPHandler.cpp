#include <ssdp/SSDPHandler.h>

#include <webserver/WebServer.h>
#include "SSDPDescription.h"
#include <hyperion/Hyperion.h>
#include <HyperionConfig.h>
#include <hyperion/AuthManager.h>

#include <QNetworkInterface>
#include <QNetworkConfigurationManager>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#include <QRandomGenerator>
#endif

static const QString SSDP_HYPERION_ST("urn:hyperion-project.org:device:basic:1");

SSDPHandler::SSDPHandler(WebServer* webserver, quint16 flatBufPort, quint16 jsonServerPort, const QString& name, QObject * parent)
	: SSDPServer(parent)
	, _webserver(webserver)
	, _localAddress()
	, _NCA(nullptr)
{
	setFlatBufPort(flatBufPort);
	setJsonServerPort(jsonServerPort);
	setHyperionName(name);
}

SSDPHandler::~SSDPHandler()
{
	stopServer();
}

void SSDPHandler::initServer()
{
	_uuid = AuthManager::getInstance()->getID();
	SSDPServer::setUuid(_uuid);

	// announce targets
	_deviceList.push_back("upnp:rootdevice");
	_deviceList.push_back("uuid:"+_uuid);
	_deviceList.push_back(SSDP_HYPERION_ST);

	// prep server
	SSDPServer::initServer();

	_NCA = new QNetworkConfigurationManager(this);

	// listen for mSearchRequestes
	connect(this, &SSDPServer::msearchRequestReceived, this, &SSDPHandler::handleMSearchRequest);

	connect(_NCA, &QNetworkConfigurationManager::configurationChanged, this, &SSDPHandler::handleNetworkConfigurationChanged);

	// get localAddress from interface
	if(!getLocalAddress().isEmpty())
	{
		_localAddress = getLocalAddress();
	}

	// startup if localAddress is found
	bool isInited = false;
	QMetaObject::invokeMethod(_webserver, "isInited", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, isInited));

	if(!_localAddress.isEmpty() && isInited)
	{
		handleWebServerStateChange(true);
	}
}

void SSDPHandler::stopServer()
{
	sendAnnounceList(false);
	SSDPServer::stop();
}

void SSDPHandler::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	const QJsonObject& obj = config.object();

	if(type == settings::FLATBUFSERVER)
	{
		if(obj["port"].toInt() != SSDPServer::getFlatBufPort())
		{
			SSDPServer::setFlatBufPort(obj["port"].toInt());
		}
	}

	if(type == settings::JSONSERVER)
	{
		if(obj["port"].toInt() != SSDPServer::getJsonServerPort())
		{
			SSDPServer::setJsonServerPort(obj["port"].toInt());
		}
	}

	if (type == settings::GENERAL)
	{
		if (obj["name"].toString() != SSDPServer::getHyperionName())
		{
			SSDPServer::setHyperionName(obj["name"].toString());
		}
	}
}

void SSDPHandler::handleWebServerStateChange(bool newState)
{
	if(newState)
	{
		// refresh info
		QMetaObject::invokeMethod(_webserver, "setSSDPDescription", Qt::BlockingQueuedConnection, Q_ARG(QString, buildDesc()));
		setDescriptionAddress(getDescAddress());
		if(start())
			sendAnnounceList(true);
	}
	else
	{
		QMetaObject::invokeMethod(_webserver, "setSSDPDescription", Qt::BlockingQueuedConnection, Q_ARG(QString, ""));
		sendAnnounceList(false);
		stop();
	}
}

void SSDPHandler::handleNetworkConfigurationChanged(const QNetworkConfiguration &config)
{
	// get localAddress from interface
	QString localAddress = getLocalAddress();
	if(!localAddress.isEmpty() && _localAddress != localAddress)
	{
		// revoke old ip
		sendAnnounceList(false);

		// update desc & notify new ip
		_localAddress = localAddress;
		QMetaObject::invokeMethod(_webserver, "setSSDPDescription", Qt::BlockingQueuedConnection, Q_ARG(QString, buildDesc()));
		setDescriptionAddress(getDescAddress());
		sendAnnounceList(true);
	}
}

QString SSDPHandler::getLocalAddress() const
{
	// get the first valid IPv4 address. This is probably not that one we actually want to announce
	for(const auto & address : QNetworkInterface::allAddresses())
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
	const auto respond = [=] () {
		// when searched for all devices / root devices / basic device
		if(target == "ssdp:all")
			sendMSearchResponse(SSDP_HYPERION_ST, address, port);
		else if(target == "upnp:rootdevice" || target == "urn:schemas-upnp-org:device:basic:1" || target == SSDP_HYPERION_ST)
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

QString SSDPHandler::getDescAddress() const
{
	return getBaseAddress()+"description.xml";
}

QString SSDPHandler::getBaseAddress() const
{
	quint16 port = 0;
	QMetaObject::invokeMethod(_webserver, "getPort", Qt::BlockingQueuedConnection, Q_RETURN_ARG(quint16, port));
	return QString("http://%1:%2/").arg(_localAddress).arg(port);
}

QString SSDPHandler::buildDesc() const
{
	/// %1 base url                   http://192.168.0.177:80/
	/// %2 friendly name              Hyperion 2.0.0 (192.168.0.177)
	/// %3 modelNumber                2.0.0
	/// %4 serialNumber / UDN (H ID)  Fjsa723dD0....
	return SSDP_DESCRIPTION.arg(getBaseAddress(), QString("Hyperion (%1)").arg(_localAddress), QString(HYPERION_VERSION), _uuid);
}

void SSDPHandler::sendAnnounceList(bool alive)
{
	for(const auto & entry : _deviceList){
		alive ? SSDPServer::sendAlive(entry) : SSDPServer::sendByeBye(entry);
	}
}

