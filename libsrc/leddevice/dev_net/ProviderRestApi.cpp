// Local-Hyperion includes
#include "ProviderRestApi.h"

// Qt includes
#include <QEventLoop>
#include <QNetworkReply>
#include <QByteArray>

//std includes
#include <iostream>

static const char ONE_SLASH = '/';

ProviderRestApi::ProviderRestApi(const QString &host, const int &port, const QString &basePath)
	:_log(Logger::getInstance("LDEDEVICE"))
	  ,_networkManager(nullptr)
	  ,_scheme("http")
	  ,_hostname(host)
	  ,_port(port)
{
	_networkManager = new QNetworkAccessManager();

	_apiUrl.setScheme(_scheme);
	_apiUrl.setHost(host);
	_apiUrl.setPort(port);
	_basePath = basePath;
}

ProviderRestApi::ProviderRestApi(const QString &host, const int &port)
	: ProviderRestApi(host, port, "") {}

ProviderRestApi::ProviderRestApi()
	: ProviderRestApi("", -1) {}

ProviderRestApi::~ProviderRestApi()
{
	if ( _networkManager != nullptr )
	{
		_networkManager->deleteLater();
	}
}

void ProviderRestApi::setBasePath(const QString &basePath)
{
	_basePath.clear();
	appendPath (_basePath, basePath );
}

void ProviderRestApi::setPath ( const QString &path )
{
	_path.clear();
	appendPath (_path, path );
}

void ProviderRestApi::appendPath ( const QString &path )
{
	appendPath (_path, path );
}

void ProviderRestApi::appendPath (QString& path, const QString &appendPath)
{
	if ( !appendPath.isEmpty() && appendPath != ONE_SLASH )
	{
		if (path.isEmpty() || path == QString(ONE_SLASH))
		{
			path.clear();
			if (appendPath.front() != ONE_SLASH )
			{
				path.push_back(ONE_SLASH);
			}
		}
		else if (path.back() == ONE_SLASH && appendPath.front() == ONE_SLASH)
		{
			path.chop(1);
		}
		else if (path.back() != ONE_SLASH && appendPath.front() != ONE_SLASH)
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

void ProviderRestApi::setFragment(const QString &fragment)
{
	_fragment = fragment;
}

void ProviderRestApi::setQuery(const QUrlQuery &query)
{
	_query = query;
}

QUrl ProviderRestApi::getUrl()
{
	QUrl url = _apiUrl;

	QString fullPath = _basePath;
	appendPath (fullPath, _path );

	url.setPath(fullPath);
	url.setFragment( _fragment );
	url.setQuery( _query );
	return url;
}

httpResponse ProviderRestApi::get()
{
	return get( getUrl() );
}

httpResponse ProviderRestApi::get(const QUrl &url)
{
	Debug(_log, "GET: [%s]", QSTRING_CSTR( url.toString() ));

	// Perform request
	QNetworkRequest request(url);
	QNetworkReply* reply = _networkManager->get(request);
	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	loop.connect(reply, SIGNAL(finished()), SLOT(quit()));
	// Go into the loop until the request is finished.
	loop.exec();

	httpResponse response;
	if(reply->operation() == QNetworkAccessManager::GetOperation)
	{
		response = getResponse(reply );
	}
	// Free space.
	reply->deleteLater();
	// Return response
	return response;
}

httpResponse ProviderRestApi::put(const QString &body)
{
	return put( getUrl(), body );
}

httpResponse ProviderRestApi::put(const QUrl &url, const QString &body)
{
	Debug(_log, "PUT: [%s] [%s]", QSTRING_CSTR( url.toString() ), QSTRING_CSTR( body ) );
	// Perform request
	QNetworkRequest request(url);
	QNetworkReply* reply = _networkManager->put(request, body.toUtf8());
	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	loop.connect(reply, SIGNAL(finished()), SLOT(quit()));
	// Go into the loop until the request is finished.
	loop.exec();

	httpResponse response;
	if(reply->operation() == QNetworkAccessManager::PutOperation)
	{
		response = getResponse(reply);
	}
	// Free space.
	reply->deleteLater();

	// Return response
	return response;
}

httpResponse ProviderRestApi::getResponse(QNetworkReply* const &reply)
{
	httpResponse response;

	int httpStatusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
	response.setHttpStatusCode(httpStatusCode);

	Debug(_log, "Reply.httpStatusCode [%d]", httpStatusCode );

	response.setNetworkReplyError(reply->error());

	if(reply->error() == QNetworkReply::NoError)
	{
		if ( httpStatusCode != 204 ){
			QByteArray replyData = reply->readAll();

			if ( !replyData.isEmpty())
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
					//std::cout << "Response: [" << QString (jsonDoc.toJson(QJsonDocument::Compact)).toStdString() << "]" << std::endl;
					response.setBody( jsonDoc );
				}
			}
			else
			{	// Create valid body which is empty
				response.setBody( QJsonDocument() );
			}
		}
	}
	else
	{
		QString errorReason;
		if ( httpStatusCode > 0 ) {
			QString httpReason = reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString();
			QString advise;
			switch ( httpStatusCode ) {
			case 400:
				advise = "Check Request Body";
				break;
			case 401:
				advise = "Check Authentication Token (API Key)";
				break;
			case 404:
				advise = "Check Resource given";
				break;
			default:
				break;
			}
			errorReason = QString ("[%3 %4] - %5").arg(QString(httpStatusCode) , httpReason, advise);
		}
		else {
			errorReason = reply->errorString();
		}
		response.setError(true);
		response.setErrorReason(errorReason);

		// Create valid body which is empty
		response.setBody( QJsonDocument() );
	}
	return response;
}

