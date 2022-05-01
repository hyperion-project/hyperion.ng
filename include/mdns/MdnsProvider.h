#ifndef MDNSPROVIDER_H
#define MDNSPROVIDER_H

#include <qmdnsengine/server.h>
#include <qmdnsengine/hostname.h>
#include <qmdnsengine/provider.h>
#include <qmdnsengine/service.h>

// Qt includes
#include <QObject>
#include <QByteArray>

// Utility includes
#include <utils/Logger.h>

class MdnsProvider : public QObject
{

public:

	MdnsProvider(QObject* parent = nullptr);
	~MdnsProvider() override;

	QList<QByteArray> getServiceTypesProvided() const { return _providedServiceTypes.keys(); }

public slots:

	///
	/// @brief Init MdnsProvider after thread start
	///
	void init();

	void publishService (const QString& serviceType, quint16 servicePort, const QByteArray& serviceName = "");

private slots:

	void onHostnameChanged(const QByteArray& hostname);

private:

	/// The logger instance for mDNS-Service
	Logger* _log;

	QMdnsEngine::Server* _server;
	QMdnsEngine::Hostname* _hostname;

	/// map of services provided
	QMap<QByteArray, QMdnsEngine::Provider*> _providedServiceTypes;
};

#endif // MDNSPROVIDER_H
