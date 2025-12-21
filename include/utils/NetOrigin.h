#pragma once

// qt
#include <QHostAddress>
#include <QJsonArray>

// utils
#include <utils/Logger.h>
#include <utils/settings.h>

///
/// @brief Checks the origin ip addresses for access allowed
///
class NetOrigin : public QObject
{
	Q_OBJECT
private:
	friend class HyperionDaemon;
	NetOrigin(QObject* parent = nullptr, QSharedPointer<Logger> log = Logger::getInstance("NETWORK"));

public:
	///
	/// @brief Check if address is in subnet of local
	/// @return True or false
	///
	bool isLocalAddress(const QHostAddress& address, const QHostAddress& local) const;

	static void createInstance(QObject* parent = nullptr);
	static QSharedPointer<NetOrigin> getInstance();
	static QWeakPointer<NetOrigin> getInstanceWeak() { return _instance.toWeakRef(); }
	static void destroyInstance();
	static bool isValid();

private:
	static QSharedPointer<NetOrigin> _instance;

private:
	QSharedPointer<Logger> _log;
};
