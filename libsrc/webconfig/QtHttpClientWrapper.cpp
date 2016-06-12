
#include "QtHttpClientWrapper.h"
#include "QtHttpRequest.h"
#include "QtHttpReply.h"
#include "QtHttpServer.h"
#include "QtHttpHeader.h"

#include <QCryptographicHash>
#include <QTcpSocket>
#include <QStringBuilder>
#include <QStringList>
#include <QDateTime>

const QByteArray & QtHttpClientWrapper::CRLF = QByteArrayLiteral ("\r\n");

QtHttpClientWrapper::QtHttpClientWrapper (QTcpSocket * sock, QtHttpServer * parent)
    : QObject          (parent)
    , m_guid           ("")
    , m_parsingStatus  (AwaitingRequest)
    , m_sockClient     (sock)
    , m_currentRequest (Q_NULLPTR)
    , m_serverHandle   (parent)
{
    connect (m_sockClient, &QTcpSocket::readyRead, this, &QtHttpClientWrapper::onClientDataReceived);
}

QString QtHttpClientWrapper::getGuid (void) {
    if (m_guid.isEmpty ()) {
        m_guid = QString::fromLocal8Bit (
                     QCryptographicHash::hash (
                         QByteArray::number ((quint64) (this)),
                         QCryptographicHash::Md5
                         ).toHex ()
                     );
    }
    return m_guid;
}

void QtHttpClientWrapper::onClientDataReceived (void) {
    if (m_sockClient != Q_NULLPTR) {
        while (m_sockClient->bytesAvailable ()) {
            QByteArray line = m_sockClient->readLine ();
            switch (m_parsingStatus) { // handle parsing steps
                case AwaitingRequest: { // "command url version" × 1
                    QString str = QString::fromUtf8 (line).trimmed ();
                    QStringList parts = str.split (SPACE, QString::SkipEmptyParts);
                    if (parts.size () == 3) {
                        QString command = parts.at (0);
                        QString url     = parts.at (1);
                        QString version = parts.at (2);
                        if (version == QtHttpServer::HTTP_VERSION) {
                            //qDebug () << "Debug : HTTP"
                            //          << "command :" << command
                            //          << "url :"     << url
                            //          << "version :" << version;
                            m_currentRequest = new QtHttpRequest (m_serverHandle);
                            m_currentRequest->setUrl     (QUrl (url));
                            m_currentRequest->setCommand (command);
                            m_parsingStatus = AwaitingHeaders;
                        }
                        else {
                            m_parsingStatus = ParsingError;
                            //qWarning () << "Error : unhandled HTTP version :" << version;
                        }
                    }
                    else {
                        m_parsingStatus = ParsingError;
                        //qWarning () << "Error : incorrect HTTP command line :" << line;
                    }
                    break;
                }
                case AwaitingHeaders: { // "header: value" × N (until empty line)
                    QByteArray raw = line.trimmed ();
                    if (!raw.isEmpty ()) { // parse headers
                        int pos = raw.indexOf (COLON);
                        if (pos > 0) {
                            QByteArray header = raw.left (pos).trimmed ();
                            QByteArray value  = raw.mid  (pos +1).trimmed ();
                            //qDebug () << "Debug : HTTP"
                            //          << "header :" << header
                            //          << "value :"  << value;
                            m_currentRequest->addHeader (header, value);
                            if (header == QtHttpHeader::ContentLength) {
                                int  len = -1;
                                bool ok  = false;
                                len = value.toInt (&ok, 10);
                                if (ok) {
                                    m_currentRequest->addHeader (QtHttpHeader::ContentLength, QByteArray::number (len));
                                }
                            }
                        }
                        else {
                            m_parsingStatus = ParsingError;
                            qWarning () << "Error : incorrect HTTP headers line :" << line;
                        }
                    }
                    else { // end of headers
                        //qDebug () << "Debug : HTTP end of headers";
                        if (m_currentRequest->getHeader (QtHttpHeader::ContentLength).toInt () > 0) {
                            m_parsingStatus = AwaitingContent;
                        }
                        else {
                            m_parsingStatus = RequestParsed;
                        }
                    }
                    break;
                }
                case AwaitingContent: { // raw data × N (until EOF ??)
                    m_currentRequest->appendRawData (line);
                    //qDebug () << "Debug : HTTP"
                    //          << "content :" << m_currentRequest->getRawData ().toHex ()
                    //          << "size :"    << m_currentRequest->getRawData ().size  ();
                    if (m_currentRequest->getRawDataSize () == m_currentRequest->getHeader (QtHttpHeader::ContentLength).toInt ()) {
                        //qDebug () << "Debug : HTTP end of content";
                        m_parsingStatus = RequestParsed;
                    }
                    break;
                }
                default: { break; }
            }
            switch (m_parsingStatus) { // handle parsing status end/error
                case RequestParsed: { // a valid request has ben fully parsed
                    QtHttpReply reply (m_serverHandle);
                    connect (&reply, &QtHttpReply::requestSendHeaders,
                             this, &QtHttpClientWrapper::onReplySendHeadersRequested);
                    connect (&reply, &QtHttpReply::requestSendData,
                             this, &QtHttpClientWrapper::onReplySendDataRequested);
                    emit m_serverHandle->requestNeedsReply (m_currentRequest, &reply); // allow app to handle request
                    m_parsingStatus = sendReplyToClient (&reply);
                    break;
                }
                case ParsingError: { // there was an error durin one of parsing steps
                    m_sockClient->readAll (); // clear remaining buffer to ignore content
                    QtHttpReply reply (m_serverHandle);
                    reply.setStatusCode (QtHttpReply::BadRequest);
                    reply.appendRawData (QByteArrayLiteral ("<h1>Bad Request (HTTP parsing error) !</h1>"));
                    reply.appendRawData (CRLF);
                    m_parsingStatus = sendReplyToClient (&reply);
                    break;
                }
                default: { break; }
            }
        }
    }
}

