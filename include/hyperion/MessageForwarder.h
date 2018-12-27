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

class Hyperion;

class MessageForwarder : public QObject
{
	Q_OBJECT
public:

	MessageForwarder(Hyperion* hyperion, const QJsonDocument & config);
	~MessageForwarder();

	void addJsonSlave(QString slave);
	void addProtoSlave(QString slave);

	bool protoForwardingEnabled();
	bool jsonForwardingEnabled();
	bool forwardingEnabled() { return jsonForwardingEnabled() || protoForwardingEnabled(); };
	QStringList getProtoSlaves() const { return _protoSlaves; };
	QStringList getJsonSlaves() const { return _jsonSlaves; };

private slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(const settings::type& type, const QJsonDocument& config);

private:
	Hyperion* _hyperion;
	Logger*   _log;
	QStringList   _protoSlaves;
	QStringList   _jsonSlaves;
};
