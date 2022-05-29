// Local-Hyperion includes
#include "ProviderRestApi.h"

// Qt includes
#include <QEventLoop>
#include <QNetworkReply>
#include <QByteArray>
#include <QJsonObject>

//std includes
#include <iostream>
#include <chrono>

// Constants
namespace {

const QChar ONE_SLASH = '/';

const int HTTP_STATUS_NO_CONTENT = 204;
const int HTTP_STATUS_BAD_REQUEST = 400;
const int HTTP_STATUS_UNAUTHORIZED = 401;
const int HTTP_STATUS_NOT_FOUND = 404;

constexpr std::chrono::milliseconds DEFAULT_REST_TIMEOUT{ 400 };

} //End of constants

ProviderRestApi::ProviderRestApi(const QString& host, int port, const QString& basePath)
	:_log(Logger::getInstance("LEDDEVICE"))
	  , _networkManager(nullptr)
{
	_networkManager = new QNetworkAccessManager();

	_apiUrl.setScheme("http");
	_apiUrl.setHost(host);
	_apiUrl.setPort(port);
	_basePath = basePath;
}

ProviderRestApi::ProviderRestApi(const QString& host, int port)
	: ProviderRestApi(host, port, "") {}

ProviderRestApi::ProviderRestApi()
	: ProviderRestApi("", -1) {}

ProviderRestApi::~ProviderRestApi()
{
	delete _networkManager;
}

void ProviderRestApi::setUrl(const QUrl& url)
{
	_apiUrl = url;
	_basePath = url.path();
}

void ProviderRestApi::setBasePath(const QString& basePath)
{
	_basePath.clear();
	appendPath(_basePath, basePath);
}

void ProviderRestApi::setPath(const QString& path)
{
	_path.clear();
	appendPath(_path, path);
}

void ProviderRestApi::appendPath(const QString& path)
{
	appendPath(_path, path);
}

void ProviderRestApi::appendPath ( QString& path, const QString &appendPath)
{
	if (!appendPath.isEmpty() && appendPath != ONE_SLASH)
	{
		if (path.isEmpty() || path == ONE_SLASH)
		{
			path.clear();
			if (appendPath[0] != ONE_SLASH)
			{
				path.push_back(ONE_SLASH);
			}
		}
		else if (path[path.size() - 1] == ONE_SLASH && appendPath[0] == ONE_SLASH)
		{
			path.chop(1);
		}
		else if (path[path.size() - 1] != ONE_SLASH && appendPath[0] != ONE_SLASH)
		{
			path.push_back(ONE_SLASH);
		}
		else
		{
			// Only one slash.
		}

		path.append(appendPath);
	}
}

void ProviderRestApi::setFragment(const QString& fragment)
{
	_fragment = fragment;
}

void ProviderRestApi::setQuery(const QUrlQuery& query)
{
	_query = query;
}

QUrl ProviderRestApi::getUrl() const
{
	QUrl url = _apiUrl;

	QString fullPath = _basePath;
	appendPath(fullPath, _path);

	url.setPath(fullPath);
	url.setFragment(_fragment);
	url.setQuery(_query);
	return url;
}

httpResponse ProviderRestApi::get()
{
	return get(getUrl());
}

httpResponse ProviderRestApi::get(const QUrl& url)
{
	// Perform request
	QNetworkRequest request(_networkRequestHeaders);
	request.setUrl(url);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	_networkManager->setTransferTimeout(DEFAULT_REST_TIMEOUT.count());
#endif

	QNetworkReply* reply = _networkManager->get(request);

	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	QEventLoop::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
	ReplyTimeout::set(reply, DEFAULT_REST_TIMEOUT.count());
#endif

	// Go into the loop until the request is finished.
	loop.exec();

	httpResponse response;
	if (reply->operation() == QNetworkAccessManager::GetOperation)
	{
		if(reply->error() != QNetworkReply::NoError)
		{
			Debug(_log, "GET: [%s]", QSTRING_CSTR( url.toString() ));
		}
		response = getResponse(reply );
	}
	// Free space.
	reply->deleteLater();
	// Return response
	return response;
}

httpResponse ProviderRestApi::put(const QJsonObject &body)
{
	return put( getUrl(), QJsonDocument(body).toJson(QJsonDocument::Compact));
}

httpResponse ProviderRestApi::put(const QString &body)
{
	return put( getUrl(), body.toUtf8() );
}

httpResponse ProviderRestApi::put(const QUrl &url, const QByteArray &body)
{
	// Perform request
	QNetworkRequest request(_networkRequestHeaders);
	request.setUrl(url);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	_networkManager->setTransferTimeout(DEFAULT_REST_TIMEOUT.count());
#endif

	QNetworkReply* reply = _networkManager->put(request, body);
	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	QEventLoop::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
	ReplyTimeout::set(reply, DEFAULT_REST_TIMEOUT.count());
#endif

	// Go into the loop until the request is finished.
	loop.exec();

	httpResponse response;
	if (reply->operation() == QNetworkAccessManager::PutOperation)
	{
		if(reply->error() != QNetworkReply::NoError)
		{
			Debug(_log, "PUT: [%s] [%s]", QSTRING_CSTR( url.toString() ),body.constData() );
		}
		response = getResponse(reply);
	}
	// Free space.
	reply->deleteLater();

	// Return response
	return response;
}

