#ifndef MDNSENGINE_H
#define MDNSENGINE_H

#include <qmdnsengine/dns.h>
#include <qmdnsengine/server.h>
#include <qmdnsengine/service.h>
#include <qmdnsengine/hostname.h>
#include <qmdnsengine/provider.h>
#include <qmdnsengine/browser.h>
#include <qmdnsengine/cache.h>

// Qt includes
#include <QObject>
#include <QByteArray>

// Utility includes
#include <utils/Logger.h>

class MdnsEngine : public QObject
{
	Q_OBJECT

//private:
//	friend class HyperionDaemon;

//	MdnsEngine(QObject* parent = nullptr);
//	~MdnsEngine();

public:

	MdnsEngine(QObject* parent = nullptr);
	~MdnsEngine() override;

	//static MdnsEngine* instance;
	//static MdnsEngine* getInstance() { return instance; }

	QList<QByteArray> getServiceTypesProvided() const { return _providedServiceTypes.keys(); }

public slots:

	///
	/// @brief Init MdnsEngine after thread start
	///
	void initEngine();

	void provideServiceType(const QByteArray& serviceType, quint16 servicePort, const QByteArray& serviceName = "");

	///
	/// @brief Browse for a service of type
	///
	void browseForServiceType(const QByteArray& serviceType);

	QHostAddress getHostAddress(const QString& hostName);
	QHostAddress getHostAddress(const QByteArray& hostName);

	QString getHostByService(const QByteArray& serviceName);

	QVariantList getServicesDiscoveredJson(const QByteArray& serviceType, const QString& filter = ".*") const;

private slots:

	void resolveService(const QMdnsEngine::Service& service);
	void resolveHostName(const QByteArray& hostName);

	void onHostnameChanged(const QByteArray& hostname);
	void onServiceAdded(const QMdnsEngine::Service& service);
	void onServiceUpdated(const QMdnsEngine::Service& service);
	void onServiceRemoved(const QMdnsEngine::Service& service);

	void onHostNameResolved(const QHostAddress& address);

private:

	void printCache(const QByteArray& name = 0, quint16 type = QMdnsEngine::ANY) const;

	/// The logger instance for mDNS-Service
	Logger* _log;

	QMdnsEngine::Server* _server;

	QMdnsEngine::Hostname* _hostname;
	QMdnsEngine::Provider* _provider;

	QMdnsEngine::Cache* _cache;

	/// map of services provided
	QMap<QByteArray, QMdnsEngine::Provider*> _providedServiceTypes;

	/// map of service names and browsers
	QMap<QByteArray, QMdnsEngine::Browser*> _browsedServiceTypes;
};

#endif // MDNSENGINE_H
