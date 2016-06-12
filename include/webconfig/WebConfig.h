#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <QObject>
#include <string>

class StaticFileServing;

class WebConfig : public QObject {
	Q_OBJECT

public:
	explicit WebConfig (std::string baseUrl, quint16 port, QObject * parent = NULL);
	virtual ~WebConfig (void);

	void start();
	void stop();

private:
	QObject            * _parent;
	QString              _baseUrl;
	quint16              _port;
	StaticFileServing  * _server;
};

#endif // WEBCONFIG_H

