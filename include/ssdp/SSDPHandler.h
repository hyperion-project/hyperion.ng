#pragma once

#include <ssdp/SSDPServer.h>

#include <QNetworkConfiguration>

// utils
#include <utils/settings.h>

class WebServer;
class QNetworkConfigurationManager;

///
/// Manage SSDP discovery. SimpleServiceDisoveryProtocol is the discovery subset of UPnP. Implemented is spec V1.0.
/// As SSDP requires a webserver, this class depends on it
/// UPnP 1.0: spec: http://upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.0.pdf
///

class SSDPHandler : public SSDPServer{
	Q_OBJECT
public:
	SSDPHandler(WebServer* webserver, const quint16& flatBufPort, const quint16& jsonServerPort, QObject * parent = nullptr);
	~SSDPHandler();

public slots:
	///
	/// @brief Init SSDP after thread start
	///
	void initServer();

	///
	/// @brief get state changes from webserver
	/// @param newState true for started and false for stopped
	///
	void handleWebServerStateChange(const bool newState);

	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(const settings::type& type, const QJsonDocument& config);

private:
	///
	/// @brief Build http url for current ip:port/desc.xml
	///
	const QString getDescAddress();

	///
	/// @brief Get the base address
	///
	const QString getBaseAddress();

	///
	/// @brief Build the ssdp description (description.xml)
	///
	const QString buildDesc();

	///
	/// @brief Get the local address of interface
	/// @return the address, might be empty
	///
	const QString getLocalAddress();

	///
	/// @brief Send alive/byebye message based on _deviceList
	/// @param alive When true send alive, else byebye
	///
	void sendAnnounceList(const bool alive);

private slots:
	///
	/// @brief Handle the mSeach request from SSDPServer
	/// @param target  The ST service type
	/// @param mx      Answer with delay in s
	/// @param address The ip of the caller
	/// @param port    The port of the caller
	///
	void handleMSearchRequest(const QString& target, const QString& mx, const QString address, const quint16 & port);

	///
	/// @brief Handle changes in the network configuration
	/// @param conig New config
	///
	void handleNetworkConfigurationChanged(const QNetworkConfiguration &config);

private:
	WebServer* _webserver;
	QString    _localAddress;
	QNetworkConfigurationManager* _NCA;
	QString _uuid;
	/// Targets for announcement
	std::vector<QString> _deviceList;
};
