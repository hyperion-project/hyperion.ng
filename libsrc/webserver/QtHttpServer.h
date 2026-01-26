#ifndef QTHTTPSERVER_H
#define QTHTTPSERVER_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QHostAddress>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(comm_http_server_track);

class QTcpSocket;
class QTcpServer;
class QtHttpRequest;
class QtHttpReply;
class QtHttpClientWrapper;
class NetOrigin;

class QtHttpServerWrapper : public QTcpServer
{
	Q_OBJECT

public:
	explicit QtHttpServerWrapper (QObject * parent = Q_NULLPTR);
	~QtHttpServerWrapper (void) override;

	void setUseSecure (const bool ssl = true);

protected:
	void incomingConnection (qintptr handle) Q_DECL_OVERRIDE;

private:
	bool m_useSsl;
};

class QtHttpServer : public QObject
{
	Q_OBJECT

public:
	explicit QtHttpServer (QObject * parent = Q_NULLPTR);
	~QtHttpServer() override;

	static const QString & HTTP_VERSION;

	using SslErrorSignal = void (QSslSocket::*)(const QList<QSslError> &);

	const QString & getServerName (void) const { return m_serverName; }

	quint16 getServerPort  (void) const { return m_sockServer->serverPort();  }
	QString getErrorString (void) const { return m_sockServer->errorString(); }
	bool    isListening() const         { return m_sockServer->isListening(); }
	bool    isSecure() const            { return m_useSsl;  }

public slots:
	void start           (quint16 port = 0);
	void stop            (void);
	void setUseSecure    (bool ssl = true);
	void setServerName   (const QString & serverName)           { m_serverName = serverName; }
	void setPrivateKey   (const QSslKey & key)                  { m_sslKey = key; }
	void setCertificates (const QList<QSslCertificate> & certs) { m_sslCerts = certs; }
	QSslKey getPrivateKey() const                  			    { return m_sslKey; }
	QList<QSslCertificate> getCertificates() const 				{ return m_sslCerts; }

signals:
	void started            (quint16 port);
	void stopped            (void);
	void error              (const QString & msg);
	void clientConnected    (const QString & guid);
	void clientDisconnected (const QString & guid);
	void requestNeedsReply  (QtHttpRequest * request, QtHttpReply * reply);

private slots:
	void onClientConnected          (void);
	void onClientDisconnected       (void);
	void onClientSslEncrypted       (void) const                             { /* Not used */    }
	void onClientSslPeerVerifyError (const QSslError & err) const      		 { Q_UNUSED (err)    }
	void onClientSslErrors          (const QList<QSslError> & errors) const  { Q_UNUSED (errors) }
	void onClientSslModeChanged     (QSslSocket::SslMode mode) const         { Q_UNUSED (mode)   }

private:
	bool                                       m_useSsl;
	QSslKey                                    m_sslKey;
	QList<QSslCertificate>                     m_sslCerts;
	QString                                    m_serverName;
	QWeakPointer<NetOrigin>                    m_netOriginWeak;
	QScopedPointer<QtHttpServerWrapper>        m_sockServer;
	QHash<QTcpSocket *, QtHttpClientWrapper *> m_socksClientsHash;
};

#endif // QTHTTPSERVER_H
