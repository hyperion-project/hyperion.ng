#ifndef MDNSENGINE_H
#define MDNSENGINE_H

#include <qmdnsengine/server.h>
#include <qmdnsengine/hostname.h>
#include <qmdnsengine/provider.h>
#include <qmdnsengine/service.h>

// Qt includes
#include <QObject>
#include <QByteArray>

// Utility includes
#include <utils/Logger.h>

class MdnsEngine : public QObject
{
	Q_OBJECT

public:

	MdnsEngine(QObject* parent = nullptr);
	~MdnsEngine() override;

	QList<QByteArray> getServiceTypesProvided() const { return _providedServiceTypes.keys(); }

public slots:

	///
	/// @brief Init MdnsEngine after thread start
	///
	void initEngine();

	void publishService (const QByteArray& serviceType, quint16 servicePort, const QByteArray& serviceName = "");

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

#endif // MDNSENGINE_H
