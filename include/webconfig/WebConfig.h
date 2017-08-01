#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <QObject>
#include <QString>
#include <hyperion/Hyperion.h>

class StaticFileServing;

class WebConfig : public QObject {
	Q_OBJECT

public:
	WebConfig (QObject * parent = NULL);

	virtual ~WebConfig (void);

	void start();
	void stop();

	quint16 getPort() { return _port; };

private:
	Hyperion*            _hyperion;
	QString              _baseUrl;
	quint16              _port;
	StaticFileServing*   _server;

	const QString        WEBCONFIG_DEFAULT_PATH = ":/webconfig";
	const quint16        WEBCONFIG_DEFAULT_PORT = 8099;
};

#endif // WEBCONFIG_H