httpResponse ProviderRestApi::post(const QJsonObject& body)
{
	return post( getUrl(), QJsonDocument(body).toJson(QJsonDocument::Compact));
}

httpResponse ProviderRestApi::post(const QString& body)
{
	return post( getUrl(), body.toUtf8() );
}

httpResponse ProviderRestApi::post(const QUrl& url, const QByteArray& body)
{
	// Perform request
	QNetworkRequest request(_networkRequestHeaders);
	request.setUrl(url);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	_networkManager->setTransferTimeout(DEFAULT_REST_TIMEOUT.count());
#endif

	QNetworkReply* reply = _networkManager->post(request, body);
	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	QEventLoop::connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);

#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
	ReplyTimeout::set(reply, DEFAULT_REST_TIMEOUT.count());
#endif

	// Go into the loop until the request is finished.
	loop.exec();

	httpResponse response;
	if (reply->operation() == QNetworkAccessManager::PostOperation)
	{
		if(reply->error() != QNetworkReply::NoError)
		{
			Debug(_log, "POST: [%s] [%s]", QSTRING_CSTR( url.toString() ),body.constData() );
		}
		response = getResponse(reply);
	}
	// Free space.
	reply->deleteLater();

	// Return response
	return response;
}

httpResponse ProviderRestApi::deleteResource(const QUrl& url)
{
	// Perform request
	QNetworkRequest request(_networkRequestHeaders);
	request.setUrl(url);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	_networkManager->setTransferTimeout(DEFAULT_REST_TIMEOUT.count());
#endif

	QNetworkReply* reply = _networkManager->deleteResource(request);
	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	QEventLoop::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	// Go into the loop until the request is finished.
	loop.exec();

#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
	ReplyTimeout::set(reply, DEFAULT_REST_TIMEOUT.count());
#endif

	httpResponse response;
	if (reply->operation() == QNetworkAccessManager::DeleteOperation)
	{
		if(reply->error() != QNetworkReply::NoError)
		{
			Debug(_log, "DELETE: [%s]", QSTRING_CSTR(url.toString()));
		}
		response = getResponse(reply);
	}
	// Free space.
	reply->deleteLater();

	// Return response
	return response;
}

httpResponse ProviderRestApi::getResponse(QNetworkReply* const& reply)
{
	httpResponse response;

	int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	response.setHttpStatusCode(httpStatusCode);
	response.setNetworkReplyError(reply->error());

	if (reply->error() == QNetworkReply::NoError)
	{
		if ( httpStatusCode != HTTP_STATUS_NO_CONTENT ){
			QByteArray replyData = reply->readAll();

			if (!replyData.isEmpty())
			{
				QJsonParseError error;
				QJsonDocument jsonDoc = QJsonDocument::fromJson(replyData, &error);

				if (error.error != QJsonParseError::NoError)
				{
					//Received not valid JSON response
					//std::cout << "Response: [" << replyData.toStdString() << "]" << std::endl;
					response.setError(true);
					response.setErrorReason(error.errorString());
				}
				else
				{
					//std::cout << "Response: [" << QString(jsonDoc.toJson(QJsonDocument::Compact)).toStdString() << "]" << std::endl;
					response.setBody(jsonDoc);
				}
			}
			else
			{	// Create valid body which is empty
				response.setBody(QJsonDocument());
			}
		}
	}
	else
	{
		Debug(_log, "Reply.httpStatusCode [%d]", httpStatusCode );
		QString errorReason;
		if (httpStatusCode > 0) {
			QString httpReason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
			QString advise;
			switch ( httpStatusCode ) {
			case HTTP_STATUS_BAD_REQUEST:
				advise = "Check Request Body";
				break;
			case HTTP_STATUS_UNAUTHORIZED:
				advise = "Check Authentication Token (API Key)";
				break;
			case HTTP_STATUS_NOT_FOUND:
				advise = "Check Resource given";
				break;
			default:
				break;
			}
			errorReason = QString ("[%3 %4] - %5").arg(httpStatusCode).arg(httpReason, advise);
		}
		else
		{
			errorReason = reply->errorString();
			{
				response.setError(true);
				response.setErrorReason(errorReason);
			}
		}

		// Create valid body which is empty
		response.setBody(QJsonDocument());
	}
	return response;
}

void ProviderRestApi::setHeader(QNetworkRequest::KnownHeaders header, const QVariant& value)
{
	QVariant headerValue = _networkRequestHeaders.header(header);
	if (headerValue.isNull())
	{
		_networkRequestHeaders.setHeader(header, value);
	}
	else
	{
		if (!headerValue.toString().contains(value.toString()))
		{
			_networkRequestHeaders.setHeader(header, headerValue.toString() + "," + value.toString());
		}
	}
}
