#include "webserver/WebServer.h"
#include "StaticFileServing.h"
#include "QtHttpServer.h"

#include <QFileInfo>
#include <QJsonObject>

// bonjour
#include <bonjour/bonjourserviceregister.h>

// netUtil
#include <utils/NetUtils.h>


WebServer::WebServer(const QJsonDocument& config, QObject * parent)
	:  QObject(parent)
	, _config(config)
	, _log(Logger::getInstance("WEBSERVER"))
	, _server()
{

}

WebServer::~WebServer()
{
	stop();
}

void WebServer::initServer()
{
	_server = new QtHttpServer (this);
	_server->setServerName (QStringLiteral ("Hyperion Webserver"));

	connect (_server, &QtHttpServer::started, this, &WebServer::onServerStarted);
	connect (_server, &QtHttpServer::stopped, this, &WebServer::onServerStopped);
	connect (_server, &QtHttpServer::error,   this, &WebServer::onServerError);

	// create StaticFileServing
	_staticFileServing = new StaticFileServing (this);
	connect(_server, &QtHttpServer::requestNeedsReply, _staticFileServing, &StaticFileServing::onRequestNeedsReply);

	// init
	handleSettingsUpdate(settings::WEBSERVER, _config);
}

void WebServer::onServerStarted (quint16 port)
{
	_inited= true;

	Info(_log, "Started on port %d name '%s'", port ,_server->getServerName().toStdString().c_str());

	if(_serviceRegister == nullptr)
	{
		_serviceRegister = new BonjourServiceRegister(this);
		_serviceRegister->registerService("_hyperiond-http._tcp", port);
	}
	else if( _serviceRegister->getPort() != port)
	{
		delete _serviceRegister;
		_serviceRegister = new BonjourServiceRegister(this);
		_serviceRegister->registerService("_hyperiond-http._tcp", port);
	}
	emit stateChange(true);
}

void WebServer::onServerStopped () {
	Info(_log, "Stopped %s", _server->getServerName().toStdString().c_str());
	emit stateChange(false);
}

void WebServer::onServerError (QString msg)
{
	Error(_log, "%s", msg.toStdString().c_str());
}

void WebServer::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	if(type == settings::WEBSERVER)
	{
		const QJsonObject& obj = config.object();

		_baseUrl = obj["document_root"].toString(WEBSERVER_DEFAULT_PATH);


		if ( (_baseUrl != ":/webconfig") && !_baseUrl.trimmed().isEmpty())
		{
			QFileInfo info(_baseUrl);
			if (!info.exists() || !info.isDir())
			{
				Error(_log, "document_root '%s' is invalid", _baseUrl.toUtf8().constData());
				_baseUrl = WEBSERVER_DEFAULT_PATH;
			}
		}
		else
			_baseUrl = WEBSERVER_DEFAULT_PATH;

		Debug(_log, "Set document root to: %s", _baseUrl.toUtf8().constData());
		_staticFileServing->setBaseUrl(_baseUrl);

		if(_port != obj["port"].toInt(WEBSERVER_DEFAULT_PORT))
		{
			_port = obj["port"].toInt(WEBSERVER_DEFAULT_PORT);
			stop();
		}

		// eval if the port is available, will be incremented if not
		if(!_server->isListening())
			NetUtils::portAvailable(_port, _log);

		start();
		emit portChanged(_port);
	}
}

void WebServer::start()
{
	_server->start(_port);
}

void WebServer::stop()
{
	_server->stop();
}

void WebServer::setSSDPDescription(const QString & desc)
{
	_staticFileServing->setSSDPDescription(desc);
}
