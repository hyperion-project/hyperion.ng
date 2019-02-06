
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

QUrl QtHttpRequest::getUrl (void) const {
    return m_url;
}

QString QtHttpRequest::getCommand (void) const {
    return m_command;
}

QtHttpRequest::ClientInfo QtHttpRequest::getClientInfo (void) const {
    return m_clientInfo;
}

int QtHttpRequest::getRawDataSize (void) const {
    return m_data.size ();
}


QByteArray QtHttpRequest::getRawData (void) const {
    return m_data;
}

QtHttpPostData QtHttpRequest::getPostData (void) const {
    return m_postData;
}

QList<QByteArray> QtHttpRequest::getHeadersList (void) const {
    return m_headersHash.keys ();
}

QtHttpClientWrapper * QtHttpRequest::getClient (void) const {
    return m_clientHandle;
}

QByteArray QtHttpRequest::getHeader (const QByteArray & header) const {
    return m_headersHash.value (header, QByteArray ());
}

void QtHttpRequest::setUrl (const QUrl & url) {
    m_url = url;
}

void QtHttpRequest::setCommand (const QString & command) {
    m_command = command;
}

void QtHttpRequest::setClientInfo (const QHostAddress & server, const QHostAddress & client) {
    m_clientInfo.serverAddress = server;
    m_clientInfo.clientAddress = client;
}

void QtHttpRequest::addHeader (const QByteArray & header, const QByteArray & value) {
    QByteArray key = header.trimmed ();
    if (!key.isEmpty ()) {
        m_headersHash.insert (key, value);
    }
}

void QtHttpRequest::appendRawData (const QByteArray & data) {
    m_data.append (data);
}

void QtHttpRequest::setPostData (const QtHttpPostData & data) {
    m_postData = data;
}
