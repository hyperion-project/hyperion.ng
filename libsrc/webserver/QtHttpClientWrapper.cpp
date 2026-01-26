#include <QCryptographicHash>
#include <QTcpSocket>
#include <QStringBuilder>
#include <QStringList>
#include <QDateTime>
#include <QHostAddress>
#include <QLoggingCategory>

#include <utils/QStringUtils.h>

#include "QtHttpClientWrapper.h"
#include "QtHttpRequest.h"
#include "QtHttpReply.h"
#include "QtHttpServer.h"
#include "QtHttpHeader.h"
#include "WebSocketJsonHandler.h"
#include "WebJsonRpc.h"
#include <utils/MemoryTracker.h>


Q_LOGGING_CATEGORY(comm_http_client_track, "hyperion.comm.http.client.track");

const QByteArray & QtHttpClientWrapper::CRLF = QByteArrayLiteral ("\r\n");

QtHttpClientWrapper::QtHttpClientWrapper (QTcpSocket * sock, const bool& localConnection, QtHttpServer * parent)
	: QObject          (parent)
	, m_guid           ("")
	, m_parsingStatus  (AwaitingRequest)
	, m_sockClient     (sock)
	, m_currentRequest (Q_NULLPTR)
	, m_serverHandle   (parent)
	, m_localConnection(localConnection)
	, m_websocketClient(nullptr)
	, m_webJsonRpc     (nullptr)
	, m_websocketServer (nullptr)
{
	TRACK_SCOPE();
	connect(m_sockClient, &QTcpSocket::readyRead, this, &QtHttpClientWrapper::onClientDataReceived);
	connect(m_sockClient, &QTcpSocket::disconnected, this, &QtHttpClientWrapper::disconnected);
}

QString QtHttpClientWrapper::getGuid (void)
{
	if (m_guid.isEmpty ())
	{
		m_guid = QString::fromLocal8Bit (
			QCryptographicHash::hash (
			QByteArray::number (reinterpret_cast<quint64>(this)),
			QCryptographicHash::Md5
			).toHex ()
		);
	}

	return m_guid;
}

void QtHttpClientWrapper::injectCorsHeaders(QtHttpReply* reply) const
{
	if (reply == nullptr)
		return;

	// Add CORS headers if not already set
	if (reply->getHeader(QtHttpHeader::AccessControlAllowOrigin).isEmpty())
		reply->addHeader(QtHttpHeader::AccessControlAllowOrigin, "*");

	if (reply->getHeader(QtHttpHeader::AccessControlAllowMethods).isEmpty())
		reply->addHeader(QtHttpHeader::AccessControlAllowMethods, "POST, GET, OPTIONS");

	if (reply->getHeader(QtHttpHeader::AccessControlAllowHeaders).isEmpty())
		reply->addHeader(QtHttpHeader::AccessControlAllowHeaders, "Authorization, Content-Type");
}

