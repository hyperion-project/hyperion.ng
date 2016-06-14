#ifndef CGIHANDLER_H
#define CGIHANDLER_H

#include <QObject>
#include <QString>
#include <QStringList>

#include <utils/jsonschema/JsonFactory.h>
#include <hyperion/Hyperion.h>

#include "QtHttpReply.h"

class CgiHandler : public QObject {
	Q_OBJECT

public:
	CgiHandler (Hyperion * hyperion, QObject * parent = NULL);
	virtual ~CgiHandler (void);

	void exec(const QStringList & args, QtHttpReply * reply);
	
	// cgi commands
	void cmd_cfg_jsonserver(const QStringList & args, QtHttpReply * reply);
	void cmd_cfg_hyperion (const QStringList & args, QtHttpReply * reply);
	
private:
	Hyperion*             _hyperion;
	QtHttpReply *         _reply;
	const Json::Value    &_hyperionConfig;
};

#endif // CGIHANDLER_H

 
