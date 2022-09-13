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
#include <effectengine/Effect.h>
#include <effectengine/ActiveEffectDefinition.h>
#include <effectengine/EffectSchema.h>
#include <utils/Logger.h>

#include <hyperion/LinearColorSmoothing.h>

// pre-declaration
class Effect;
class EffectFileHandler;

class EffectEngine : public QObject
{
	Q_OBJECT

public:
	EffectEngine(Hyperion * hyperion);
	~EffectEngine() override;

	std::list<EffectDefinition> getEffects() const { return _availableEffects; }

	std::list<ActiveEffectDefinition> getActiveEffects() const;

	///
	/// Get available schemas from EffectFileHandler
	/// @return all schemas
	///
	std::list<EffectSchema> getEffectSchemas() const;

	///
	/// @brief Save an effect with EffectFileHandler
	/// @param  obj   The effect args
	/// @return If not empty, it contains the error
	///
	QString saveEffect(const QJsonObject& obj);

	///
	/// @brief Delete an effect by name.
	/// @param  effectName  The effect name to delete
	/// @return If not empty, it contains the error
	///
	QString deleteEffect(const QString& effectName);

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
	int runEffect(const QString &effectName, int priority, int timeout = PriorityMuxer::ENDLESS, const QString &origin="System");

	/// Run the specified effect on the given priority channel and optionally specify a timeout
	int runEffect(const QString &effectName
				, const QJsonObject &args
				, int priority
				, int timeout = PriorityMuxer::ENDLESS
				, const QString &pythonScript = ""
				, const QString &origin = "System"
				, unsigned smoothCfg=SmoothingConfigID::SYSTEM
				, const QString &imageData = ""
	);

	/// Clear any effect running on the provided channel
	void channelCleared(int priority);

	/// Clear all effects
	void allChannelsCleared();

private slots:
	void effectFinished();

	///
	/// @brief is called whenever the EffectFileHandler emits updated effect list
	///
	void handleUpdatedEffectList();

private:
	/// Run the specified effect on the given priority channel and optionally specify a timeout
	int runEffectScript(const QString &script
				,const QString &name
				, const QJsonObject &args
				, int priority
				, int timeout = PriorityMuxer::ENDLESS
				, const QString &origin="System"
				, unsigned smoothCfg=SmoothingConfigID::SYSTEM
				, const QString &imageData = ""
	);

private:
	Hyperion * _hyperion;

	std::list<EffectDefinition> _availableEffects;

	std::list<Effect *> _activeEffects;

	std::list<ActiveEffectDefinition> _cachedActiveEffects;

	Logger * _log;

	// The global effect file handler
	EffectFileHandler* _effectFileHandler;
};