void QtHttpClientWrapper::onReplySendHeadersRequested (void) {
    QtHttpReply * reply = qobject_cast<QtHttpReply *> (sender ());
    if (reply != Q_NULLPTR) {
        QByteArray data;
        // HTTP Version + Status Code + Status Msg
        data.append (QtHttpServer::HTTP_VERSION);
        data.append (SPACE);
        data.append (QByteArray::number (reply->getStatusCode ()));
        data.append (SPACE);
        data.append (QtHttpReply::getStatusTextForCode (reply->getStatusCode ()));
        data.append (CRLF);
        // Header name: header value
        if (reply->useChunked ()) {
            static const QByteArray & CHUNKED = QByteArrayLiteral ("chunked");
            reply->addHeader (QtHttpHeader::TransferEncoding, CHUNKED);
        }
        else {
            reply->addHeader (QtHttpHeader::ContentLength, QByteArray::number (reply->getRawDataSize ()));
        }
        const QList<QByteArray> & headersList = reply->getHeadersList ();
        foreach (const QByteArray & header, headersList) {
            data.append (header);
            data.append (COLON);
            data.append (SPACE);
            data.append (reply->getHeader (header));
            data.append (CRLF);
        }
        // empty line
        data.append (CRLF);
        m_sockClient->write (data);
        m_sockClient->flush ();
    }
}

void QtHttpClientWrapper::onReplySendDataRequested (void) {
    QtHttpReply * reply = qobject_cast<QtHttpReply *> (sender ());
    if (reply != Q_NULLPTR) {
        // content raw data
        QByteArray data = reply->getRawData ();
        if (reply->useChunked ()) {
            data.prepend (QByteArray::number (data.size (), 16) % CRLF);
            data.append (CRLF);
            reply->resetRawData ();
        }
        // write to socket
        m_sockClient->write (data);
        m_sockClient->flush ();
    }
}

QtHttpClientWrapper::ParsingStatus QtHttpClientWrapper::sendReplyToClient (QtHttpReply * reply) {
    if (reply != Q_NULLPTR) {
        if (!reply->useChunked ()) {
            reply->appendRawData (CRLF);
            // send all headers and all data in one shot
            reply->requestSendHeaders ();
            reply->requestSendData ();
        }
        else {
            // last chunk
            m_sockClient->write ("0" % CRLF % CRLF);
            m_sockClient->flush ();
        }
        if (m_currentRequest != Q_NULLPTR) {
            static const QByteArray & CLOSE = QByteArrayLiteral ("close");
            if (m_currentRequest->getHeader (QtHttpHeader::Connection).toLower () == CLOSE) {
                // must close connection after this request
                m_sockClient->close ();
            }
            m_currentRequest->deleteLater ();
            m_currentRequest = Q_NULLPTR;
        }
    }
    return AwaitingRequest;
}
