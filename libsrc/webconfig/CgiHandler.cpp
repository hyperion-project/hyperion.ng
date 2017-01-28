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

CgiHandler::CgiHandler (Hyperion * hyperion, QString baseUrl, QObject * parent)
	: QObject(parent)
	, _hyperion(hyperion)
	, _args(QStringList())
	, _hyperionConfig(_hyperion->getQJsonConfig())
	, _baseUrl(baseUrl)
	, _log(Logger::getInstance("WEBSERVER"))
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
		_args = args;
		_request = request;
		_reply   = reply;
		cmd_cfg_jsonserver();
		cmd_cfg_get();
		cmd_cfg_set();
		cmd_runscript();
		throw 1;
	}
	catch(int e)
	{
		if (e != 0)
			throw 1;
	}
}

void CgiHandler::cmd_cfg_jsonserver()
{
	if ( _args.at(0) == "cfg_jsonserver" )
	{
		quint16 jsonPort = 19444;
		if (_hyperionConfig.contains("jsonServer"))
		{
			const QJsonObject jsonConfig = _hyperionConfig["jsonServer"].toObject();
			jsonPort = jsonConfig["port"].toInt(jsonPort);
		}

		// send result as reply
		_reply->addHeader ("Content-Type", "text/plain" );
		_reply->appendRawData (QByteArrayLiteral(":") % QString::number(jsonPort).toUtf8() );

		throw 0;
	}
}


void CgiHandler::cmd_cfg_get()
{
	if ( _args.at(0) == "cfg_get" )
	{
		QFile file ( _hyperion->getConfigFileName().c_str() );
		if (file.exists ())
		{
			if (file.open (QFile::ReadOnly)) {
				QByteArray data = file.readAll ();
				_reply->addHeader ("Content-Type", "text/plain");
				_reply->appendRawData (data);
				file.close ();
			}
		}
		throw 0;
	}
}

void CgiHandler::cmd_cfg_set()
{
	_reply->addHeader ("Content-Type", "text/plain");
	if ( _args.at(0) == "cfg_set" )
	{
		QtHttpPostData data = _request->getPostData();
		QJsonParseError error;
		if (data.contains("cfg"))
		{
			QJsonDocument hyperionConfig = QJsonDocument::fromJson(QByteArray::fromPercentEncoding(data["cfg"]), &error);

			if (error.error == QJsonParseError::NoError)
			{
				QJsonObject hyperionConfigJsonObj = hyperionConfig.object();
				QJsonFactory::writeJson(QString::fromStdString(_hyperion->getConfigFileName()), hyperionConfigJsonObj);
			}
			else
			{
				//Debug(_log, "error while saving: %s", error.errorString()).toLocal8bit.constData());
				_reply->appendRawData (QString("Error while validating json: "+error.errorString()).toUtf8());
			}
		}

		throw 0;
	}
}

void CgiHandler::cmd_runscript()
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
			throw 1;
		}

		scriptFilePath = _baseUrl+"/server_scripts/"+scriptFilePath;

 		if (QFile::exists(scriptFilePath) && scriptFilePath.endsWith(".py") )
		{
			QtHttpPostData postData = _request->getPostData();
			QByteArray inputData; // should  be filled with post data
			QByteArray data = Process::command_exec("python " + scriptFilePath, inputData);
			_reply->addHeader ("Content-Type", "text/plain");
			_reply->appendRawData (data);
			throw 0;
		}
		else
		{
			Error( _log, "script %s doesn't exists or is no python file", scriptFilePath.toStdString().c_str());
		}

		throw 1;
	}
}
