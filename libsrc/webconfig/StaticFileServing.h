#ifndef STATICFILESERVING_H
#define STATICFILESERVING_H

#include <QObject>
#include <QMimeDatabase>

#include "QtHttpServer.h"
#include "QtHttpRequest.h"
#include "QtHttpReply.h"
#include "QtHttpHeader.h"
#include "CgiHandler.h"

#include <hyperion/Hyperion.h>

class StaticFileServing : public QObject {
    Q_OBJECT

public:
    explicit StaticFileServing (Hyperion *hyperion, QString baseUrl, quint16 port, QObject * parent = NULL);
    virtual ~StaticFileServing (void);

public slots:
    void onServerStopped      (void);
    void onServerStarted      (quint16 port);
    void onServerError        (QString msg);
    void onRequestNeedsReply  (QtHttpRequest * request, QtHttpReply * reply);

private:
	Hyperion      * _hyperion;
	QString         _baseUrl;
	QtHttpServer  * _server;
	QMimeDatabase * _mimeDb;
	CgiHandler      _cgi;
};

#endif // STATICFILESERVING_H
