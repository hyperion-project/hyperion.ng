
#include "StaticFileServing.h"

#include <QStringBuilder>
#include <QUrlQuery>
#include <QDebug>
#include <QList>
#include <QPair>
#include <QFile>

StaticFileServing::StaticFileServing (QString baseUrl, quint16 port, quint16 jsonPort, QObject * parent)
		: QObject   (parent)
		, m_baseUrl (baseUrl)
		, _jsonPort (jsonPort)
{
	m_mimeDb = new QMimeDatabase;

	_server = new QtHttpServer (this);
	_server->setServerName (QStringLiteral ("Qt Static HTTP File Server"));

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
	qDebug () << "QtHttpServer started on port" << port << _server->getServerName ();
}

void StaticFileServing::onServerStopped () {
	qDebug () << "QtHttpServer stopped" << _server->getServerName ();
}

void StaticFileServing::onServerError (QString msg)
{
	qDebug () << "QtHttpServer error :" << msg;
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

		// special uri handling for server commands
		if ( path == "/serverinfo" )
		{
			reply->addHeader ("Content-Type", "text/plain" );
			reply->appendRawData (QByteArrayLiteral(":") % QString::number(_jsonPort).toUtf8() );
			return;
		}
		
		// get static files
		if ( path == "/" || path.isEmpty() || ! QFile::exists(m_baseUrl % "/" % path) )
			path = "index.html";

		QFile file (m_baseUrl % "/" % path);
		if (file.exists ())
		{
			QMimeType mime = m_mimeDb->mimeTypeForFile (file.fileName ());
			if (file.open (QFile::ReadOnly)) {
				QByteArray data = file.readAll ();
				reply->addHeader ("Content-Type", mime.name ().toLocal8Bit ());
				reply->appendRawData (data);
				file.close ();
			}
			else
			{
				printErrorToReply (reply, "Requested file " % m_baseUrl % "/" % path % " couldn't be open for reading !");
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

