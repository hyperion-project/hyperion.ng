#include "webconfig/WebConfig.h"
#include "StaticFileServing.h"


WebConfig::WebConfig(QObject * parent)
	:  QObject(parent)
	, _server(nullptr)
{
	_port = WEBCONFIG_DEFAULT_PORT;
	_hyperion = Hyperion::getInstance();
	const Json::Value &config = _hyperion->getJsonConfig();
	_baseUrl = QString::fromStdString(WEBCONFIG_DEFAULT_PATH);
	_port = WEBCONFIG_DEFAULT_PORT;

	bool webconfigEnable = true; 

	if (config.isMember("webConfig"))
	{
		const Json::Value & webconfigConfig = config["webConfig"];
		webconfigEnable = webconfigConfig.get("enable", true).asBool();
		_port = webconfigConfig.get("port", WEBCONFIG_DEFAULT_PORT).asUInt();
		_baseUrl = QString::fromStdString( webconfigConfig.get("document_root", WEBCONFIG_DEFAULT_PATH).asString() );
	}

	if ( webconfigEnable )
		start();
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


