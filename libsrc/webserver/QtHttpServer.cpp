
#include "QtHttpServer.h"
#include "QtHttpRequest.h"
#include "QtHttpReply.h"
#include "QtHttpClientWrapper.h"

#include <QUrlQuery>

#include <utils/NetOrigin.h>

const QString & QtHttpServer::HTTP_VERSION = QStringLiteral ("HTTP/1.1");

QtHttpServerWrapper::QtHttpServerWrapper (QObject * parent)
	: QTcpServer (parent)
	, m_useSsl   (false)
{

}

QtHttpServerWrapper::~QtHttpServerWrapper (void)
{

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
	, m_serverName (QStringLiteral ("The Qt6 HTTP Server"))
	, m_netOrigin  (NetOrigin::getInstance())
{
	m_sockServer = new QtHttpServerWrapper (this);
	connect (m_sockServer, &QtHttpServerWrapper::newConnection, this, &QtHttpServer::onClientConnected);
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
	if (m_sockServer->isListening ())
	{
		m_sockServer->close ();
		// disconnect clients
		const QList<QTcpSocket*> socks = m_socksClientsHash.keys();
		for(auto sock : socks)
		{
			sock->close();
		}

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
			if(m_netOrigin->accessAllowed(sock->peerAddress(), sock->localAddress()))
			{
				connect (sock, &QTcpSocket::disconnected, this, &QtHttpServer::onClientDisconnected);

				if (m_useSsl)
				{
					if (QSslSocket * ssl = qobject_cast<QSslSocket *> (sock))
					{
						connect (ssl, SslErrorSignal (&QSslSocket::sslErrors), this, &QtHttpServer::onClientSslErrors);
						connect (ssl, &QSslSocket::encrypted,                  this, &QtHttpServer::onClientSslEncrypted);
						connect (ssl, &QSslSocket::peerVerifyError,            this, &QtHttpServer::onClientSslPeerVerifyError);
						connect (ssl, &QSslSocket::modeChanged,                this, &QtHttpServer::onClientSslModeChanged);
						ssl->setLocalCertificateChain (m_sslCerts);
						ssl->setPrivateKey (m_sslKey);
						ssl->setPeerVerifyMode (QSslSocket::AutoVerifyPeer);
						ssl->startServerEncryption ();
					}
				}

				QtHttpClientWrapper * wrapper = new QtHttpClientWrapper (sock, m_netOrigin->isLocalAddress(sock->peerAddress(), sock->localAddress()), this);
				m_socksClientsHash.insert (sock, wrapper);
				emit clientConnected (wrapper->getGuid ());
			}
			else
			{
				sock->close();
			}
		}
	}
}

void QtHttpServer::onClientDisconnected (void)
{
	if (QTcpSocket * sockClient = qobject_cast<QTcpSocket *> (sender ()))
	{
		if (QtHttpClientWrapper * wrapper = m_socksClientsHash.value (sockClient, Q_NULLPTR))
		{
			emit clientDisconnected (wrapper->getGuid ());
			wrapper->deleteLater ();
			m_socksClientsHash.remove (sockClient);
		}
	}
}
