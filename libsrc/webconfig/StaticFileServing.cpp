
#include "StaticFileServing.h"

#include <QStringBuilder>
#include <QUrlQuery>
#include <QList>
#include <QPair>
#include <QFile>
#include <QFileInfo>

StaticFileServing::StaticFileServing (Hyperion *hyperion, QString baseUrl, quint16 port, QObject * parent)
		:  QObject   (parent)
		, _hyperion(hyperion)
		, _baseUrl (baseUrl)
		, _cgi(hyperion, this)
		, _log(Logger::getInstance("WEBSERVER"))
{
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

void StaticFileServing::onRequestNeedsReply (QtHttpRequest * request, QtHttpReply * reply)
{
	QString command = request->getCommand ();
	if (command == QStringLiteral ("GET"))
	{
		QString path = request->getUrl ().path ();
		QStringList uri_parts = path.split('/', QString::SkipEmptyParts);

		// special uri handling for server commands
		if ( ! uri_parts.empty() && uri_parts.at(0) == "cgi"  )
		{
			uri_parts.removeAt(0);
			try
			{
				_cgi.exec(uri_parts, request, reply);
			}
			catch(...)
			{
				printErrorToReply (reply, "cgi script failed (" % path % ")");
			}
			return;
		}

		QFileInfo info(_baseUrl % "/" % path);
		if ( path == "/" || path.isEmpty() || ! info.exists() )
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
			printErrorToReply (reply, "Requested file " % path % " couldn't be found !");
		}
	}
	else
	{
		printErrorToReply (reply, "Unhandled HTTP/1.1 method " % command % " on static file server !");
	}
}

