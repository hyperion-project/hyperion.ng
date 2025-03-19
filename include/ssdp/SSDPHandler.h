#pragma once

#include <ssdp/SSDPServer.h>

// utils
#include <utils/settings.h>

///
/// Manage SSDP discovery. SimpleServiceDisoveryProtocol is the discovery subset of UPnP. Implemented is spec V1.0.
/// As SSDP requires a webserver, this class depends on it
/// UPnP 1.0: spec: http://upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.0.pdf
///

class SSDPHandler : public SSDPServer
{
	Q_OBJECT
public:
	SSDPHandler(QObject* parent = nullptr);
	~SSDPHandler() override;

public slots:
	///
	/// @brief Init SSDP after thread start
	///
	void initServer();

	///
	/// @brief Sends BYE BYE and stop server
	///
	void stop() override;

	///
	/// @brief get state changes from webserver
	/// @param newState true for started and false for stopped
	///
	void onStateChange(bool newState);

	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

	void onPortChanged(const QString& serviceType, quint16 servicePort);


signals:
	void descriptionUpdated(const QString& description);

private:
	///
	/// @brief Build http url for current ip:port/desc.xml
	///
	QString getDescriptionAddress() const override;

	///
	/// @brief Get the base address
	///
	QString getBaseAddress() const;

	///
	/// @brief Build the ssdp description (description.xml)
	///
	QString getDescription() const;

	///
	/// @brief Get the local address of interface
	/// @return the address, might be empty
	///
	QString getLocalAddress() const;

	QString getInfo() const override;

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
	QString  _localAddress;

	quint16 _httpPort;
	quint16 _httpsPort;
	quint16 _jsonApiPort;
	quint16 _flatBufferPort;
	quint16 _protoBufferPort;

	QString _name;

	/// Targets for announcement
	std::vector<QString> _deviceList;
};
