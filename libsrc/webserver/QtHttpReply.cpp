
#include "QtHttpReply.h"
#include "QtHttpHeader.h"
#include "QtHttpServer.h"

#include <QDateTime>

QtHttpReply::QtHttpReply (QtHttpServer * parent)
	: QObject        (parent)
	, m_useChunked   (false)
	, m_statusCode   (Ok)
	, m_data         (QByteArray ())
	, m_serverHandle (parent)
{
	// set some additional headers
	addHeader (QtHttpHeader::Date,   QDateTime::currentDateTimeUtc ().toString ("ddd, dd MMM yyyy hh:mm:ss t").toUtf8 ());
	addHeader (QtHttpHeader::Server, m_serverHandle->getServerName ().toUtf8 ());
}

const QByteArray QtHttpReply::getStatusTextForCode (QtHttpReply::StatusCode statusCode)
{
	switch (statusCode)
	{
		case Ok:         return QByteArrayLiteral ("OK.");
		case BadRequest: return QByteArrayLiteral ("Bad request !");
		case Forbidden:  return QByteArrayLiteral ("Forbidden !");
		case NotFound:   return QByteArrayLiteral ("Not found !");
		default:         return QByteArrayLiteral ("");
	}
}

void QtHttpReply::addHeader (const QByteArray & header, const QByteArray & value)
{
	QByteArray key = header.trimmed ();

	if (!key.isEmpty ())
	{
		m_headersHash.insert (key, value);
	}
}
