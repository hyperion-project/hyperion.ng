
#include "QtHttpRequest.h"
#include "QtHttpHeader.h"
#include "QtHttpServer.h"

QtHttpRequest::QtHttpRequest (QtHttpClientWrapper * client, QtHttpServer * parent)
	: QObject         (parent)
	, m_url           (QUrl ())
	, m_command       (QString ())
	, m_data          (QByteArray ())
	, m_serverHandle  (parent)
	, m_clientHandle  (client)
	, m_postData      (QtHttpPostData())
{
	// set some additional headers
	addHeader (QtHttpHeader::ContentLength, QByteArrayLiteral ("0"));
	addHeader (QtHttpHeader::Connection,    QByteArrayLiteral ("Keep-Alive"));
}

void QtHttpRequest::setClientInfo (const QHostAddress & server, const QHostAddress & client)
{
	m_clientInfo.serverAddress = server;
	m_clientInfo.clientAddress = client;
}

void QtHttpRequest::addHeader (const QByteArray & header, const QByteArray & value)
{
	QByteArray key = header.trimmed().toLower();

	if (!key.isEmpty ())
	{
		m_headersHash.insert (key, value);
	}
}
