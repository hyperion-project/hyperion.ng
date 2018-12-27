#include "webserver/WebServer.h"
#include "StaticFileServing.h"
#include "QtHttpServer.h"

// bonjour
#include <bonjour/bonjourserviceregister.h>
#include <bonjour/bonjourrecord.h>

#include <QFileInfo>

WebServer::WebServer(const QJsonDocument& config, QObject * parent)
	:  QObject(parent)
	, _log(Logger::getInstance("WEBSERVER"))
	, _hyperion(Hyperion::getInstance())
	, _server(new QtHttpServer (this))
{
	_server->setServerName (QStringLiteral ("Hyperion Webserver"));

	connect (_server, &QtHttpServer::started, this, &WebServer::onServerStarted);
	connect (_server, &QtHttpServer::stopped, this, &WebServer::onServerStopped);
	connect (_server, &QtHttpServer::error,   this, &WebServer::onServerError);

	// create StaticFileServing
	_staticFileServing = new StaticFileServing (_hyperion, this);
	connect(_server, &QtHttpServer::requestNeedsReply, _staticFileServing, &StaticFileServing::onRequestNeedsReply);

	Debug(_log, "Instance created");
	// init
	handleSettingsUpdate(settings::WEBSERVER, config);
}

WebServer::~WebServer()
{
	stop();
}

void WebServer::onServerStarted (quint16 port)
{
	Info(_log, "Started on port %d name '%s'", port ,_server->getServerName().toStdString().c_str());

	BonjourServiceRegister *bonjourRegister_http = new BonjourServiceRegister();
	bonjourRegister_http->registerService("_hyperiond-http._tcp", port);
}

void WebServer::onServerStopped () {
	Info(_log, "Stopped %s", _server->getServerName().toStdString().c_str());
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

		bool webconfigEnable = obj["enable"].toBool(true);
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
		if ( webconfigEnable )
		{
			start();
		}
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
