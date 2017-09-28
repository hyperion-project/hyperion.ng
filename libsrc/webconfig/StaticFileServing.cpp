
#include "StaticFileServing.h"

#include <QStringBuilder>
#include <QUrlQuery>
#include <QList>
#include <QPair>
#include <QFile>
#include <QFileInfo>
#include <QResource>
#include <QHostInfo>
#include <bonjour/bonjourserviceregister.h>
#include <bonjour/bonjourrecord.h>
#include <HyperionConfig.h>
#include <exception>

StaticFileServing::StaticFileServing (Hyperion *hyperion, QString baseUrl, quint16 port, QObject * parent)
	:  QObject   (parent)
	, _hyperion(hyperion)
	, _baseUrl (baseUrl)
	, _cgi(hyperion, baseUrl, this)
	, _log(Logger::getInstance("WEBSERVER"))
{
	Q_INIT_RESOURCE(WebConfig);

	_mimeDb = new QMimeDatabase;

	_server = new QtHttpServer (this);
	_server->setServerName (QStringLiteral ("Hyperion WebConfig"));

	connect (_server, &QtHttpServer::started,           this, &StaticFileServing::onServerStarted);
	connect (_server, &QtHttpServer::stopped,           this, &StaticFileServing::onServerStopped);
	connect (_server, &QtHttpServer::error,             this, &StaticFileServing::onServerError);
	connect (_server, &QtHttpServer::requestNeedsReply, this, &StaticFileServing::onRequestNeedsReply);

	_server->start (port);

}

StaticFileServing::~StaticFileServing ()
{
	_server->stop ();
}

void StaticFileServing::onServerStarted (quint16 port)
{
	Info(_log, "started on port %d name '%s'", port ,_server->getServerName().toStdString().c_str());
	const QJsonObject & generalConfig = _hyperion->getQJsonConfig()["general"].toObject();
	const QString mDNSDescr = generalConfig["name"].toString("") + "@" + QHostInfo::localHostName() + ":" + QString::number(port);

	// txt record for zeroconf
	QString id = _hyperion->id;
	std::string version = HYPERION_VERSION;
	std::vector<std::pair<std::string, std::string> > txtRecord = {{"id",id.toStdString()},{"version",version}};

	BonjourServiceRegister *bonjourRegister_http = new BonjourServiceRegister();
	bonjourRegister_http->registerService(
		BonjourRecord(mDNSDescr, "_hyperiond-http._tcp", QString()),
		port,
		txtRecord
		);
	Debug(_log, "Web Config mDNS responder started");

	// json-rpc for http
	_jsonProcessor = new JsonProcessor(QString("HTTP-API"), _log, true);
}

void StaticFileServing::onServerStopped () {
	Info(_log, "stopped %s", _server->getServerName().toStdString().c_str());
	delete _jsonProcessor;
}

void StaticFileServing::onServerError (QString msg)
{
	Error(_log, "%s", msg.toStdString().c_str());
}

void StaticFileServing::printErrorToReply (QtHttpReply * reply, QtHttpReply::StatusCode code, QString errorMessage)
{
	reply->setStatusCode(code);
	reply->addHeader ("Content-Type", QByteArrayLiteral ("text/html"));
	QFile errorPageHeader(_baseUrl %  "/errorpages/header.html" );
	QFile errorPageFooter(_baseUrl %  "/errorpages/footer.html" );
	QFile errorPage      (_baseUrl %  "/errorpages/" % QString::number((int)code) % ".html" );

	if (errorPageHeader.open (QFile::ReadOnly))
	{
		QByteArray data = errorPageHeader.readAll();
		reply->appendRawData (data);
		errorPageHeader.close ();
	}

	if (errorPage.open (QFile::ReadOnly))
	{
		QByteArray data = errorPage.readAll();
		data = data.replace("{MESSAGE}", errorMessage.toLocal8Bit() );
		reply->appendRawData (data);
		errorPage.close ();
	}
	else
	{
		reply->appendRawData (QString(QString::number(code) + " - " +errorMessage).toLocal8Bit());
	}

	if (errorPageFooter.open (QFile::ReadOnly))
	{
		QByteArray data = errorPageFooter.readAll ();
		reply->appendRawData (data);
		errorPageFooter.close ();
	}
}

void StaticFileServing::onRequestNeedsReply (QtHttpRequest * request, QtHttpReply * reply)
{
	QString command = request->getCommand ();
	if (command == QStringLiteral ("GET") || command == QStringLiteral ("POST"))
	{
		QString path = request->getUrl ().path ();
		QStringList uri_parts = path.split('/', QString::SkipEmptyParts);

		// special uri handling for server commands
		if ( ! uri_parts.empty() )
		{
			if(uri_parts.at(0) == "cgi")
			{
				uri_parts.removeAt(0);
				try
				{
					_cgi.exec(uri_parts, request, reply);
				}
				catch(int err)
				{
					Error(_log,"Exception while executing cgi %s :  %d", path.toStdString().c_str(), err);
					printErrorToReply (reply, QtHttpReply::InternalError, "script failed (" % path % ")");
				}
				catch(std::exception &e)
				{
					Error(_log,"Exception while executing cgi %s :  %s", path.toStdString().c_str(), e.what());
					printErrorToReply (reply, QtHttpReply::InternalError, "script failed (" % path % ")");
				}
				return;
			}
			else if ( uri_parts.at(0) == "json-rpc" )
			{
				QMetaObject::Connection m_connection;
				QByteArray data = request->getRawData();
				QtHttpRequest::ClientInfo info = request->getClientInfo();

				m_connection = QObject::connect(_jsonProcessor, &JsonProcessor::callbackMessage,
					[reply](QJsonObject result) {
						QJsonDocument doc(result);
						reply->addHeader ("Content-Type", "application/json");
						reply->appendRawData (doc.toJson());
				});

				_jsonProcessor->handleMessage(data,info.clientAddress.toString());
				QObject::disconnect( m_connection );
				return;
			}
		}
		Q_INIT_RESOURCE(WebConfig);

		QFileInfo info(_baseUrl % "/" % path);
		if ( path == "/" || path.isEmpty()  )
		{
			path = "index.html";
		}
		else if (info.isDir() && path.endsWith("/") )
		{
			path += "index.html";
		}
		else if (info.isDir() && ! path.endsWith("/") )
		{
			path += "/index.html";
		}

		// get static files
		QFile file(_baseUrl % "/" % path);
		if (file.exists())
		{
			QMimeType mime = _mimeDb->mimeTypeForFile (file.fileName ());
			if (file.open (QFile::ReadOnly)) {
				QByteArray data = file.readAll ();
				reply->addHeader ("Content-Type", mime.name ().toLocal8Bit ());
				reply->appendRawData (data);
				file.close ();
			}
			else
			{
				printErrorToReply (reply, QtHttpReply::Forbidden ,"Requested file: " % path);
			}
		}
		else
		{
			printErrorToReply (reply, QtHttpReply::NotFound, "Requested file: " % path);
		}
	}
	else
	{
		printErrorToReply (reply, QtHttpReply::MethodNotAllowed,"Unhandled HTTP/1.1 method " % command);
	}
}
