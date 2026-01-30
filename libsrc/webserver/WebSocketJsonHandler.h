#ifndef WEBSOCKETJSONHANDLER_H
#define WEBSOCKETJSONHANDLER_H

#include <utils/Logger.h>
#include <api/JsonAPI.h>

#include <QObject>
#include <QWebSocket>
#include <QWeakPointer>
#include <QLoggingCategory>

#include <utils/NetOrigin.h>

Q_DECLARE_LOGGING_CATEGORY(comm_websocket_track);
Q_DECLARE_LOGGING_CATEGORY(comm_websocket_receive);
Q_DECLARE_LOGGING_CATEGORY(comm_websocket_send);

class WebSocketJsonHandler : public QObject
{
	Q_OBJECT

public:
	explicit WebSocketJsonHandler(QWebSocket* websocket, QObject* parent = nullptr);

	void close();

signals:
	void disconnected();

private slots:
	void onTextMessageReceived(const QString& message);
	void onBinaryMessageReceived(const QByteArray& message);
	void onDisconnected();
	qint64 sendMessage(QJsonObject obj);

private:
	QWebSocket* _websocket;

	QSharedPointer<Logger> _log;
	QSharedPointer<JsonAPI> _jsonAPI;
	QWeakPointer<NetOrigin> _netOriginWeak;
	QString _peerAddress;
	QString _origin;
};

#endif // WEBSOCKETJSONHANDLER_H
