#pragma once

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QMap>
#include <QSharedPointer>
#include <QWeakPointer>

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
	static void createInstance(const QString& rootPath, const QJsonDocument& effectConfig, QObject* parent = nullptr);
	static QSharedPointer<EffectFileHandler> getInstance();
	static QWeakPointer<EffectFileHandler> getInstanceWeak() { return _instance.toWeakRef(); }
	static void destroyInstance();
	static bool isValid();

private:
	static QSharedPointer<EffectFileHandler> _instance;

public:
	QJsonObject getEffectConfig() const { return _effectConfig; };

	QString getRootPath() const { return _rootPath; }

	///
	/// @brief Get all available effects
	///
	QList<EffectDefinition> getEffects() const { return _availableEffects; }

	///
	/// @brief Get all available schemas
	///
	QList<EffectSchema> getEffectSchemas() const { return _effectSchemas; }

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

	///
	/// @brief Load all effect definitions from a directory, called by updateEffects()
	///
	void loadEffectsFromDirectory(const QString& path, const QDir& directory, const QStringList& disableList, QMap<QString, EffectDefinition>& availableEffects);

	///
	/// @brief Load all effect schemas from a directory, called by updateEffects()
	///
	void loadSchemasFromDirectory(const QString& path, QDir& directory);

	///
	/// @brief Resolve the file path for a new/updated effect and write it, called by saveEffect()
	///
	QString resolveEffectFilePath(const QJsonObject& message, const QString& effectName, const QJsonArray& effectArray, QJsonObject& effectJson);

	///
	/// @brief Save an embedded image for a gif effect, called by saveEffect()
	///
	QString saveEffectImage(const QJsonObject& message, const QJsonArray& effectArray, QJsonObject& effectJson);

	///
	/// @brief Remove the unused imageSource key (url/file) from the effect args
	///
	void cleanImageSource(const QJsonObject& message, QJsonObject& effectJson) const;

	///
	/// @brief Remove effect configuration and associated image files
	///
	QString removeEffectFiles(const EffectDefinition& effect, const QFileInfo& effectConfigurationFile);

private:
 	QJsonObject _effectConfig;
	QSharedPointer<Logger> _log;
	const QString _rootPath;

	// available effects
	QList<EffectDefinition> _availableEffects;

	// all schemas
	QList<EffectSchema> _effectSchemas;
};