void QtHttpClientWrapper::onClientDataReceived (void)
{
	if (m_sockClient != Q_NULLPTR && m_sockClient->isOpen())
	{
		if (!m_sockClient->isTransactionStarted())
		{
			m_sockClient->startTransaction();
		}

		while (m_sockClient->bytesAvailable () != 0)
		{
			QByteArray line = m_sockClient->readLine ();

			switch (m_parsingStatus) // handle parsing steps
			{
			case AwaitingRequest: // "command url version" × 1
			{
				QString str = QString::fromUtf8 (line).trimmed ();
				QStringList parts = QStringUtils::split(str,SPACE, QStringUtils::SplitBehavior::SkipEmptyParts);
				if (parts.size () == 3)
				{
					const QString& command = parts.at (0);
					const QString& url     = parts.at (1);
					const QString& version = parts.at (2);

					if (version == QtHttpServer::HTTP_VERSION)
					{
						m_currentRequest = new QtHttpRequest (this, m_serverHandle);
						m_currentRequest->setClientInfo(m_sockClient->localAddress(), m_sockClient->peerAddress());
						m_currentRequest->setUrl (QUrl (url));
						m_currentRequest->setCommand (command);
						m_parsingStatus = AwaitingHeaders;
					}
					else
					{
						m_parsingStatus = ParsingError;
						// Error : unhandled HTTP version
					}
				}
				else
				{
					m_parsingStatus = ParsingError;
					// Error : incorrect HTTP command line
				}
				m_fragment.clear();

				break;
			}
			case AwaitingHeaders: // "header: value" × N (until empty line)
			{
				m_fragment.append(line);

				if ( m_fragment.endsWith(CRLF))
				{
					QByteArray raw = m_fragment.trimmed ();
					if (!raw.isEmpty ()) // parse headers
					{
						auto pos = raw.indexOf (COLON);
						if (pos > 0)
						{
							QByteArray header = raw.left (pos).trimmed();
							QByteArray value  = raw.mid  (pos +1).trimmed();
							m_currentRequest->addHeader (header, value);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
							if (header.compare(QtHttpHeader::ContentLength, Qt::CaseInsensitive) == 0)
#else
							if (header.toLower() == QtHttpHeader::ContentLength.toLower())
#endif
							{
								bool isConversionOk  = false;
								const int len = value.toInt (&isConversionOk, 10);
								if (isConversionOk)
								{
									m_currentRequest->addHeader (QtHttpHeader::ContentLength, QByteArray::number (len));
								}
							}
						}
						else
						{
							m_parsingStatus = ParsingError;
							qWarning () << "Error : incorrect HTTP headers line :" << line;
						}
					}
					else // end of headers
					{
						if (m_currentRequest->getHeader (QtHttpHeader::ContentLength).toInt () > 0)
						{
							m_parsingStatus = AwaitingContent;
						}
						else
						{
							m_parsingStatus = RequestParsed;
						}
					}
					m_fragment.clear();
				}

				break;
			}
			case AwaitingContent: // raw data × N (until EOF ??)
			{
				m_currentRequest->appendRawData (line);

				if (m_currentRequest->getRawDataSize () == m_currentRequest->getHeader (QtHttpHeader::ContentLength).toInt ())
				{
					m_parsingStatus = RequestParsed;
				}

				break;
			}
			default:
			{
				break;
			}
			}

			switch (m_parsingStatus) // handle parsing status end/error
			{
			case RequestParsed: // a valid request has ben fully parsed
			{
				// Handle CORS Preflight (OPTIONS)
				if (m_currentRequest->getCommand() == "OPTIONS")
				{
					QtHttpReply reply(m_serverHandle);
					reply.setStatusCode(QtHttpReply::NoContent); // 204 No Content

					injectCorsHeaders(&reply);
					reply.addHeader(QtHttpHeader::AccessControlMaxAge, "86400"); // Cache for 1 day

					connect(&reply, &QtHttpReply::requestSendHeaders, this, &QtHttpClientWrapper::onReplySendHeadersRequested, Qt::UniqueConnection);
					connect(&reply, &QtHttpReply::requestSendData, this, &QtHttpClientWrapper::onReplySendDataRequested, Qt::UniqueConnection);

					m_parsingStatus = sendReplyToClient(&reply);

					return; // Important: return early, do NOT continue to normal POST/jsonrpc handling
				}

				const auto& upgradeValue = m_currentRequest->getHeader(QtHttpHeader::Upgrade).toLower();
				if (upgradeValue == "websocket")
				{
					if(m_websocketClient == Q_NULLPTR)
					{
						qCDebug(comm_http_client_track) << "Upgrading connection to WebSocket.";
						// disconnect this slot from socket for further requests
						disconnect(m_sockClient, &QTcpSocket::readyRead, this, &QtHttpClientWrapper::onClientDataReceived);
						m_sockClient->rollbackTransaction();

						QString servername = QCoreApplication::applicationName() + QLatin1Char('/') + HYPERION_VERSION;
						QWebSocketServer::SslMode secureMode = m_serverHandle->isSecure() ? QWebSocketServer::SecureMode : QWebSocketServer::NonSecureMode;
						m_websocketServer = MAKE_TRACKED_SHARED(QWebSocketServer, servername, secureMode);
						connect(m_websocketServer.get(), &QWebSocketServer::newConnection,
							this, &QtHttpClientWrapper::onNewWebSocketConnection);

						m_websocketServer->handleConnection(m_sockClient);
						emit m_sockClient->readyRead();
						return;
					}

					break;
				}

				m_sockClient->commitTransaction();
				// add  post data to request and catch /jsonrpc subroute url
				if ( m_currentRequest->getCommand() == "POST")
				{
					QtHttpPostData  postData;
					QByteArray data = m_currentRequest->getRawData();
					QList<QByteArray> parts = data.split('&');

					for (const auto &part : parts)
					{
						QList<QByteArray> keyValue = part.split('=');
						QByteArray value;

						if (keyValue.size()>1)
						{
							value = QByteArray::fromPercentEncoding(keyValue.at(1));
						}

						postData.insert(QString::fromUtf8(keyValue.at(0)),value);
					}

					m_currentRequest->setPostData(postData);

					// catch /jsonrpc in url, we need async callback, StaticFileServing is sync
					QString path = m_currentRequest->getUrl ().path ();

					QStringList uri_parts = QStringUtils::split(path,'/', QStringUtils::SplitBehavior::SkipEmptyParts);
					if ( ! uri_parts.empty() && uri_parts.at(0) == "json-rpc" )
					{
						if(m_webJsonRpc.isNull())
						{
							m_webJsonRpc = MAKE_TRACKED_SHARED(WebJsonRpc, this, m_currentRequest, m_serverHandle, m_localConnection);
						}

						m_webJsonRpc->handleMessage(m_currentRequest);
						break;
					}
				}

				QtHttpReply reply (m_serverHandle);
				connect(&reply, &QtHttpReply::requestSendHeaders, this, &QtHttpClientWrapper::onReplySendHeadersRequested, Qt::UniqueConnection);
				connect(&reply, &QtHttpReply::requestSendData, this, &QtHttpClientWrapper::onReplySendDataRequested, Qt::UniqueConnection);

				emit m_serverHandle->requestNeedsReply (m_currentRequest, &reply); // allow app to handle request
				m_parsingStatus = sendReplyToClient (&reply);

				break;
			}
			case ParsingError: // there was an error durin one of parsing steps
			{
				m_sockClient->readAll (); // clear remaining buffer to ignore content
				m_sockClient->commitTransaction();

				QtHttpReply reply (m_serverHandle);
				reply.setStatusCode (QtHttpReply::BadRequest);
				reply.appendRawData (QByteArrayLiteral ("<h1>Bad Request (HTTP parsing error) !</h1>"));
				reply.appendRawData (CRLF);
				m_parsingStatus = sendReplyToClient (&reply);

				break;
			}
			default:
			{
				break;
			}
			}
		}
	}
}

