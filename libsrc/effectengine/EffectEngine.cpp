// Python includes
#include <Python.h>
#undef B0

// Qt includes
#include <QResource>
#include <QMetaType>
#include <QFile>
#include <QDir>
#include <QMap>

// hyperion util includes
#include <utils/jsonschema/QJsonSchemaChecker.h>
#include <utils/JsonUtils.h>
#include <utils/Components.h>

// effect engine includes
#include <effectengine/EffectEngine.h>
#include <effectengine/Effect.h>
#include <effectengine/EffectModule.h>
#include <effectengine/EffectFileHandler.h>
#include "HyperionConfig.h"

EffectEngine::EffectEngine(Hyperion * hyperion)
	: _hyperion(hyperion)
	, _availableEffects()
	, _activeEffects()
	, _log(Logger::getInstance("EFFECTENGINE"))
	, _effectFileHandler(EffectFileHandler::getInstance())
{

	Q_INIT_RESOURCE(EffectEngine);
	qRegisterMetaType<hyperion::Components>("hyperion::Components");

	// connect the Hyperion channel clear feedback
	connect(_hyperion, SIGNAL(channelCleared(int)), this, SLOT(channelCleared(int)));
	connect(_hyperion, SIGNAL(allChannelsCleared()), this, SLOT(allChannelsCleared()));

	// get notifications about refreshed effect list
	connect(_effectFileHandler, &EffectFileHandler::effectListChanged, this, &EffectEngine::handleUpdatedEffectList);

	// register smooth cfgs and fill available effects
	handleUpdatedEffectList();
}

EffectEngine::~EffectEngine()
{
}

bool EffectEngine::saveEffect(const QJsonObject& obj, QString& resultMsg)
{
	return _effectFileHandler->saveEffect(obj, resultMsg);
}

bool EffectEngine::deleteEffect(const QString& effectName, QString& resultMsg)
{
	return _effectFileHandler->deleteEffect(effectName, resultMsg);
}

const std::list<ActiveEffectDefinition> &EffectEngine::getActiveEffects()
{
	_availableActiveEffects.clear();

	for (Effect * effect : _activeEffects)
	{
		ActiveEffectDefinition activeEffectDefinition;
		activeEffectDefinition.script   = effect->getScript();
		activeEffectDefinition.name     = effect->getName();
		activeEffectDefinition.priority = effect->getPriority();
		activeEffectDefinition.timeout  = effect->getTimeout();
		activeEffectDefinition.args     = effect->getArgs();
		_availableActiveEffects.push_back(activeEffectDefinition);
	}

	return _availableActiveEffects;
}

const std::list<EffectSchema> & EffectEngine::getEffectSchemas()
{
	return _effectFileHandler->getEffectSchemas();
}

void EffectEngine::cacheRunningEffects()
{
	_cachedActiveEffects.clear();

	for (Effect * effect : _activeEffects)
	{
		ActiveEffectDefinition activeEffectDefinition;
		activeEffectDefinition.script    = effect->getScript();
		activeEffectDefinition.name      = effect->getName();
		activeEffectDefinition.priority  = effect->getPriority();
		activeEffectDefinition.timeout   = effect->getTimeout();
		activeEffectDefinition.args      = effect->getArgs();
		_cachedActiveEffects.push_back(activeEffectDefinition);
		channelCleared(effect->getPriority());
	}
}

void EffectEngine::startCachedEffects()
{
	for (const auto & def : _cachedActiveEffects)
	{
		// the smooth cfg AND origin are ignored for this start!
		runEffect(def.name, def.args, def.priority, def.timeout, def.script);
	}
	_cachedActiveEffects.clear();
}

void EffectEngine::handleUpdatedEffectList()
{
	_availableEffects.clear();

	for (auto def : _effectFileHandler->getEffects())
	{
		// add smoothing configs to Hyperion
		if (def.args["smoothing-custom-settings"].toBool())
		{
			def.smoothCfg = _hyperion->addSmoothingConfig(
				def.args["smoothing-time_ms"].toInt(),
				def.args["smoothing-updateFrequency"].toDouble(),
				0 );
		}
		else
		{
			def.smoothCfg = _hyperion->addSmoothingConfig(true);
		}
		_availableEffects.push_back(def);
	}
	emit effectListUpdated();
}

int EffectEngine::runEffect(const QString &effectName, int priority, int timeout, const QString &origin)
{
	return runEffect(effectName, QJsonObject(), priority, timeout, "", origin);
}

int EffectEngine::runEffect(const QString &effectName, const QJsonObject &args, int priority, int timeout, const QString &pythonScript, const QString &origin, unsigned smoothCfg, const QString &imageData)
{
	Info( _log, "run effect %s on channel %d", QSTRING_CSTR(effectName), priority);

	if (pythonScript.isEmpty())
	{
		const EffectDefinition *effectDefinition = nullptr;
		for (const EffectDefinition &e : _availableEffects)
		{
			if (e.name == effectName)
			{
				effectDefinition = &e;
				break;
			}
		}
		if (effectDefinition == nullptr)
		{
			// no such effect
			Error(_log, "Effect %s not found",  QSTRING_CSTR(effectName));
			return -1;
		}

		return runEffectScript(effectDefinition->script, effectName, (args.isEmpty() ? effectDefinition->args : args), priority, timeout, origin, effectDefinition->smoothCfg);
	}
	return runEffectScript(pythonScript, effectName, args, priority, timeout, origin, smoothCfg, imageData);
}

int EffectEngine::runEffectScript(const QString &script, const QString &name, const QJsonObject &args, int priority, int timeout, const QString &origin, unsigned smoothCfg, const QString &imageData)
{
	// clear current effect on the channel
	channelCleared(priority);

	// create the effect
	Effect *effect = new Effect(_hyperion, priority, timeout, script, name, args, imageData);
	connect(effect, &Effect::setInput, _hyperion, &Hyperion::setInput, Qt::QueuedConnection);
	connect(effect, &Effect::setInputImage, _hyperion, &Hyperion::setInputImage, Qt::QueuedConnection);
	connect(effect, &QThread::finished, this, &EffectEngine::effectFinished);
	connect(_hyperion, &Hyperion::finished, effect, &Effect::requestInterruption, Qt::DirectConnection);
	_activeEffects.push_back(effect);

	// start the effect
	_hyperion->registerInput(priority, hyperion::COMP_EFFECT, origin, name ,smoothCfg);
	effect->start();

	return 0;
}

void EffectEngine::channelCleared(int priority)
{
	for (Effect * effect : _activeEffects)
	{
		if (effect->getPriority() == priority && !effect->isInterruptionRequested())
		{
			effect->requestInterruption();
		}
	}
}

void EffectEngine::allChannelsCleared()
{
	for (Effect * effect : _activeEffects)
	{
		if (effect->getPriority() != 254 && !effect->isInterruptionRequested())
		{
			effect->requestInterruption();
		}
	}
}

void EffectEngine::effectFinished()
{
	Effect* effect = qobject_cast<Effect*>(sender());
	if (!effect->isInterruptionRequested())
	{
		// effect stopped by itself. Clear the channel
		_hyperion->clear(effect->getPriority());
	}

	Info( _log, "effect finished");
	for (auto effectIt = _activeEffects.begin(); effectIt != _activeEffects.end(); ++effectIt)
	{
		if (*effectIt == effect)
		{
			_activeEffects.erase(effectIt);
			break;
		}
	}

	// cleanup the effect
	effect->deleteLater();
}
