#ifndef MDNSPROVIDER_H
#define MDNSPROVIDER_H

#include <qmdnsengine/server.h>
#include <qmdnsengine/hostname.h>
#include <qmdnsengine/provider.h>
#include <qmdnsengine/service.h>

// Qt includes
#include <QObject>
#include <QByteArray>
#include <QScopedPointer>
#include <QLoggingCategory>

// Utility includes
#include <utils/Logger.h>

Q_DECLARE_LOGGING_CATEGORY(mdns_provider)

class MdnsProvider : public QObject
{

public:

	explicit MdnsProvider(QObject* parent = nullptr);

	QList<QByteArray> getServiceTypesProvided() const { return _providedServiceTypes.keys(); }

public slots:

	///
	/// @brief Init MdnsProvider after thread start
	///
	void init();

	///
	/// @brief Stop MdnsProvider to cleanup objects with thread affinity
	///
	void stop();

	void publishService (const QString& serviceType, quint16 servicePort, const QByteArray& serviceName = "");

private slots:

	void onHostnameChanged(const QByteArray& hostname) const;

private:

	/// The logger instance for mDNS-Service
	Logger* _log;

	QScopedPointer<QMdnsEngine::Server> _server;
	QScopedPointer<QMdnsEngine::Hostname> _hostname;

	/// map of services provided
	QMap<QByteArray, QSharedPointer<QMdnsEngine::Provider>> _providedServiceTypes;
};

#endif // MDNSPROVIDER_H
