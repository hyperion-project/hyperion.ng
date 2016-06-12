#include "webconfig/webconfig.h"
#include "StaticFileServing.h"

WebConfig::WebConfig(std::string baseUrl, quint16 port, QObject * parent) :
	_parent(parent),
	_baseUrl(QString::fromStdString(baseUrl)),
	_port(port),
	_server(nullptr)
{
}

WebConfig::~WebConfig()
{
	stop();
}


void WebConfig::start()
{
	if ( _server == nullptr )
		_server = new StaticFileServing (_baseUrl, _port, this);
}

void WebConfig::stop()
{
	if ( _server != nullptr )
	{
		delete _server;
		_server = nullptr;
	}
}


