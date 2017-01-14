#ifndef QTHTTPREQUEST_H
#define QTHTTPREQUEST_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QUrl>
#include <QHostAddress>
#include <QMap>

class QtHttpServer;
class QtHttpClientWrapper;

using QtHttpPostData = QMap<QString,QByteArray>;

class QtHttpRequest : public QObject {
    Q_OBJECT

public:
    explicit QtHttpRequest (QtHttpClientWrapper * client, QtHttpServer * parent);

    struct ClientInfo {
        QHostAddress serverAddress;
        QHostAddress clientAddress;
    };

    int                   getRawDataSize (void) const;
    QUrl                  getUrl         (void) const;
    QString               getCommand     (void) const;
    QByteArray            getRawData     (void) const;
    QList<QByteArray>     getHeadersList (void) const;
    QtHttpClientWrapper * getClient      (void) const;

    QByteArray getHeader (const QByteArray & header) const;
    QtHttpPostData       getPostData    (void) const;

    ClientInfo getClientInfo (void) const;

public slots:
    void setUrl        (const QUrl & url);
    void setCommand    (const QString & command);
    void setClientInfo (const QHostAddress & server, const QHostAddress & client);
    void addHeader     (const QByteArray & header, const QByteArray & value);
    void appendRawData (const QByteArray & data);
    void setPostData   (const QtHttpPostData & data);

private:
    QUrl                          m_url;
    QString                       m_command;
    QByteArray                    m_data;
    QtHttpServer *                m_serverHandle;
    QtHttpClientWrapper *         m_clientHandle;
    QHash<QByteArray, QByteArray> m_headersHash;
    ClientInfo                    m_clientInfo;
	QtHttpPostData                m_postData;
};

#endif // QTHTTPREQUEST_H
