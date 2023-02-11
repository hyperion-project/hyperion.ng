#ifndef PROVIDERRESTKAPI_H
#define PROVIDERRESTKAPI_H

// Local-Hyperion includes
#include <utils/Logger.h>

// Qt includes
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>

#include <QBasicTimer>
#include <QTimerEvent>

#include <chrono>

constexpr std::chrono::milliseconds DEFAULT_REST_TIMEOUT{ 1000 };

//Set QNetworkReply timeout without external timer
//https://stackoverflow.com/questions/37444539/how-to-set-qnetworkreply-timeout-without-external-timer

class ReplyTimeout : public QObject
{
	Q_OBJECT

public:
	enum HandleMethod { Abort, Close };

	ReplyTimeout(QNetworkReply* reply, const int timeout, HandleMethod method = Abort) :
		  QObject(reply), m_method(method), m_timedout(false)
	{
		Q_ASSERT(reply);
		if (reply && reply->isRunning()) {
			m_timer.start(timeout, this);
			connect(reply, &QNetworkReply::finished, this, &QObject::deleteLater);
		}
	}

	bool isTimedout() const
	{
		return m_timedout;
	}

	static ReplyTimeout * set(QNetworkReply* reply, const int timeout, HandleMethod method = Abort)
	{
		return new ReplyTimeout(reply, timeout, method);
	}

signals:
	void timedout();

protected:

	void timerEvent(QTimerEvent * ev) override {
		if (!m_timer.isActive() || ev->timerId() != m_timer.timerId())
			return;
		auto reply = static_cast<QNetworkReply*>(parent());
		if (reply->isRunning())
		{
			m_timedout = true;
			emit timedout();
			if (m_method == Close)
				reply->close();
			else if (m_method == Abort)
				reply->abort();
			m_timer.stop();
		}
	}

	QBasicTimer m_timer;
	HandleMethod m_method;
	bool m_timedout;
};

///
/// Response object for REST-API calls and JSON-responses
///
class httpResponse
{
public:
	httpResponse() = default;

	bool error() const { return _hasError; }
	void setError(const bool hasError) { _hasError = hasError; }

	QJsonDocument getBody() const { return _responseBody; }
	void setBody(const QJsonDocument& body) { _responseBody = body; }

	QString getErrorReason() const { return _errorReason; }
	void setErrorReason(const QString& errorReason) { _errorReason = errorReason; }

	int getHttpStatusCode() const { return _httpStatusCode; }
	void setHttpStatusCode(int httpStatusCode) { _httpStatusCode = httpStatusCode; }

	QNetworkReply::NetworkError getNetworkReplyError() const { return _networkReplyError; }
	void setNetworkReplyError(const QNetworkReply::NetworkError networkReplyError) { _networkReplyError = networkReplyError; }

private:

	QJsonDocument _responseBody {};
	bool _hasError = false;
	QString _errorReason;

	int _httpStatusCode = 0;
	QNetworkReply::NetworkError _networkReplyError { QNetworkReply::NoError };
};

///
/// Wrapper class supporting REST-API calls with JSON requests and responses
///
/// Usage sample:
/// @code
///
/// ProviderRestApi* _restApi = new ProviderRestApi(hostname, port );
///
/// _restApi->setBasePath( QString("/api/%1/").arg(token) );
/// _restApi->setPath( QString("%1/%2").arg( "groups" ).arg( groupId ) );
///
/// httpResponse response = _restApi->get();
/// if ( !response.error() )
///		response.getBody();
///
/// delete _restApi;
///
///@endcode
///
class ProviderRestApi : public QObject
{
	Q_OBJECT

public:

	/// @brief Constructor of the REST-API wrapper
	///
	ProviderRestApi();

	///
	/// @brief Constructor of the REST-API wrapper
	///
	/// @param[in] host
	/// @param[in] port
	///
	explicit ProviderRestApi(const QString& host, int port);

	///
	/// @brief Constructor of the REST-API wrapper
	///
	/// @param[in] scheme
	/// @param[in] host
	/// @param[in] port
	///
	explicit ProviderRestApi(const QString& scheme, const QString& host, int port);

	///
	/// @brief Constructor of the REST-API wrapper
	///
	/// @param[in] host
	/// @param[in] port
	/// @param[in] API base-path
	///
	explicit ProviderRestApi(const QString& host, int port, const QString& basePath);

	///
	/// @brief Constructor of the REST-API wrapper
	///
	/// @param[in] scheme
	/// @param[in] host
	/// @param[in] port
	/// @param[in] API base-path
	///
	explicit ProviderRestApi(const QString& scheme, const QString& host, int port, const QString& basePath);

	///
	/// @brief Destructor of the REST-API wrapper
	///
	virtual ~ProviderRestApi() override;

	///
	/// @brief Set an API's host
	///
	/// @param[in] host
	///
	void setHost(const QString& host) { _apiUrl.setHost(host); }

	///
	/// @brief Set an API's port
	///
	/// @param[in] port
	///
	void setPort(const int port) { _apiUrl.setPort(port); }

	///
	/// @brief Set an API's url
	///
	/// @param[in] url, e.g. "http://locahost:60351/chromalink/"
	///
	void setUrl(const QUrl& url);

