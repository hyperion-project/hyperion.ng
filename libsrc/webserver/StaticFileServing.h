#ifndef STATICFILESERVING_H
#define STATICFILESERVING_H

#include <QMimeDatabase>

#include "QtHttpRequest.h"
#include "QtHttpReply.h"
#include "CgiHandler.h"

#include <utils/Logger.h>

class StaticFileServing : public QObject
{
    Q_OBJECT

public:
	explicit StaticFileServing (QObject * parent = nullptr);
	~StaticFileServing() override;

	///
	/// @brief Overwrite current base url
	///
	void setBaseUrl(const QString& url);
	///
	/// @brief Set a new SSDP description, if empty the description will be unset and clients will get a NotFound
	/// @param The description
	///
	void setSSDPDescription(const QString& desc);

public slots:
	void onRequestNeedsReply  (QtHttpRequest * request, QtHttpReply * reply);

private:
	QString         _baseUrl;
	QMimeDatabase * _mimeDb;
	CgiHandler      _cgi;
	Logger        * _log;
	QByteArray      _ssdpDescription;

	void printErrorToReply (QtHttpReply * reply, QtHttpReply::StatusCode code, QString errorMessage);

};

#endif // STATICFILESERVING_H
