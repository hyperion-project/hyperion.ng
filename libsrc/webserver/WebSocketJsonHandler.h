#ifndef WEBSOCKETJSONHANDLER_H
#define WEBSOCKETJSONHANDLER_H

#include <utils/Logger.h>
#include <api/JsonAPI.h>

#include <QObject>
#include <QWebSocket>
#include <QScopedPointer>

class WebSocketJsonHandler : public QObject
{
	Q_OBJECT

public:
	WebSocketJsonHandler(QWebSocket* websocket, QObject* parent = nullptr);

private slots:
	void onTextMessageReceived(const QString& message);
	void onBinaryMessageReceived(const QByteArray& message);
	void onDisconnected();
	qint64 sendMessage(QJsonObject obj);

private:
	QWebSocket* _websocket;

	Logger* _log;
	QScopedPointer<JsonAPI> _jsonAPI;
	QString _peerAddress;
	QString _origin;
};

#endif // WEBSOCKETJSONHANDLER_H
