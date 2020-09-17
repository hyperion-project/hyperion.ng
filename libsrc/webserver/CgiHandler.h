#ifndef CGIHANDLER_H
#define CGIHANDLER_H

#include <QObject>
#include <QString>
#include <QStringList>

#include <utils/Logger.h>

#include "QtHttpReply.h"
#include "QtHttpRequest.h"

class CgiHandler : public QObject
{
	Q_OBJECT

public:
	CgiHandler(QObject * parent = nullptr);

	void setBaseUrl(const QString& url);
	void exec(const QStringList & args, QtHttpRequest * request, QtHttpReply * reply);

private:
	// CGI commands
	void cmd_cfg_jsonserver();
	void cmd_runscript();

	QtHttpReply *       _reply;
	QtHttpRequest *     _request;
	QStringList         _args;
	QString             _baseUrl;
	Logger *            _log;
};

#endif // CGIHANDLER_H