	///
	/// @brief Get the URL as defined using scheme, host, port, API-basepath, path, query, fragment
	///
	/// @return url
	///
	QUrl getUrl() const;

	///
	/// @brief Set an API's base path (the stable path element before addressing resources)
	///
	/// @param[in] basePath, e.g. "/api/v1/" or "/json"
	///
	void setBasePath(const QString& basePath);

	///
	/// @brief Set an API's path to address resources
	///
	/// @param[in] path, e.g. "/lights/1/state/"
	///
	void setPath(const QString& path);

	/// @brief Set an API's path to address resources
	///
	/// @param[in] pathElements to form a path, e.g. (lights,1,state) results in "/lights/1/state/"
	///
	void setPath(const QStringList& pathElements);

	///
	/// @brief Append an API's path element to path set before
	///
	/// @param[in] path
	///
	void appendPath(const QString& appendPath);

	///
	/// @brief Append API's path elements to path set before
	///
	/// @param[in] pathElements
	///
	void appendPath(const QStringList& pathElements);

	///
	/// @brief Set an API's fragment
	///
	/// @param[in] fragment, e.g. "question3"
	///
	void setFragment(const QString& fragment);

	///
	/// @brief Set an API's query string
	///
	/// @param[in] query, e.g. "&A=128&FX=0"
	///
	void setQuery(const QUrlQuery& query);

	///
	/// @brief Execute GET request
	///
	/// @return Response The body of the response in JSON
	///
	httpResponse get();

	///
	/// @brief Execute GET request
	///
	/// @param[in] url GET request for URL
	/// @return Response The body of the response in JSON
	///
	httpResponse get(const QUrl& url);

	/// @brief Execute PUT request
	///
	/// @param[in] body The body of the request in JSON
	/// @return Response The body of the response in JSON
	///
	httpResponse put(const QJsonObject& body);

	///
	/// @brief Execute PUT request
	///
	/// @param[in] body The body of the request in JSON
	/// @return Response The body of the response in JSON
	///
	httpResponse put(const QString& body = "");

	///
	/// @brief Execute PUT request
	///
	/// @param[in] URL for PUT request
	/// @param[in] body The body of the request in JSON
	/// @return Response The body of the response in JSON
	///
	httpResponse put(const QUrl &url, const QByteArray& body);

	///
	/// @brief Execute POST request
	///
	/// @param[in] body The body of the request in JSON
	/// @return Response The body of the response in JSON
	///
	httpResponse post(const QString& body = "");

	/// @brief Execute POST request
	///
	/// @param[in] body The body of the request in JSON
	/// @return Response The body of the response in JSON
	///
	httpResponse post(const QJsonObject& body);

	///
	/// @brief Execute POST request
	///
	/// @param[in] URL for POST request
	/// @param[in] body The body of the request in JSON
	/// @return Response The body of the response in JSON
	///
	httpResponse post(const QUrl &url, const QByteArray& body);

	///
	/// @brief Execute DELETE request
	///
	/// @param[in] URL (Resource) for DELETE request
	/// @return Response The body of the response in JSON
	///
	httpResponse deleteResource(const QUrl& url);

	///
	/// @brief Handle responses for REST requests
	///
	/// @param[in] reply Network reply
	/// @return Response The body of the response in JSON
	///
	httpResponse getResponse(QNetworkReply* const& reply);

	///
	/// Adds a header field.
	///
	/// @param[in] The type of the header field.
	/// @param[in] The value of the header field.
	/// If the header field exists, the value will be combined as comma separated string.
	void setHeader(QNetworkRequest::KnownHeaders header, const QVariant& value);

	///
	/// Set a header field.
	///
	/// @param[in] The type of the header field.
	/// @param[in] The value of the header field.
	/// If the header field exists, the value will override the previous setting.
	void setHeader(const QByteArray &headerName, const QByteArray &headerValue);

	///
	/// Remove all header fields.
	///
	void removeAllHeaders() { _networkRequestHeaders = QNetworkRequest(); }

	///
	/// Sets the timeout time frame after a request is aborted
	/// Zero means no timer is set.
	///
	/// @param[in] timeout in milliseconds.
	void setTransferTimeout(std::chrono::milliseconds timeout = DEFAULT_REST_TIMEOUT) { _requestTimeout = timeout; }

	///
	/// @brief Set the common logger for LED-devices.
	///
	/// @param[in] log The logger to be used
	///
	void setLogger(Logger* log) { _log = log; }

private:

	///
	/// @brief Append an API's path element to path given as param
	///
	/// @param[in/out] path to be updated
	/// @param[in] path, element to be appended
	///
	static void appendPath (QString &path, const QString &appendPath) ;


	httpResponse executeOperation(QNetworkAccessManager::Operation op, const QUrl& url, const QByteArray& body = {});

	Logger* _log;

	// QNetworkAccessManager object for sending REST-requests.
	QNetworkAccessManager* _networkManager;
	std::chrono::milliseconds _requestTimeout;

	QUrl _apiUrl;

	QString _basePath;
	QString _path;

	QString _fragment;
	QUrlQuery _query;

	QNetworkRequest _networkRequestHeaders;
};

#endif // PROVIDERRESTKAPI_H
