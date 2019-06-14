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
	static EffectFileHandler* getInstance() { return efhInstance; };

	///
	/// @brief Get all available effects
	///
	const std::list<EffectDefinition> & getEffects() const { return _availableEffects; };

	///
	/// @brief Get all available schemas
	///
	const std::list<EffectSchema> & getEffectSchemas() { return _effectSchemas; };

	///
	/// @brief Save an effect
	/// @param       obj       The effect args
	/// @param[out] resultMsg  The feedback message
	/// @return True on success else false
	///
	bool saveEffect(const QJsonObject& obj, QString& resultMsg);

	///
	/// @brief Delete an effect by name.
	/// @param[in]  effectName  The effect name to delete
	/// @param[out] resultMsg   The message on error
	/// @return True on success else false
	///
	bool deleteEffect(const QString& effectName, QString& resultMsg);

public slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(const settings::type& type, const QJsonDocument& config);

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
	bool loadEffectDefinition(const QString & path, const QString & effectConfigFile, EffectDefinition &effectDefinition);

	///
	/// @brief load effect schemas, called by updateEffects()
	///
	bool loadEffectSchema(const QString & path, const QString & effectSchemaFile, EffectSchema &effectSchema);

private:
	QJsonObject _effectConfig;
	Logger* _log;
	const QString _rootPath;

	// available effects
	std::list<EffectDefinition> _availableEffects;

	// all schemas
	std::list<EffectSchema> _effectSchemas;
};
