#pragma once

// Qt includes
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonArray>

// Hyperion includes
#include <hyperion/Hyperion.h>

// Effect engine includes
#include <effectengine/EffectDefinition.h>
#include <effectengine/ActiveEffectDefinition.h>
#include <effectengine/EffectSchema.h>
#include <utils/Logger.h>

// pre-declarioation
class Effect;

class EffectEngine : public QObject
{
	Q_OBJECT

public:
	EffectEngine(Hyperion * hyperion, const QJsonObject & jsonEffectConfig);
	virtual ~EffectEngine();

	void readEffects();

	const std::list<EffectDefinition> & getEffects() const
	{
		return _availableEffects;
	};

	const std::list<ActiveEffectDefinition> & getActiveEffects();

	const std::list<EffectSchema> & getEffectSchemas()
	{
		return _effectSchemas;
	};

	///
	/// @brief Get all init data of the running effects and stop them
	///
	void cacheRunningEffects();

	///
	/// @brief Start all cached effects, origin and smooth cfg is default
	///
	void startCachedEffects();

signals:
	/// Emit when the effect list has been updated
	void effectListUpdated();

public slots:
	/// Run the specified effect on the given priority channel and optionally specify a timeout
	int runEffect(const QString &effectName, int priority, int timeout = -1, const QString &origin="System");

	/// Run the specified effect on the given priority channel and optionally specify a timeout
	int runEffect(const QString &effectName, const QJsonObject & args, int priority, int timeout = -1, const QString &pythonScript = "", const QString &origin = "System", unsigned smoothCfg=0);

	/// Clear any effect running on the provided channel
	void channelCleared(int priority);

	/// Clear all effects
	void allChannelsCleared();

private slots:
	void effectFinished();

private:
	bool loadEffectDefinition(const QString & path, const QString & effectConfigFile, EffectDefinition &effectDefinition);

	bool loadEffectSchema(const QString & path, const QString & effectSchemaFile, EffectSchema &effectSchema);

	/// Run the specified effect on the given priority channel and optionally specify a timeout
	int runEffectScript(const QString &script, const QString &name, const QJsonObject & args, int priority, int timeout = -1, const QString & origin="System", unsigned smoothCfg=0);

private:
	Hyperion * _hyperion;

	QJsonObject _effectConfig;

	std::list<EffectDefinition> _availableEffects;

	std::list<Effect *> _activeEffects;

	std::list<ActiveEffectDefinition> _availableActiveEffects;

	std::list<ActiveEffectDefinition> _cachedActiveEffects;

	std::list<EffectSchema> _effectSchemas;

	Logger * _log;
};
