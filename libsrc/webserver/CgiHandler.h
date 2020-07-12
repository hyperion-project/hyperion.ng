#ifndef CGIHANDLER_H
#define CGIHANDLER_H

#include <QObject>
#include <QString>
#include <QStringList>

#include <utils/Logger.h>

#include "QtHttpReply.h"
#include "QtHttpRequest.h"

class CgiHandler : public QObject {
	Q_OBJECT

public:
	CgiHandler (QObject * parent = NULL);
	virtual ~CgiHandler (void);

	void setBaseUrl(const QString& url);
	bool exec(const QStringList & args,QtHttpRequest * request, QtHttpReply * reply);

private:
	// cgi commands
	bool cmd_cfg_jsonserver();
	bool cmd_runscript ();

	QtHttpReply *       _reply;
	QtHttpRequest *     _request;
	QStringList         _args;
	QString             _baseUrl;
	Logger *            _log;
};

#endif // CGIHANDLER_H
