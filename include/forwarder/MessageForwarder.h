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

// Utils includes
#include <utils/ColorRgb.h>
#include <utils/settings.h>
#include <utils/Logger.h>
#include <utils/Components.h>
#include <utils/Image.h>

// Hyperion includes
#include <hyperion/PriorityMuxer.h>

// Forward declaration
class Hyperion;
class QTcpSocket;
class FlatBufferConnection;
class MessageForwarderFlatbufferClientsHelper;

struct TargetHost {
	QHostAddress host;
	quint16 port;

	bool operator == (TargetHost const& a) const
	{
		return ((host == a.host) && (port == a.port));
	}
};

class MessageForwarder : public QObject
{
	Q_OBJECT
public:
	MessageForwarder(Hyperion* hyperion);
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
	void forwardJsonMessage(const QJsonObject &message);

#if defined(ENABLE_FLATBUF_SERVER) || defined(ENABLE_PROTOBUF_SERVER)
	///
	/// @brief Forward image to all flatbuffer target hosts
	/// @param image The flatbuffer image to send
	///
	///
	void forwardFlatbufferMessage(const QString& name, const Image<ColorRgb> &image);
#endif

	///
	/// @brief Forward message to a single json target host
	/// @param message The JSON message to send
	/// @param socket The TCP-Socket with the connection to the target host
	///
	void sendJsonMessage(const QJsonObject &message, QTcpSocket *socket);

private:

	void enableTargets(bool enable, const QJsonObject& config);

	int startJsonTargets(const QJsonObject& config);
	void stopJsonTargets();

	int startFlatbufferTargets(const QJsonObject& config);
	void stopFlatbufferTargets();

	/// Hyperion instance
	Hyperion *_hyperion;

	/// Logger instance
	Logger   *_log;

	/// Muxer instance
	PriorityMuxer *_muxer;

	// JSON connections for forwarding
	QList<TargetHost> _jsonTargets;

	/// Flatbuffer connection for forwarding
	QList<TargetHost> _flatbufferTargets;

	/// Flag if forwarder is enabled
	bool _forwarder_enabled = true;

	const int _priority;

	MessageForwarderFlatbufferClientsHelper* _messageForwarderFlatBufHelper;
};

class MessageForwarderFlatbufferClientsHelper : public QObject
{
	Q_OBJECT


public:
	MessageForwarderFlatbufferClientsHelper();
	~MessageForwarderFlatbufferClientsHelper();

signals:
	void addClient(const QString& origin, const TargetHost& targetHost, int priority, bool skipReply);
	void clearClients();

public slots:
	bool isFree() const;

	void forwardImage(const Image<ColorRgb>& image);
	void addClientHandler(const QString& origin, const TargetHost& targetHost, int priority, bool skipReply);
	void clearClientsHandler();

private:

	QList<FlatBufferConnection*> _forwardClients;
	bool _free;
};
