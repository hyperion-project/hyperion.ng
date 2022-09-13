#include "webserver/WebServer.h"
#include "HyperionConfig.h"
#include "StaticFileServing.h"
#include "QtHttpServer.h"

#include <QFileInfo>
#include <QJsonObject>

// netUtil
#include <utils/NetUtils.h>

// Constants
namespace {

	const char HTTP_SERVICE_TYPE[] = "http";
	const char HTTPS_SERVICE_TYPE[] = "https";
	const char HYPERION_SERVICENAME[] = "Hyperion";

} //End of constants

WebServer::WebServer(const QJsonDocument& config, bool useSsl, QObject* parent)
	: QObject(parent)
	, _config(config)
	, _useSsl(useSsl)
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
	Debug(_log, "Initialize %s-Webserver", _useSsl ? "https" : "http");
	_server = new QtHttpServer(this);
	_server->setServerName(QStringLiteral("Hyperion %1-Webserver").arg(_useSsl ? "https" : "http"));

	if (_useSsl)
	{
		_server->setUseSecure();
		WEBSERVER_DEFAULT_PORT = 8092;
	}

	connect(_server, &QtHttpServer::started, this, &WebServer::onServerStarted);
	connect(_server, &QtHttpServer::stopped, this, &WebServer::onServerStopped);
	connect(_server, &QtHttpServer::error, this, &WebServer::onServerError);

	// create StaticFileServing
	_staticFileServing = new StaticFileServing(this);
	connect(_server, &QtHttpServer::requestNeedsReply, _staticFileServing, &StaticFileServing::onRequestNeedsReply);

	// init
	handleSettingsUpdate(settings::WEBSERVER, _config);
}

void WebServer::onServerStarted(quint16 port)
{
	_inited = true;

	Info(_log, "'%s' started on port %d", _server->getServerName().toStdString().c_str(), port);

	if (_useSsl)
	{
		emit publishService(HTTPS_SERVICE_TYPE, _port, HYPERION_SERVICENAME);
	}
	else
	{
		emit publishService(HTTP_SERVICE_TYPE, _port, HYPERION_SERVICENAME);
	}

	emit stateChange(true);
}

void WebServer::onServerStopped()
{
	Info(_log, "Stopped %s", _server->getServerName().toStdString().c_str());
	emit stateChange(false);
}

void WebServer::onServerError(QString msg)
{
	Error(_log, "%s", msg.toStdString().c_str());
}

void WebServer::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if (type == settings::WEBSERVER)
	{
		Debug(_log, "Apply Webserver settings");
		const QJsonObject& obj = config.object();

		_baseUrl = obj["document_root"].toString(WEBSERVER_DEFAULT_PATH);


		if ((_baseUrl != ":/webconfig") && !_baseUrl.trimmed().isEmpty())
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

		// ssl different port
		quint16 newPort = _useSsl ? obj["sslPort"].toInt(WEBSERVER_DEFAULT_PORT) : obj["port"].toInt(WEBSERVER_DEFAULT_PORT);
		if (_port != newPort)
		{
			_port = newPort;
			stop();
		}

		// eval if the port is available, will be incremented if not
		if (!_server->isListening())
			NetUtils::portAvailable(_port, _log);

		// on ssl we want .key .cert and probably key password
		if (_useSsl)
		{
			QString keyPath = obj["keyPath"].toString(WEBSERVER_DEFAULT_KEY_PATH);
			QString crtPath = obj["crtPath"].toString(WEBSERVER_DEFAULT_CRT_PATH);

			QSslKey currKey = _server->getPrivateKey();
			QList<QSslCertificate> currCerts = _server->getCertificates();

			// check keyPath
			if ((keyPath != WEBSERVER_DEFAULT_KEY_PATH) && !keyPath.trimmed().isEmpty())
			{
				QFileInfo kinfo(keyPath);
				if (!kinfo.exists())
				{
					Error(_log, "No SSL key found at '%s' falling back to internal", keyPath.toUtf8().constData());
					keyPath = WEBSERVER_DEFAULT_KEY_PATH;
				}
			}
			else
				keyPath = WEBSERVER_DEFAULT_KEY_PATH;

			// check crtPath
			if ((crtPath != WEBSERVER_DEFAULT_CRT_PATH) && !crtPath.trimmed().isEmpty())
			{
				QFileInfo cinfo(crtPath);
				if (!cinfo.exists())
				{
					Error(_log, "No SSL certificate found at '%s' falling back to internal", crtPath.toUtf8().constData());
					crtPath = WEBSERVER_DEFAULT_CRT_PATH;
				}
			}
			else
				crtPath = WEBSERVER_DEFAULT_CRT_PATH;

			// load and verify crt
			QFile cfile(crtPath);
			cfile.open(QIODevice::ReadOnly);
			QList<QSslCertificate> validList;
			QList<QSslCertificate> cList = QSslCertificate::fromDevice(&cfile, QSsl::Pem);
			cfile.close();

			// Filter for valid certs
			for (const auto& entry : cList) {
				if (!entry.isNull() && QDateTime::currentDateTime().daysTo(entry.expiryDate()) > 0)
					validList.append(entry);
				else
					Error(_log, "The provided SSL certificate is invalid/not supported/reached expiry date ('%s')", crtPath.toUtf8().constData());
			}

			if (!validList.isEmpty()) {
				Debug(_log, "Setup SSL certificate");
				_server->setCertificates(validList);
			}
			else {
				Error(_log, "No valid SSL certificate has been found ('%s')", crtPath.toUtf8().constData());
			}

			// load and verify key
			QFile kfile(keyPath);
			kfile.open(QIODevice::ReadOnly);
			// The key should be RSA enrcrypted and PEM format, optional the passPhrase
			QSslKey key(&kfile, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, obj["keyPassPhrase"].toString().toUtf8());
			kfile.close();

			if (key.isNull()) {
				Error(_log, "The provided SSL key is invalid or not supported use RSA encrypt and PEM format ('%s')", keyPath.toUtf8().constData());
			}
			else {
				Debug(_log, "Setup private SSL key");
				_server->setPrivateKey(key);
			}
		}

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

void WebServer::setSSDPDescription(const QString& desc)
{
	_staticFileServing->setSSDPDescription(desc);
}
