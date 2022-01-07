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

class MessageForwarder : public QObject
{
	Q_OBJECT
public:
	MessageForwarder(Hyperion* hyperion);
	~MessageForwarder() override;

	void addJsonTarget(const QJsonObject& targetConfig);
	void addFlatbufferTarget(const QJsonObject& targetConfig);

private slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument &config);

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
	void handlePriorityChanges(quint8 priority);

	///
	/// @brief Forward message to all json target hosts
	/// @param message The JSON message to send
	///
	void forwardJsonMessage(const QJsonObject &message);

	///
	/// @brief Forward image to all flatbuffer target hosts
	/// @param image The flatbuffer image to send
	///
	void forwardFlatbufferMessage(const QString& name, const Image<ColorRgb> &image);

	///
	/// @brief Forward message to a single json target host
	/// @param message The JSON message to send
	/// @param socket The TCP-Socket with the connection to the target host
	///
	void sendJsonMessage(const QJsonObject &message, QTcpSocket *socket);

private:

	struct TargetHost {
		QHostAddress host;
		quint16 port;

		bool operator == (TargetHost const& a) const
		{
			return ((host == a.host) && (port == a.port));
		}
	};

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
	QList<FlatBufferConnection*> _forwardClients;

	/// Flag if forwarder is enabled
	bool _forwarder_enabled = true;

	const int _priority;
};
