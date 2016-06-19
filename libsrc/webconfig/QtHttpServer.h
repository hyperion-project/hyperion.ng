#ifndef QTHTTPSERVER_H
#define QTHTTPSERVER_H

#include <QObject>
#include <QString>
#include <QHash>

class QTcpSocket;
class QTcpServer;

class QtHttpRequest;
class QtHttpReply;
class QtHttpClientWrapper;

class QtHttpServer : public QObject {
    Q_OBJECT

public:
    explicit QtHttpServer (QObject * parent = Q_NULLPTR);

    static const QString & HTTP_VERSION;

    const QString getServerName (void) const;

public slots:
    void start         (quint16 port = 0);
    void stop          (void);
    void setServerName (const QString & serverName);

signals:
    void started            (quint16 port);
    void stopped            (void);
    void error              (const QString & msg);
    void clientConnected    (const QString & guid);
    void clientDisconnected (const QString & guid);
    void requestNeedsReply  (QtHttpRequest * request, QtHttpReply * reply);

private slots:
    void onClientConnected    (void);
    void onClientDisconnected (void);

private:
    QString                                    m_serverName;
    QTcpServer *                               m_sockServer;
    QHash<QTcpSocket *, QtHttpClientWrapper *> m_socksClientsHash;
};

#endif // QTHTTPSERVER_H
