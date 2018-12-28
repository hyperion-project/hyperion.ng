#ifndef STATICFILESERVING_H
#define STATICFILESERVING_H

// locales includes
#include "CgiHandler.h"

// qt includes
#include <QMimeDatabase>
#include "QtHttpRequest.h"
#include "QtHttpReply.h"
#include "QtHttpHeader.h"

//utils includes
#include <utils/Logger.h>

class StaticFileServing : public QObject {
    Q_OBJECT

public:
    explicit StaticFileServing (QObject * parent = nullptr);
    virtual ~StaticFileServing (void);

	void setBaseUrl(const QString& url);

public slots:
    void onRequestNeedsReply  (QtHttpRequest * request, QtHttpReply * reply);

private:
	QString         _baseUrl;
	QMimeDatabase * _mimeDb;
	CgiHandler      _cgi;
	Logger        * _log;

	void printErrorToReply (QtHttpReply * reply, QtHttpReply::StatusCode code, QString errorMessage);

};

#endif // STATICFILESERVING_H
