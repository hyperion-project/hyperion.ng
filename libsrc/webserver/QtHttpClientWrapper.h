#ifndef QTHTTPCLIENTWRAPPER_H
#define QTHTTPCLIENTWRAPPER_H

#include <QObject>
#include <QString>

class QTcpSocket;

class QtHttpRequest;
class QtHttpReply;
class QtHttpServer;
class WebSocketClient;
class WebJsonRpc;

class QtHttpClientWrapper : public QObject {
	Q_OBJECT

public:
	explicit QtHttpClientWrapper (QTcpSocket * sock, const bool& localConnection, QtHttpServer * parent);

	static const char SPACE = ' ';
	static const char COLON = ':';
	static const QByteArray & CRLF;

	enum ParsingStatus {
		ParsingError    = -1,
		AwaitingRequest =  0,
		AwaitingHeaders =  1,
		AwaitingContent =  2,
		RequestParsed   =  3
	};

	QString getGuid (void);
	/// @brief Wrapper for sendReplyToClient(), handles m_parsingStatus and signal connect
	void sendToClientWithReply (QtHttpReply * reply);

	///
	/// @brief close a connection with FORBIDDEN header (used from JsonAPI over HTTP)
	///
	void closeConnection();

private slots:
	void onClientDataReceived (void);

protected:
	ParsingStatus sendReplyToClient (QtHttpReply * reply);

protected slots:
	void onReplySendHeadersRequested (void);
	void onReplySendDataRequested    (void);

private:
	QString           m_guid;
	ParsingStatus     m_parsingStatus;
	QTcpSocket    *   m_sockClient;
	QtHttpRequest *   m_currentRequest;
	QtHttpServer  *   m_serverHandle;
	const bool        m_localConnection;
	WebSocketClient * m_websocketClient;
	WebJsonRpc *      m_webJsonRpc;
};

#endif // QTHTTPCLIENTWRAPPER_H
