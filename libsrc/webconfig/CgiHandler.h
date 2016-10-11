#ifndef CGIHANDLER_H
#define CGIHANDLER_H

#include <QObject>
#include <QString>
#include <QStringList>

#include <hyperion/Hyperion.h>

#include "QtHttpReply.h"
#include "QtHttpRequest.h"

class CgiHandler : public QObject {
	Q_OBJECT

public:
	CgiHandler (Hyperion * hyperion, QString baseUrl, QObject * parent = NULL);
	virtual ~CgiHandler (void);

	void exec(const QStringList & args,QtHttpRequest * request, QtHttpReply * reply);
	
	// cgi commands
	void cmd_cfg_jsonserver(const QStringList & args, QtHttpReply * reply);
	void cmd_cfg_hyperion (const QStringList & args, QtHttpReply * reply);
	void cmd_runscript (const QStringList & args, QtHttpReply * reply);
	
private:
	Hyperion*             _hyperion;
	QtHttpReply *         _reply;
	const QJsonObject    &_hyperionConfig;
	const QString     _baseUrl;
};

#endif // CGIHANDLER_H

 
