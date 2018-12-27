#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <QString>
#include <QJsonDocument>

// hyperion / utils
#include <hyperion/Hyperion.h>
#include <utils/Logger.h>

// settings
#include <utils/settings.h>

class StaticFileServing;
class QtHttpServer;

class WebServer : public QObject {
	Q_OBJECT

public:
	WebServer (const QJsonDocument& config, QObject * parent = 0);

	virtual ~WebServer (void);

	void start();
	void stop();

	quint16 getPort() { return _port; };

public slots:
	void onServerStopped      (void);
	void onServerStarted      (quint16 port);
	void onServerError        (QString msg);

	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(const settings::type& type, const QJsonDocument& config);

private:
	Logger*              _log;
	Hyperion*            _hyperion;
	QString              _baseUrl;
	quint16              _port;
	StaticFileServing*   _staticFileServing;
	QtHttpServer*        _server;

	const QString        WEBSERVER_DEFAULT_PATH = ":/webconfig";
	const quint16        WEBSERVER_DEFAULT_PORT = 8090;
};

#endif // WEBSERVER_H
