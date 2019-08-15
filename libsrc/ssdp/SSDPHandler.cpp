#include <ssdp/SSDPHandler.h>

#include <webserver/WebServer.h>
#include "SSDPDescription.h"
#include <hyperion/Hyperion.h>
#include <HyperionConfig.h>
#include <hyperion/AuthManager.h>

#include <QNetworkInterface>
#include <QNetworkConfigurationManager>

SSDPHandler::SSDPHandler(WebServer* webserver, const quint16& flatBufPort, const quint16& jsonServerPort, QObject * parent)
	: SSDPServer(parent)
	, _webserver(webserver)
	, _localAddress()
	, _NCA(nullptr)
{
	setFlatBufPort(flatBufPort);
	setJsonServerPort(jsonServerPort);
}

SSDPHandler::~SSDPHandler()
{
	sendAnnounceList(false);
}

void SSDPHandler::initServer()
{
	_uuid = AuthManager::getInstance()->getID();
	SSDPServer::setUuid(_uuid);

	// announce targets
	_deviceList.push_back("upnp:rootdevice");
	_deviceList.push_back("uuid:"+_uuid);
	_deviceList.push_back("urn:hyperion-project.org:device:basic:1");

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
	if(!_localAddress.isEmpty() && _webserver->isInited())
	{
		handleWebServerStateChange(true);
	}
}

void SSDPHandler::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	if(type == settings::FLATBUFSERVER)
	{
		const QJsonObject& obj = config.object();
		if(obj["port"].toInt() != SSDPServer::getFlatBufPort())
		{
			SSDPServer::setFlatBufPort(obj["port"].toInt());
		}
	}

	if(type == settings::JSONSERVER)
	{
		const QJsonObject& obj = config.object();
		if(obj["port"].toInt() != SSDPServer::getJsonServerPort())
		{
			SSDPServer::setJsonServerPort(obj["port"].toInt());
		}
	}
}

void SSDPHandler::handleWebServerStateChange(const bool newState)
{
	if(newState)
	{
		// refresh info
		_webserver->setSSDPDescription(buildDesc());
		setDescriptionAddress(getDescAddress());
		if(start())
			sendAnnounceList(true);
	}
	else
	{
		_webserver->setSSDPDescription("");
		sendAnnounceList(false);
		stop();
	}
}

void SSDPHandler::handleNetworkConfigurationChanged(const QNetworkConfiguration &config)
{
	// get localAddress from interface
	if(!getLocalAddress().isEmpty())
	{
		QString localAddress = getLocalAddress();
		if(_localAddress != localAddress)
		{
			// revoke old ip
			sendAnnounceList(false);

			// update desc & notify new ip
			_localAddress = localAddress;
			_webserver->setSSDPDescription(buildDesc());
			setDescriptionAddress(getDescAddress());
			sendAnnounceList(true);
		}
	}
}

const QString SSDPHandler::getLocalAddress()
{
	// get the first valid IPv4 address. This is probably not that one we actually want to announce
	for( const auto & address : QNetworkInterface::allAddresses())
	{
		// is valid when, no loopback, IPv4
		if (!address.isLoopback() && address.protocol() == QAbstractSocket::IPv4Protocol )
		{
			return address.toString();
		}
	}
	return QString();
}

void SSDPHandler::handleMSearchRequest(const QString& target, const QString& mx, const QString address, const quint16 & port)
{
	// TODO Response delay according to MX field (sec) random between 0 and MX

	// when searched for all devices / root devices / basic device
	if(target == "ssdp:all" || target == "upnp:rootdevice" || target == "urn:schemas-upnp-org:device:basic:1" || target == "urn:hyperion-project.org:device:basic:1")
		sendMSearchResponse(target, address, port);
}

const QString SSDPHandler::getDescAddress()
{
	return getBaseAddress()+"description.xml";
}

const QString SSDPHandler::getBaseAddress()
{
	return "http://"+_localAddress+":"+QString::number(_webserver->getPort())+"/";
}

const QString SSDPHandler::buildDesc()
{
	/// %1 base url                   http://192.168.0.177:80/
	/// %2 friendly name              Hyperion 2.0.0 (192.168.0.177)
	/// %3 modelNumber                2.0.0
	/// %4 serialNumber / UDN (H ID)  Fjsa723dD0....
	return SSDP_DESCRIPTION.arg(getBaseAddress(), QString("Hyperion (%2)").arg(_localAddress), QString(HYPERION_VERSION), _uuid);
}

void SSDPHandler::sendAnnounceList(const bool alive){
	for(const auto & entry : _deviceList){
		alive ? SSDPServer::sendAlive(entry) : SSDPServer::sendByeBye(entry);
	}
}
