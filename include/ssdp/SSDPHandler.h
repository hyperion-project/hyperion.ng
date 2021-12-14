#pragma once

#include <ssdp/SSDPServer.h>

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QNetworkConfiguration>
#endif

// utils
#include <utils/settings.h>

class WebServer;
class QNetworkConfigurationManager;

///
/// Manage SSDP discovery. SimpleServiceDisoveryProtocol is the discovery subset of UPnP. Implemented is spec V1.0.
/// As SSDP requires a webserver, this class depends on it
/// UPnP 1.0: spec: http://upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.0.pdf
///

class SSDPHandler : public SSDPServer
{
	Q_OBJECT
public:
	SSDPHandler(WebServer* webserver, quint16 flatBufPort, quint16 protoBufPort, quint16 jsonServerPort, quint16 sslPort, const QString& name, QObject* parent = nullptr);
	~SSDPHandler() override;

	///
	/// @brief Sends BYE BYE and stop server
	///
	void stopServer();

public slots:
	///
	/// @brief Init SSDP after thread start
	///
	void initServer();

	///
	/// @brief get state changes from webserver
	/// @param newState true for started and false for stopped
	///
	void handleWebServerStateChange(bool newState);

	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

private:
	///
	/// @brief Build http url for current ip:port/desc.xml
	///
	QString getDescAddress() const;

	///
	/// @brief Get the base address
	///
	QString getBaseAddress() const;

	///
	/// @brief Build the ssdp description (description.xml)
	///
	QString buildDesc() const;

	///
	/// @brief Get the local address of interface
	/// @return the address, might be empty
	///
	QString getLocalAddress() const;

	///
	/// @brief Send alive/byebye message based on _deviceList
	/// @param alive When true send alive, else byebye
	///
	void sendAnnounceList(bool alive);

private slots:
	///
	/// @brief Handle the mSeach request from SSDPServer
	/// @param target  The ST service type
	/// @param mx      Answer with delay in s
	/// @param address The ip of the caller
	/// @param port    The port of the caller
	///
	void handleMSearchRequest(const QString& target, const QString& mx, const QString address, quint16 port);

private:
	WebServer* _webserver;
	QString    _localAddress;
	QString _uuid;

	/// Targets for announcement
	std::vector<QString> _deviceList;

//Handle elements deprecated from Qt 6.x and reported as deprecatedsince 5.15.x
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
#endif

private slots:

///
/// @brief Handle changes in the network configuration
/// @param conig New config
///
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	void handleNetworkConfigurationChanged(const QNetworkConfiguration& config);
#endif

private:

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	QNetworkConfigurationManager* _NCA;
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
QT_WARNING_POP
#endif
};
