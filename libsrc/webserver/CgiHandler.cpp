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

CgiHandler::~CgiHandler()
{
}

void CgiHandler::setBaseUrl(const QString& url)
{
	_baseUrl = url;
}

void CgiHandler::exec(const QStringList & args, QtHttpRequest * request, QtHttpReply * reply)
{
// 	QByteArray header = reply->getHeader(QtHttpHeader::Host);
// 	QtHttpRequest::ClientInfo info = request->getClientInfo();
	_args = args;
	_request = request;
	_reply   = reply;

	if ( _args.at(0) == "cfg_jsonserver" )
	{
		if ( cmd_cfg_jsonserver() )
		{
			return;
		}
	}
	else if ( _args.at(0) == "run" )
	{
		if ( cmd_runscript() )
		{
			return;
		}
	}
	throw std::runtime_error("CGI execution failed");
}

bool CgiHandler::cmd_cfg_jsonserver()
{
	if ( _args.at(0) == "cfg_jsonserver" )
	{
		quint16 jsonPort = 19444;
		// send result as reply
		_reply->addHeader ("Content-Type", "text/plain" );
		_reply->appendRawData (QByteArrayLiteral(":") % QString::number(jsonPort).toUtf8() );

		return true;
	}
	return false;
}

bool CgiHandler::cmd_runscript()
{
	if ( _args.at(0) == "run" )
	{
		QStringList scriptFilePathList(_args);
		scriptFilePathList.removeAt(0);

		QString scriptFilePath = scriptFilePathList.join('/');
		// relative path not allowed
		if (scriptFilePath.indexOf("..") >=0)
		{
			Error( _log, "relative path not allowed (%s)", scriptFilePath.toStdString().c_str());
			return false;
		}

		scriptFilePath = _baseUrl+"/server_scripts/"+scriptFilePath;

 		if (QFile::exists(scriptFilePath) && scriptFilePath.endsWith(".py") )
		{
			QtHttpPostData postData = _request->getPostData();
			QByteArray inputData; // should  be filled with post data
			QByteArray data = Process::command_exec("python " + scriptFilePath, inputData);
			_reply->addHeader ("Content-Type", "text/plain");
			_reply->appendRawData (data);
			return true;
		}
		else
		{
			Error( _log, "script %s doesn't exists or is no python file", scriptFilePath.toStdString().c_str());
		}

		return false;
	}
}
