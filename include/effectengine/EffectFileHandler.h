#pragma once

// util
#include <utils/Logger.h>
#include <effectengine/EffectDefinition.h>
#include <effectengine/EffectSchema.h>
#include <utils/settings.h>

class EffectFileHandler : public QObject
{
	Q_OBJECT
private:
	friend class HyperionDaemon;
	EffectFileHandler(const QString& rootPath, const QJsonDocument& effectConfig, QObject* parent = nullptr);

public:
	static EffectFileHandler* efhInstance;
	static EffectFileHandler* getInstance() { return efhInstance; }

	///
	/// @brief Get all available effects
	///
	std::list<EffectDefinition> getEffects() const { return _availableEffects; }

	///
	/// @brief Get all available schemas
	///
	std::list<EffectSchema> getEffectSchemas() const { return _effectSchemas; }

	///
	/// @brief Save an effect
	/// @param  obj   The effect args
	/// @return If not empty, it contains the error
	///
	QString saveEffect(const QJsonObject& obj);

	///
	/// @brief Delete an effect by name.
	/// @param effectName  The effect name to delete
	/// @return If not empty, it contains the error
	///
	QString deleteEffect(const QString& effectName);

public slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

signals:
	///
	/// @brief Emits whenever the data changes for an effect
	///
	void effectListChanged();

private:
	///
	/// @brief refresh available schemas and effects
	///
	void updateEffects();

	///
	/// @brief Load the effect definition, called by updateEffects()
	///
	bool loadEffectDefinition(const QString& path, const QString& effectConfigFile, EffectDefinition& effectDefinition);

	///
	/// @brief load effect schemas, called by updateEffects()
	///
	bool loadEffectSchema(const QString& path, const QString& effectSchemaFile, EffectSchema& effectSchema);

private:
	QJsonObject _effectConfig;
	Logger* _log;
	const QString _rootPath;

	// available effects
	std::list<EffectDefinition> _availableEffects;

	// all schemas
	std::list<EffectSchema> _effectSchemas;
};
