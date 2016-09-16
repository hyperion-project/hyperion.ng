#include "webconfig/WebConfig.h"
#include "StaticFileServing.h"

#include <QFileInfo>

WebConfig::WebConfig(QObject * parent)
	:  QObject(parent)
	, _hyperion(Hyperion::getInstance())
	, _server(nullptr)
{
	Logger* log = Logger::getInstance("WEBSERVER");
	_port       = WEBCONFIG_DEFAULT_PORT;
	_baseUrl    = WEBCONFIG_DEFAULT_PATH;
	const Json::Value &config = _hyperion->getJsonConfig();
	
	bool webconfigEnable = true; 

	if (config.isMember("webConfig"))
	{
		const Json::Value & webconfigConfig = config["webConfig"];
		webconfigEnable = webconfigConfig.get("enable", true).asBool();
		_port = webconfigConfig.get("port", _port).asUInt();
		_baseUrl = QString::fromStdString( webconfigConfig.get("document_root", _baseUrl.toStdString()).asString() );
	}

	if (_baseUrl != ":/webconfig")
	{
		QFileInfo info(_baseUrl);
		if (!info.exists() || !info.isDir())
		{
			Error(log, "document_root '%s' is invalid, set to default '%s'", _baseUrl.toUtf8().constData(), WEBCONFIG_DEFAULT_PATH.toUtf8().constData());
			_baseUrl = WEBCONFIG_DEFAULT_PATH;
		}
	}

	Debug(log, "WebUI initialized, document root: %s", _baseUrl.toUtf8().constData());
	if ( webconfigEnable )
	{
		start();
	}
}


WebConfig::~WebConfig()
{
	stop();
}


void WebConfig::start()
{
	if ( _server == nullptr )
		_server = new StaticFileServing (_hyperion, _baseUrl, _port, this);
}

void WebConfig::stop()
{
	if ( _server != nullptr )
	{
		delete _server;
		_server = nullptr;
	}
}


