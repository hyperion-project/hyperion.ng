#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <QObject>
#include <QString>
#include <string>
#include <utils/jsonschema/JsonFactory.h>

class StaticFileServing;

class WebConfig : public QObject {
	Q_OBJECT

public:
	WebConfig (std::string baseUrl, quint16 port, quint16 jsonPort, QObject * parent = NULL);
	WebConfig (const Json::Value &config, QObject * parent = NULL);

	virtual ~WebConfig (void);

	void start();
	void stop();

private:
	QObject*             _parent;
	QString              _baseUrl;
	quint16              _port;
	quint16              _jsonPort;
	StaticFileServing*   _server;

	const std::string    WEBCONFIG_DEFAULT_PATH = "/usr/share/hyperion/webconfig";
	const quint16        WEBCONFIG_DEFAULT_PORT = 8099;
};

#endif // WEBCONFIG_H

