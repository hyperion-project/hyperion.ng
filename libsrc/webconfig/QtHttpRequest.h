#ifndef QTHTTPREQUEST_H
#define QTHTTPREQUEST_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QUrl>
#include <QHostAddress>

class QtHttpServer;

class QtHttpRequest : public QObject {
    Q_OBJECT

public:
    explicit QtHttpRequest (QtHttpServer * parent);

    struct ClientInfo {
		QHostAddress serverAddress;
		QHostAddress clientAddress;
    };

	int               getRawDataSize (void) const;
    QUrl              getUrl         (void) const;
    QString           getCommand     (void) const;
    QByteArray        getRawData     (void) const;
    QList<QByteArray> getHeadersList (void) const;
    ClientInfo        getClientInfo  (void) const;

    QByteArray getHeader (const QByteArray & header) const;

public slots:
    void setUrl        (const QUrl & url);
    void setCommand    (const QString & command);
    void setClientInfo (const QHostAddress & server, const QHostAddress & client);
    void addHeader     (const QByteArray & header, const QByteArray & value);
    void appendRawData (const QByteArray & data);

private:
    QUrl                          m_url;
    QString                       m_command;
    QByteArray                    m_data;
    QtHttpServer *                m_serverHandle;
    QHash<QByteArray, QByteArray> m_headersHash;
    ClientInfo                    m_clientInfo;
};

#endif // QTHTTPREQUEST_H
