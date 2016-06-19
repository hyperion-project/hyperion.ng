
#include "QtHttpServer.h"
#include "QtHttpRequest.h"
#include "QtHttpReply.h"
#include "QtHttpClientWrapper.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDebug>

const QString & QtHttpServer::HTTP_VERSION = QStringLiteral ("HTTP/1.1");

QtHttpServer::QtHttpServer (QObject * parent)
    : QObject      (parent)
    , m_serverName (QStringLiteral ("The Qt5 HTTP Server"))
{
    m_sockServer = new QTcpServer (this);
    connect (m_sockServer, &QTcpServer::newConnection, this, &QtHttpServer::onClientConnected);
}

const QString QtHttpServer::getServerName (void) const {
    return m_serverName;
}

void QtHttpServer::start (quint16 port) {
    if (m_sockServer->listen (QHostAddress::Any, port)) {
        emit started (m_sockServer->serverPort ());
    }
    else {
        emit error (m_sockServer->errorString ());
    }
}

void QtHttpServer::stop (void) {
    if (m_sockServer->isListening ()) {
        m_sockServer->close ();
        emit stopped ();
    }
}

void QtHttpServer::setServerName (const QString & serverName) {
    m_serverName = serverName;
}

void QtHttpServer::onClientConnected (void) {
    while (m_sockServer->hasPendingConnections ()) {
        QTcpSocket * sockClient = m_sockServer->nextPendingConnection ();
        QtHttpClientWrapper * wrapper = new QtHttpClientWrapper (sockClient, this);
        connect (sockClient, &QTcpSocket::disconnected, this, &QtHttpServer::onClientDisconnected);
        m_socksClientsHash.insert (sockClient, wrapper);
        emit clientConnected (wrapper->getGuid ());
    }
}

void QtHttpServer::onClientDisconnected (void) {
    QTcpSocket * sockClient = qobject_cast<QTcpSocket *> (sender ());
    if (sockClient) {
        QtHttpClientWrapper * wrapper = m_socksClientsHash.value (sockClient, Q_NULLPTR);
        if (wrapper) {
            emit clientDisconnected (wrapper->getGuid ());
            wrapper->deleteLater ();
            m_socksClientsHash.remove (sockClient);
        }
    }
}
