#include "webconfig/webconfig.h"
#include "StaticFileServing.h"


WebConfig::WebConfig(std::string baseUrl, quint16 port, quint16 jsonPort, QObject * parent) :
	_parent(parent),
	_baseUrl(QString::fromStdString(baseUrl)),
	_port(port),
	_jsonPort(jsonPort),
	_server(nullptr)
{
}

WebConfig::WebConfig(const Json::Value &config, QObject * parent) :
	_parent(parent),
	_port(WEBCONFIG_DEFAULT_PORT),
	_server(nullptr)
{
	_baseUrl = QString::fromStdString(WEBCONFIG_DEFAULT_PATH);
	_jsonPort = 19444;
	bool webconfigEnable = true; 

	if (config.isMember("webConfig"))
	{
		const Json::Value & webconfigConfig = config["webConfig"];
		webconfigEnable = webconfigConfig.get("enable", true).asBool();
		_port = webconfigConfig.get("port", WEBCONFIG_DEFAULT_PORT).asUInt();
		_baseUrl = QString::fromStdString( webconfigConfig.get("document_root", WEBCONFIG_DEFAULT_PATH).asString() );
	}

	if (config.isMember("jsonServer"))
	{
		const Json::Value & jsonConfig = config["jsonServer"];
		_jsonPort = jsonConfig.get("port", 19444).asUInt();
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
		_server = new StaticFileServing (_baseUrl, _port, _jsonPort, this);
}

void WebConfig::stop()
{
	if ( _server != nullptr )
	{
		delete _server;
		_server = nullptr;
	}
}


