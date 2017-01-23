
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
	Info(_log, "started on port %d name \"%s\"", port ,_server->getServerName().toStdString().c_str());

        const std::string mDNSDescr = ( _server->getServerName().toStdString()
                                        + "@" +
                                        QHostInfo::localHostName().toStdString()
                                        );

	BonjourServiceRegister *bonjourRegister_http = new BonjourServiceRegister();
	bonjourRegister_http->registerService(
		BonjourRecord(mDNSDescr.c_str(), "_http._tcp", QString()),
		port
		);
	Debug(_log, "Web Config mDNS responder started");
}

void StaticFileServing::onServerStopped () {
	Info(_log, "stopped %s", _server->getServerName().toStdString().c_str());
}

void StaticFileServing::onServerError (QString msg)
{
	Error(_log, "%s", msg.toStdString().c_str());
}

static inline void printErrorToReply (QtHttpReply * reply, QString errorMessage)
{
	reply->addHeader ("Content-Type", QByteArrayLiteral ("text/plain"));
	reply->appendRawData (errorMessage.toLocal8Bit ());
}

static inline void printError404ToReply (QtHttpReply * reply, QString errorMessage)
{
	reply->setStatusCode(QtHttpReply::NotFound);
	reply->addHeader ("Content-Type", QByteArrayLiteral ("text/plain"));
	reply->appendRawData (errorMessage.toLocal8Bit ());
}

void StaticFileServing::onRequestNeedsReply (QtHttpRequest * request, QtHttpReply * reply)
{
	QString command = request->getCommand ();
	if (command == QStringLiteral ("GET") || command == QStringLiteral ("POST"))
	{
		QString path = request->getUrl ().path ();
		QStringList uri_parts = path.split('/', QString::SkipEmptyParts);

		// special uri handling for server commands
		if ( ! uri_parts.empty() && uri_parts.at(0) == "cgi"  )
		{
			uri_parts.removeAt(0);
			try
			{
				if (command == QStringLiteral ("POST"))
				{
					QString postData = request->getRawData();
					uri_parts.append(postData.split('&', QString::SkipEmptyParts));
				}
				_cgi.exec(uri_parts, request, reply);
			}
			catch(...)
			{
				printErrorToReply (reply, "script failed (" % path % ")");
			}
			return;
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
				printErrorToReply (reply, "Requested file " % path % " couldn't be open for reading !");
			}
		}
		else
		{
			printError404ToReply (reply, "404 Not Found\n" % path % " couldn't be found !");
		}
	}
	else
	{
		printErrorToReply (reply, "Unhandled HTTP/1.1 method " % command % " on web server !");
	}
}

