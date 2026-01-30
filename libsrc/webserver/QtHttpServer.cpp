
#include "QtHttpServer.h"
#include "QtHttpRequest.h"
#include "QtHttpReply.h"
#include "QtHttpClientWrapper.h"

#include <QUrlQuery>

#include <utils/NetOrigin.h>

Q_LOGGING_CATEGORY(comm_http_server_track, "hyperion.comm.http.server.track");

const QString & QtHttpServer::HTTP_VERSION = QStringLiteral ("HTTP/1.1");

QtHttpServerWrapper::QtHttpServerWrapper (QObject * parent)
	: QTcpServer (parent)
	, m_useSsl   (false)
{
	TRACK_SCOPE();
}

QtHttpServerWrapper::~QtHttpServerWrapper (void)
{
	TRACK_SCOPE();
}

void QtHttpServerWrapper::setUseSecure (const bool ssl)
{
	m_useSsl = ssl;
}

void QtHttpServerWrapper::incomingConnection (qintptr handle)
{
	QTcpSocket * sock = (m_useSsl ? new QSslSocket (this) : new QTcpSocket (this));
	sock->setSocketDescriptor(handle) ? addPendingConnection (sock) : delete sock;
}

QtHttpServer::QtHttpServer (QObject * parent)
	: QObject      (parent)
	, m_useSsl     (false)
	, m_serverName (QStringLiteral ("The Hyperion HTTP Server"))
	, m_netOriginWeak  (NetOrigin::getInstance())
	, m_sockServer (nullptr)
{
	TRACK_SCOPE();
	m_sockServer.reset(new QtHttpServerWrapper());
	connect (m_sockServer.get(), &QtHttpServerWrapper::newConnection, this, &QtHttpServer::onClientConnected, Qt::UniqueConnection);
}

QtHttpServer::~QtHttpServer()
{
	TRACK_SCOPE() << m_socksClientsHash.size() << "clients connected. SSL:" << m_useSsl;
}

void QtHttpServer::start (quint16 port)
{
	if(!m_sockServer->isListening())
	{
		m_sockServer->listen (QHostAddress::Any, port)
			? emit started (m_sockServer->serverPort ())
			: emit error (m_sockServer->errorString ());
	}
}

void QtHttpServer::stop (void)
{
	qCDebug(comm_http_server_track) << "Stopping HTTP-Server..." << m_socksClientsHash.size() << "clients connected. SSL:" << m_useSsl;
	if (m_sockServer != nullptr && m_sockServer->isListening ())
	{
		m_sockServer->close ();
		// disconnect clients
		const auto wrappers = m_socksClientsHash.values();
		for(auto *wrapper : wrappers)
		{
			if (wrapper->isWebSocketConnectionOpen())
			{
				qCDebug(comm_http_server_track) << "Closing Websocket connection for client:" << wrapper->getGuid();
				wrapper->closeWebSocketConnection();
			}
			else
			{
				if (QTcpSocket* sock = m_socksClientsHash.key(wrapper, Q_NULLPTR))
				{
					qCDebug(comm_http_server_track) << "Closing socket connection for client:" << sock->peerAddress().toString() << ":" << sock->peerPort();
					sock->close();
				}
			}
		}
	}

	if (m_socksClientsHash.isEmpty())
	{
		qCDebug(comm_http_server_track) <<  "All clients disconnected.";
		emit stopped ();
	}
}

void QtHttpServer::setUseSecure (const bool ssl)
{
	m_useSsl = ssl;
	m_sockServer->setUseSecure (m_useSsl);
}

void QtHttpServer::onClientConnected (void)
{
	while (m_sockServer->hasPendingConnections ())
	{
		if (QTcpSocket * sock = m_sockServer->nextPendingConnection ())
		{
			if (m_useSsl)
			{
				if (QSslSocket* ssl = qobject_cast<QSslSocket*> (sock))
				{
					connect(ssl, SslErrorSignal(&QSslSocket::sslErrors), this, &QtHttpServer::onClientSslErrors);
					connect(ssl, &QSslSocket::encrypted, this, &QtHttpServer::onClientSslEncrypted);
					connect(ssl, &QSslSocket::peerVerifyError, this, &QtHttpServer::onClientSslPeerVerifyError);
					connect(ssl, &QSslSocket::modeChanged, this, &QtHttpServer::onClientSslModeChanged);
					ssl->setLocalCertificateChain(m_sslCerts);
					ssl->setPrivateKey(m_sslKey);
					ssl->setPeerVerifyMode(QSslSocket::AutoVerifyPeer);
					ssl->startServerEncryption();
				}
			}

			bool isLocal = false;
			if (auto origin = m_netOriginWeak.toStrongRef())
			{
				isLocal = origin->isLocalAddress(sock->peerAddress(), sock->localAddress());
			}
			auto* wrapper = new QtHttpClientWrapper(sock, isLocal, this);
			connect(wrapper, &QtHttpClientWrapper::disconnected, this, &QtHttpServer::onClientDisconnected);

			qCDebug(comm_http_server_track) << "Add Socket connection -" << wrapper->getGuid ();
			m_socksClientsHash.insert(sock, wrapper);
			emit clientConnected (wrapper->getGuid ());
		}
	}
}

void QtHttpServer::onClientDisconnected (void)
{
	if (QtHttpClientWrapper* wrapper = qobject_cast<QtHttpClientWrapper*>(sender()))
	{
		if (QTcpSocket* sockClient = m_socksClientsHash.key(wrapper, Q_NULLPTR))
		{
			qCDebug(comm_http_server_track) << "Remove Socket connection -" << wrapper->getGuid();
			m_socksClientsHash.remove(sockClient);
			emit clientDisconnected(wrapper->getGuid());
			wrapper->deleteLater();
		}
	}

	if (!m_sockServer->isListening() && m_socksClientsHash.isEmpty())
	{
		qCDebug(comm_http_server_track) << "All clients disconnected.";
		emit stopped ();
	}
}
