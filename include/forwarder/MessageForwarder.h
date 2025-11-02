#pragma once

// STL includes
#include <vector>
#include <map>
#include <cstdint>
#include <limits>

// QT includes
#include <QList>
#include <QStringList>
#include <QHostAddress>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSharedPointer>
#include <QWeakPointer>

// Utils includes
#include <utils/ColorRgb.h>
#include <utils/settings.h>
#include <utils/Logger.h>
#include <utils/Components.h>
#include <utils/Image.h>

// Hyperion includes
#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#endif
#include <db/SettingsTable.h>
#include <hyperion/PriorityMuxer.h>

// Forward declaration
class Hyperion;
class QTcpSocket;
class FlatBufferConnection;
class MessageForwarderFlatbufferClientsHelper;

struct TargetHost {
	QHostAddress host;
	quint16 port;
	QJsonArray instanceIds;

	bool operator == (TargetHost const& a) const
	{
		return ((host == a.host) && (port == a.port));
	}
};

class MessageForwarder : public QObject
{
	Q_OBJECT
public:
	explicit MessageForwarder(const QJsonDocument& config);
	~MessageForwarder() override;

	void addJsonTarget(const QJsonObject& targetConfig);
	void addFlatbufferTarget(const QJsonObject& targetConfig);

public slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingsType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument &config);

	void init();
	bool connectToInstance(quint8 instanceID);
	void disconnectFromInstance(quint8 instanceID);
	void start();
	void stop();

private slots:
	///
	/// @brief Handle component state change MessageForwarder
	/// @param component  The component from enum
	/// @param enable     The new state
	///
	void handleCompStateChangeRequest(hyperion::Components component, bool enable);

	///
	/// @brief Handle priority updates from Priority Muxer
	/// @param  priority  The new visible priority
	///
	void handlePriorityChanges(int priority);

	///
	/// @brief Forward message to all json target hosts
	/// @param message The JSON message to send
	///
	void forwardJsonMessage(const QJsonObject &message, quint8 instanceId);

#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
	///
	/// @brief Forward image to all flatbuffer target hosts
	/// @param image The flatbuffer image to send
	///
	///
	void forwardFlatbufferMessage(const QString& name, const Image<ColorRgb> &image) const;
#endif

	///
	/// @brief Forward message to a single json target host
	/// @param message The JSON message to send
	/// @param socket The TCP-Socket with the connection to the target host
	/// @param targetInstanceIds The instances the command shoulkd be applied to
	///
	void sendJsonMessage(const QJsonObject &message, QTcpSocket *socket, const QJsonArray& targetInstanceIds = {});

private:

	void handleTargets(bool enable, const QJsonObject& config = {});
	bool activateFlatbufferTargets(int priority = PriorityMuxer::LOWEST_PRIORITY);

	bool isFlatbufferComponent(int priority);
	void disconnectFlatBufferComponents(int priority = PriorityMuxer::LOWEST_PRIORITY) const;

	int startJsonTargets(const QJsonObject& config);
	void stopJsonTargets();

	int startFlatbufferTargets(const QJsonObject& config);
	void stopFlatbufferTargets();

	/// Logger instance
	QSharedPointer<Logger> _log;

	QJsonDocument _config;
	bool _isActive = true;
	const int _priority;
	bool _isEnabled;
	quint8 _toBeForwardedInstanceID;

	SettingsTable _settings;

	/// Hyperion instance forwarded
	///  Hyperion instance
	QWeakPointer<Hyperion> _hyperionWeak;

	/// Muxer instance
	QWeakPointer<PriorityMuxer> _muxerWeak;

	// JSON connections for forwarding
	QList<TargetHost> _jsonTargets;

	/// Flatbuffer connection for forwarding
	QList<TargetHost> _flatbufferTargets;

	QSharedPointer<MessageForwarderFlatbufferClientsHelper> _messageForwarderFlatBufHelper;

#ifdef ENABLE_MDNS
	QSharedPointer<MdnsBrowser> _mdnsBrowser = MdnsBrowser::getInstance();
#endif
};

class MessageForwarderFlatbufferClientsHelper : public QObject
{
	Q_OBJECT


public:
	MessageForwarderFlatbufferClientsHelper();
	~MessageForwarderFlatbufferClientsHelper() override;

signals:
	void addClient(const QString& origin, const TargetHost& targetHost, int priority, bool skipReply);
	void clearClients();

public slots:
	bool isFree() const;

	void forwardImage(const Image<ColorRgb>& image);
	void addClientHandler(const QString& origin, const TargetHost& targetHost, int priority, bool skipReply);
	void clearClientsHandler();

private:

	QList<QSharedPointer<FlatBufferConnection>> _forwardClients;

	bool _isFree;
};
