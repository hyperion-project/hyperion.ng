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
typedef struct _ts PyThreadState;

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

public slots:
	/// Run the specified effect on the given priority channel and optionally specify a timeout
	int runEffect(const QString &effectName, int priority, int timeout = -1)
	{
		return runEffect(effectName, QJsonObject(), priority, timeout);
	};

	/// Run the specified effect on the given priority channel and optionally specify a timeout
	int runEffect(const QString &effectName, const QJsonObject & args, int priority, int timeout = -1, QString pythonScript = "");

	/// Clear any effect running on the provided channel
	void channelCleared(int priority);

	/// Clear all effects
	void allChannelsCleared();

private slots:
	void effectFinished(Effect * effect);

private:
	bool loadEffectDefinition(const QString & path, const QString & effectConfigFile, EffectDefinition &effectDefinition);

	bool loadEffectSchema(const QString & path, const QString & effectSchemaFile, EffectSchema &effectSchema);

	/// Run the specified effect on the given priority channel and optionally specify a timeout
	int runEffectScript(const QString &script, const QString &name, const QJsonObject & args, int priority, int timeout = -1);

private:
	Hyperion * _hyperion;

	QJsonObject _effectConfig;

	std::list<EffectDefinition> _availableEffects;

	std::list<Effect *> _activeEffects;

	std::list<ActiveEffectDefinition> _availableActiveEffects;
	
	std::list<EffectSchema> _effectSchemas;

	PyThreadState * _mainThreadState;

	Logger * _log;
};
