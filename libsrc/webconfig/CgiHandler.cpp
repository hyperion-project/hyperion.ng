#include <QStringBuilder>
#include <QUrlQuery>
#include <QFile>
#include <QByteArray>

#include "CgiHandler.h"
#include "QtHttpHeader.h"
#include <utils/FileUtils.h>
#include <utils/Process.h>

CgiHandler::CgiHandler (Hyperion * hyperion, QString baseUrl, QObject * parent)
	: QObject(parent)
	, _hyperion(hyperion)
	, _hyperionConfig(_hyperion->getQJsonConfig())
	, _baseUrl(baseUrl)
{
}

CgiHandler::~CgiHandler()
{
}

void CgiHandler::exec(const QStringList & args, QtHttpRequest * request, QtHttpReply * reply)
{
	try
	{
// 		QByteArray header = reply->getHeader(QtHttpHeader::Host);
// 		QtHttpRequest::ClientInfo info = request->getClientInfo();
		
		cmd_cfg_jsonserver(args,reply);
		cmd_cfg_hyperion(args,reply);
		cmd_runscript(args,reply);
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
		if (_hyperionConfig.contains("jsonServer"))
		{
			const QJsonObject jsonConfig = _hyperionConfig["jsonServer"].toObject();
			jsonPort = jsonConfig["port"].toInt(jsonPort);
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

void CgiHandler::cmd_runscript(const QStringList & args, QtHttpReply * reply)
{
	if ( args.at(0) == "run" )
	{
		QStringList scriptFilePathList(args);
		scriptFilePathList.removeAt(0);
		
		QString scriptFilePath = scriptFilePathList.join('/');
		// relative path not allowed
		if (scriptFilePath.indexOf("..") >=0)
		{
			throw 1;
		}

		scriptFilePath = _baseUrl+"/server_scripts/"+scriptFilePath;
		QString interpreter = "";
		if (scriptFilePath.endsWith(".sh")) interpreter = "sh";
		if (scriptFilePath.endsWith(".py")) interpreter = "python";
			
 		if (QFile::exists(scriptFilePath) && !interpreter.isEmpty())
		{
			QByteArray data = Process::command_exec(QString(interpreter + " " + scriptFilePath).toUtf8().constData()).c_str();
			
			reply->addHeader ("Content-Type", "text/plain");
			reply->appendRawData (data);
			throw 0;
		}
		throw 1;
	}
}
