#ifndef STATICFILESERVING_H
#define STATICFILESERVING_H

#include <QMimeDatabase>

//#include "QtHttpServer.h"
#include "QtHttpRequest.h"
#include "QtHttpReply.h"
#include "QtHttpHeader.h"
#include "CgiHandler.h"

#include <hyperion/Hyperion.h>
#include <utils/Logger.h>

class StaticFileServing : public QObject {
    Q_OBJECT

public:
    explicit StaticFileServing (Hyperion *hyperion, QObject * parent = nullptr);
    virtual ~StaticFileServing (void);

	void setBaseUrl(const QString& url);

public slots:
    void onRequestNeedsReply  (QtHttpRequest * request, QtHttpReply * reply);

private:
	Hyperion      * _hyperion;
	QString         _baseUrl;
	QMimeDatabase * _mimeDb;
	CgiHandler      _cgi;
	Logger        * _log;

	void printErrorToReply (QtHttpReply * reply, QtHttpReply::StatusCode code, QString errorMessage);

};

#endif // STATICFILESERVING_H
