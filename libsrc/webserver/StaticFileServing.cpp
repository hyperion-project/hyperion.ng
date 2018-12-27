
#include "StaticFileServing.h"

#include <QStringBuilder>
#include <QUrlQuery>
#include <QList>
#include <QPair>
#include <QFile>
#include <QFileInfo>
#include <QResource>
#include <exception>

StaticFileServing::StaticFileServing (Hyperion *hyperion, QObject * parent)
	:  QObject   (parent)
	, _hyperion(hyperion)
	, _baseUrl ()
	, _cgi(hyperion, this)
	, _log(Logger::getInstance("WEBSERVER"))
{
	Q_INIT_RESOURCE(WebConfig);

	_mimeDb = new QMimeDatabase;
}

StaticFileServing::~StaticFileServing ()
{

}

void StaticFileServing::setBaseUrl(const QString& url)
{
	_baseUrl = url;
	_cgi.setBaseUrl(url);
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
	if (command == QStringLiteral ("GET"))
	{
		QString path = request->getUrl ().path ();
		QStringList uri_parts = path.split('/', QString::SkipEmptyParts);

		// special uri handling for server commands
		if ( ! uri_parts.empty() && uri_parts.at(0) == "cgi" )
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
