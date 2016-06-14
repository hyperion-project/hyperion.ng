#include <QStringBuilder>
#include <QUrlQuery>
#include <QFile>
#include <QByteArray>

#include "CgiHandler.h"

CgiHandler::CgiHandler (Hyperion * hyperion, QObject * parent)
	: QObject(parent)
	, _hyperion(hyperion)
	, _hyperionConfig(_hyperion->getJsonConfig())
{
}

CgiHandler::~CgiHandler()
{
}

void CgiHandler::exec(const QStringList & args, QtHttpReply * reply)
{
	try
	{
		cmd_cfg_jsonserver(args,reply);
		cmd_cfg_hyperion(args,reply);
		throw 1;
	}
	catch(int e)
	{
		if (e != 0)
			throw 1;
	}
}

void CgiHandler::cmd_cfg_jsonserver(const QStringList & args, QtHttpReply * reply)
{
	if ( args.at(0) == "cfg_jsonserver" )
	{
		quint16 jsonPort = 19444;
		if (_hyperionConfig.isMember("jsonServer"))
		{
			const Json::Value & jsonConfig = _hyperionConfig["jsonServer"];
			jsonPort = jsonConfig.get("port", jsonPort).asUInt();
		}

		// send result as reply
		reply->addHeader ("Content-Type", "text/plain" );
		reply->appendRawData (QByteArrayLiteral(":") % QString::number(jsonPort).toUtf8() );
		throw 0;
	}
}


void CgiHandler::cmd_cfg_hyperion(const QStringList & args, QtHttpReply * reply)
{
	if ( args.at(0) == "cfg_hyperion" )
	{
		QFile file ( _hyperion->getConfigFileName().c_str() );
		if (file.exists ())
		{
			if (file.open (QFile::ReadOnly)) {
				QByteArray data = file.readAll ();
				reply->addHeader ("Content-Type", "text/plain");
				reply->appendRawData (data);
				file.close ();
			}
		}
		throw 0;
	}
}
