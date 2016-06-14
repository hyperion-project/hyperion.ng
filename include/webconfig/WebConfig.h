#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <QObject>
#include <QString>
#include <string>
#include <utils/jsonschema/JsonFactory.h>
#include <hyperion/Hyperion.h>

class StaticFileServing;

class WebConfig : public QObject {
	Q_OBJECT

public:
	WebConfig (Hyperion *hyperion, QObject * parent = NULL);

	virtual ~WebConfig (void);

	void start();
	void stop();

private:
	Hyperion*            _hyperion;
	QString              _baseUrl;
	quint16              _port;
	StaticFileServing*   _server;

	const std::string    WEBCONFIG_DEFAULT_PATH = "/usr/share/hyperion/webconfig";
	const quint16        WEBCONFIG_DEFAULT_PORT = 8099;
};

#endif // WEBCONFIG_H

