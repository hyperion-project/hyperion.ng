#ifndef QTHTTPREQUEST_H
#define QTHTTPREQUEST_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QUrl>
#include <QHostAddress>
#include <QMap>

class QtHttpServer;
class QtHttpClientWrapper;

using QtHttpPostData = QMap<QString,QByteArray>;

class QtHttpRequest : public QObject
{
	Q_OBJECT

public:
	explicit QtHttpRequest (QtHttpClientWrapper * client, QtHttpServer * parent);

	struct ClientInfo
	{
		QHostAddress serverAddress;
		QHostAddress clientAddress;
	};

	int                   getRawDataSize (void) const { return m_data.size ();        };
	QUrl                  getUrl         (void) const { return m_url;                 };
	QString               getCommand     (void) const { return m_command;             };
	QByteArray            getRawData     (void) const { return m_data;                };
	QList<QByteArray>     getHeadersList (void) const { return m_headersHash.keys (); };
	QtHttpClientWrapper * getClient      (void) const { return m_clientHandle;        };
	QtHttpPostData        getPostData    (void) const { return m_postData;            };
	ClientInfo            getClientInfo  (void) const { return m_clientInfo;          };

	QByteArray            getHeader      (const QByteArray & header) const
	{
		return m_headersHash.value (header.toLower(), QByteArray ());
	};

public slots:
	void setUrl        (const QUrl & url)            { m_url = url;          };
	void setCommand    (const QString & command)     { m_command = command;  };
	void appendRawData (const QByteArray & data)     { m_data.append (data); };
	void setPostData   (const QtHttpPostData & data) { m_postData = data;    };

	void setClientInfo (const QHostAddress & server, const QHostAddress & client);
	void addHeader     (const QByteArray & header, const QByteArray & value);

private:
	QUrl                          m_url;
	QString                       m_command;
	QByteArray                    m_data;
	QtHttpServer *                m_serverHandle;
	QtHttpClientWrapper *         m_clientHandle;
	QHash<QByteArray, QByteArray> m_headersHash;
	ClientInfo                    m_clientInfo;
	QtHttpPostData                m_postData;
};

#endif // QTHTTPREQUEST_H
