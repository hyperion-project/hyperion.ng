#ifndef CGIHANDLER_H
#define CGIHANDLER_H

#include <QObject>
#include <QString>
#include <QStringList>

#include <hyperion/Hyperion.h>
#include <utils/Logger.h>

#include "QtHttpReply.h"
#include "QtHttpRequest.h"

class CgiHandler : public QObject {
	Q_OBJECT

public:
	CgiHandler (Hyperion * hyperion, QString baseUrl, QObject * parent = NULL);
	virtual ~CgiHandler (void);

	void exec(const QStringList & args,QtHttpRequest * request, QtHttpReply * reply);
	
	// cgi commands
	void cmd_cfg_jsonserver();
	void cmd_cfg_get ();
	void cmd_cfg_set ();
	void cmd_runscript ();
	
private:
	Hyperion*           _hyperion;
	QtHttpReply *       _reply;
	QtHttpRequest *     _request;
	QStringList         _args;
	const QJsonObject & _hyperionConfig;
	const QString       _baseUrl;
	Logger *            _log;
};

#endif // CGIHANDLER_H

 
