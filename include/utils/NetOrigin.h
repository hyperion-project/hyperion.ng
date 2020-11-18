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
	NetOrigin(QObject* parent = nullptr, Logger* log = Logger::getInstance("NETWORK"));

public:
	///
	/// @brief Check if address is allowed to connect. The local address is the network adapter ip this connection comes from
	/// @param address  The peer address
	/// @param local    The local address of the socket (Differs based on NetworkAdapter IP or localhost)
	/// @return         True when allowed, else false
	///
	bool accessAllowed(const QHostAddress& address, const QHostAddress& local) const;

	///
	/// @brief Check if address is in subnet of local
	/// @return True or false
	///
	bool isLocalAddress(const QHostAddress& address, const QHostAddress& local) const;

	static NetOrigin *getInstance() { return instance; }
	static NetOrigin *instance;

private slots:
	///
	/// @brief Handle settings update from SettingsManager
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

private:
	Logger* _log;
	/// True when internet access is allowed
	bool _internetAccessAllowed;
	/// Whitelisted ip addresses
	QList<QHostAddress> _ipWhitelist;

};
