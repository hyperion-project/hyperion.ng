#include <QStringBuilder>
#include <QUrlQuery>
#include <QFile>
#include <QByteArray>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QProcess>

#include "CgiHandler.h"
#include "QtHttpHeader.h"
#include <utils/FileUtils.h>
#include <utils/Process.h>
#include <utils/jsonschema/QJsonFactory.h>

CgiHandler::CgiHandler (QObject * parent)
	: QObject(parent)
	, _args(QStringList())
	, _baseUrl()
	, _log(Logger::getInstance("WEBSERVER"))
{
}

void CgiHandler::setBaseUrl(const QString& url)
{
	_baseUrl = url;
}

void CgiHandler::exec(const QStringList & args, QtHttpRequest * request, QtHttpReply * reply)
{
	_args = args;
	_request = request;
	_reply   = reply;

	if ( _args.at(0) == "cfg_jsonserver" )
	{
		cmd_cfg_jsonserver();
	}
	else if ( _args.at(0) == "run" )
	{
		cmd_runscript();
	}
	else
	{
		throw std::runtime_error("CGI command not found");
	}
}

void CgiHandler::cmd_cfg_jsonserver()
{
	quint16 jsonPort = 19444;
	// send result as reply
	_reply->addHeader ("Content-Type", "text/plain" );
	_reply->appendRawData (QByteArrayLiteral(":") % QString::number(jsonPort).toUtf8() );
}

void CgiHandler::cmd_runscript()
{
	QStringList scriptFilePathList(_args);
	scriptFilePathList.removeAt(0);

	QString scriptFilePath = scriptFilePathList.join('/');
	// relative path not allowed
	if ( scriptFilePath.indexOf("..") >=0 )
	{
		throw std::runtime_error("[cmd_runscript] Relative path not allowed : %s" + scriptFilePath.toStdString());
	}

	scriptFilePath = _baseUrl+"/server_scripts/"+scriptFilePath;

	if ( !QFile::exists(scriptFilePath) || !scriptFilePath.endsWith(".py") )
	{
		throw std::runtime_error("[cmd_runscript] Script %s doesn't exists or is no python file : " + scriptFilePath.toStdString());
	}

	QtHttpPostData postData = _request->getPostData();
	QByteArray inputData; // should  be filled with post data
	QByteArray data = Process::command_exec("python " + scriptFilePath, inputData);
	_reply->addHeader ("Content-Type", "text/plain");
	_reply->appendRawData (data);
}