void QtHttpClientWrapper::onReplySendHeadersRequested (void)
{
	QtHttpReply * reply = qobject_cast<QtHttpReply *> (sender ());

	if (reply != Q_NULLPTR)
	{
		injectCorsHeaders(reply);

		QByteArray data;
		// HTTP Version + Status Code + Status Msg
		data.append (QtHttpServer::HTTP_VERSION.toUtf8());
		data.append (SPACE);
		data.append (QByteArray::number (reply->getStatusCode ()));
		data.append (SPACE);
		data.append (QtHttpReply::getStatusTextForCode (reply->getStatusCode ()));
		data.append (CRLF);

		if (reply->useChunked ()) // Header name: header value
		{
			static const QByteArray & CHUNKED = QByteArrayLiteral ("chunked");
			reply->addHeader (QtHttpHeader::TransferEncoding, CHUNKED);
		}
		else
		{
			reply->addHeader (QtHttpHeader::ContentLength, QByteArray::number (reply->getRawDataSize ()));
		}

		const QList<QByteArray> & headersList = reply->getHeadersList ();
		foreach (const QByteArray & header, headersList)
		{
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

void QtHttpClientWrapper::onReplySendDataRequested (void)
{
	QtHttpReply * reply = qobject_cast<QtHttpReply *> (sender ());
	if (reply != Q_NULLPTR)
	{
		// content raw data
		QByteArray data = reply->getRawData ();

		if (reply->useChunked ())
		{
			data.prepend (QByteArray::number (data.size (), 16) % CRLF);
			data.append (CRLF);
			reply->resetRawData ();
		}

		// write to socket
		m_sockClient->write (data);
		m_sockClient->flush ();
	}
}

void QtHttpClientWrapper::sendToClientWithReply(QtHttpReply * reply)
{
	connect (reply, &QtHttpReply::requestSendHeaders, this, &QtHttpClientWrapper::onReplySendHeadersRequested, Qt::UniqueConnection);
	connect (reply, &QtHttpReply::requestSendData, this, &QtHttpClientWrapper::onReplySendDataRequested, Qt::UniqueConnection);
	m_parsingStatus = sendReplyToClient (reply);
}

QtHttpClientWrapper::ParsingStatus QtHttpClientWrapper::sendReplyToClient (QtHttpReply * reply)
{
	if (reply != Q_NULLPTR)
	{
		if (!reply->useChunked ())
		{
			// send all headers and all data in one shot
			emit reply->requestSendHeaders ();
			emit reply->requestSendData ();
		}
		else
		{
			// last chunk
			m_sockClient->write ("0" % CRLF % CRLF);
			m_sockClient->flush ();
		}

		if (m_currentRequest != Q_NULLPTR)
		{
			static const QByteArray & CLOSE = QByteArrayLiteral ("close");

			if (m_currentRequest->getHeader(QtHttpHeader::Connection).toLower() == CLOSE)
			{
				qCDebug(comm_http_client_track) << "Closing connection as per 'Connection: close' header from client request.";
				// must close connection after this request
				m_sockClient->close ();
			}

			m_currentRequest->deleteLater ();
			m_currentRequest = Q_NULLPTR;
		}
	}

	return AwaitingRequest;
}

void QtHttpClientWrapper::closeConnection()
{
	qCDebug(comm_http_client_track) << "Closing connection for client:" << this->getGuid();
	// probably filter for request to follow http spec
	if(m_currentRequest != Q_NULLPTR)
	{
		QtHttpReply reply(m_serverHandle);
		reply.setStatusCode(QtHttpReply::StatusCode::Forbidden);

		connect (&reply, &QtHttpReply::requestSendHeaders, this, &QtHttpClientWrapper::onReplySendHeadersRequested, Qt::UniqueConnection);
		connect (&reply, &QtHttpReply::requestSendData, this, &QtHttpClientWrapper::onReplySendDataRequested, Qt::UniqueConnection);

		m_parsingStatus = sendReplyToClient(&reply);
	}
	m_sockClient->close ();
}

void QtHttpClientWrapper::closeWebSocketConnection()
{
	if (!m_websocketClient.isNull())
	{
		qCDebug(comm_http_client_track) << "Closing WebSocket connection for client:" << this->getGuid();
		m_websocketClient->close();
	}
}

void QtHttpClientWrapper::onNewWebSocketConnection() {

	// Handle the pending connection
	QWebSocket* webSocket = m_websocketServer->nextPendingConnection();
	if (webSocket != nullptr) {
		// Manage the WebSocketJsonHandler for this connection
		m_websocketClient = MAKE_TRACKED_SHARED(WebSocketJsonHandler, webSocket);
		connect(webSocket, &QWebSocket::disconnected, this, [this]() {
			qCDebug(comm_http_client_track) << "WebSocket disconnected for client:" << this->getGuid();
			emit disconnected();
			deleteLater();
		});
	}
}
