#ifndef STATICFILESERVING_H
#define STATICFILESERVING_H

#include <QObject>
#include <QMimeDatabase>

#include "QtHttpServer.h"
#include "QtHttpRequest.h"
#include "QtHttpReply.h"
#include "QtHttpHeader.h"

class StaticFileServing : public QObject {
    Q_OBJECT

public:
    explicit StaticFileServing (QString baseUrl, quint16 port, quint16 jsonPort, QObject * parent = NULL);
    virtual ~StaticFileServing (void);

public slots:
    void onServerStopped      (void);
    void onServerStarted      (quint16 port);
    void onServerError        (QString msg);
    void onRequestNeedsReply  (QtHttpRequest * request, QtHttpReply * reply);

private:
    QString         m_baseUrl;
    QtHttpServer  * _server;
    QMimeDatabase * m_mimeDb;
    quint16         _jsonPort;
};

#endif // STATICFILESERVING_H
