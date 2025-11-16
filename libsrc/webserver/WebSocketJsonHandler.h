#ifndef WEBSOCKETJSONHANDLER_H
#define WEBSOCKETJSONHANDLER_H

#include <utils/Logger.h>
#include <api/JsonAPI.h>

#include <QObject>
#include <QWebSocket>
#include <QScopedPointer>
#include <QWeakPointer>

#include <utils/NetOrigin.h>

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

	QSharedPointer<Logger> _log;
	QScopedPointer<JsonAPI> _jsonAPI;
	QWeakPointer<NetOrigin> _netOriginWeak;
	QString _peerAddress;
	QString _origin;
};

#endif // WEBSOCKETJSONHANDLER_H
