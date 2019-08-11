#ifndef QTHTTPREPLY_H
#define QTHTTPREPLY_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QList>

class QtHttpServer;

class QtHttpReply : public QObject
{
	Q_OBJECT
	Q_ENUMS (StatusCode)

public:
	explicit QtHttpReply (QtHttpServer * parent);

	enum StatusCode
	{
		Ok                 = 200,
		SeeOther           = 303,
		BadRequest         = 400,
		Forbidden          = 403,
		NotFound           = 404,
		MethodNotAllowed   = 405,
		InternalError      = 500,
		NotImplemented     = 501,
		BadGateway         = 502,
		ServiceUnavailable = 503,
	};

	int               getRawDataSize (void) const { return m_data.size();         };
	bool              useChunked     (void) const { return m_useChunked;          };
	StatusCode        getStatusCode  (void) const { return m_statusCode;          };
	QByteArray        getRawData     (void) const { return m_data;                };
	QList<QByteArray> getHeadersList (void) const { return m_headersHash.keys (); };

	QByteArray getHeader (const QByteArray & header) const
	{
		return m_headersHash.value (header, QByteArray ());
	};

	static const QByteArray getStatusTextForCode (StatusCode statusCode);

public slots:
	void setUseChunked (bool chunked = false)    { m_useChunked = chunked;    };
	void setStatusCode (StatusCode statusCode)   { m_statusCode = statusCode; };
	void appendRawData (const QByteArray & data) { m_data.append(data);       };
	void addHeader     (const QByteArray & header, const QByteArray & value);
	void resetRawData  (void) { m_data.clear (); };

signals:
	void requestSendHeaders (void);
	void requestSendData    (void);

private:
	bool                          m_useChunked;
	StatusCode                    m_statusCode;
	QByteArray                    m_data;
	QtHttpServer *                m_serverHandle;
	QHash<QByteArray, QByteArray> m_headersHash;
};

#endif // QTHTTPREPLY_H
