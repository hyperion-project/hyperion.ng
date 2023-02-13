// Python includes
#include <Python.h>
#undef B0

// Qt includes
#include <QResource>

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
	, _log(nullptr)
	, _effectFileHandler(EffectFileHandler::getInstance())
{
	QString subComponent = hyperion->property("instance").toString();
	_log= Logger::getInstance("EFFECTENGINE", subComponent);

	Q_INIT_RESOURCE(EffectEngine);
	qRegisterMetaType<hyperion::Components>("hyperion::Components");

	// connect the Hyperion channel clear feedback
	connect(_hyperion, &Hyperion::channelCleared, this, &EffectEngine::channelCleared);
	connect(_hyperion, &Hyperion::allChannelsCleared, this, &EffectEngine::allChannelsCleared);

	// get notifications about refreshed effect list
	connect(_effectFileHandler, &EffectFileHandler::effectListChanged, this, &EffectEngine::handleUpdatedEffectList);

	// register smooth cfgs and fill available effects
	handleUpdatedEffectList();
}

EffectEngine::~EffectEngine()
{
	for (Effect * effect : _activeEffects)
	{
		effect->wait();
		delete effect;
	}
}

QString EffectEngine::saveEffect(const QJsonObject& obj)
{
	return _effectFileHandler->saveEffect(obj);
}

QString EffectEngine::deleteEffect(const QString& effectName)
{
	return _effectFileHandler->deleteEffect(effectName);
}

std::list<ActiveEffectDefinition> EffectEngine::getActiveEffects() const
{
	std::list<ActiveEffectDefinition> availableActiveEffects;

	for (Effect * effect : _activeEffects)
	{
		ActiveEffectDefinition activeEffectDefinition;
		activeEffectDefinition.script   = effect->getScript();
		activeEffectDefinition.name     = effect->getName();
		activeEffectDefinition.priority = effect->getPriority();
		activeEffectDefinition.timeout  = effect->getTimeout();
		activeEffectDefinition.args     = effect->getArgs();
		availableActiveEffects.push_back(activeEffectDefinition);
	}

	return availableActiveEffects;
}

std::list<EffectSchema> EffectEngine::getEffectSchemas() const
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

	//Add smoothing config entry to support dynamic effects done in configurator
	_hyperion->updateSmoothingConfig(SmoothingConfigID::EFFECT_DYNAMIC);

	unsigned specificId = SmoothingConfigID::EFFECT_SPECIFIC;
	for (auto def : _effectFileHandler->getEffects())
	{
		// add smoothing configurations to Hyperion
		if (def.args["smoothing-custom-settings"].toBool())
		{
			int settlingTime_ms = def.args["smoothing-time_ms"].toInt();
			double ledUpdateFrequency_hz = def.args["smoothing-updateFrequency"].toDouble();
			unsigned updateDelay {0};

			Debug(_log, "Effect \"%s\": Add custom smoothing settings [%d]. Type: Linear, Settling time: %dms, Interval: %.fHz ", QSTRING_CSTR(def.name), specificId, settlingTime_ms, ledUpdateFrequency_hz);

			def.smoothCfg = _hyperion->updateSmoothingConfig(
								++specificId,
								settlingTime_ms,
								ledUpdateFrequency_hz,
								updateDelay );
		}
		else
		{
			def.smoothCfg = SmoothingConfigID::SYSTEM;
		}
		_availableEffects.push_back(def);
	}
	emit effectListUpdated();
}

int EffectEngine::runEffect(const QString &effectName, int priority, int timeout, const QString &origin)
{
	unsigned smoothCfg = SmoothingConfigID::SYSTEM;
	for (const auto &def : _availableEffects)
	{
		if (def.name == effectName)
		{
			smoothCfg = def.smoothCfg;
			break;
		}
	}
	return runEffect(effectName, QJsonObject(), priority, timeout, "", origin, smoothCfg);
}

int EffectEngine::runEffect(const QString &effectName, const QJsonObject &args, int priority, int timeout, const QString &pythonScript, const QString &origin, unsigned smoothCfg, const QString &imageData)
{
	//In case smoothing information is provided dynamically use temp smoothing config item (2)
	if (smoothCfg == SmoothingConfigID::SYSTEM && args["smoothing-custom-settings"].toBool())
	{
		int settlingTime_ms = args["smoothing-time_ms"].toInt();
		double ledUpdateFrequency_hz = args["smoothing-updateFrequency"].toDouble();
		unsigned updateDelay {0};

		Debug(_log, "Effect \"%s\": Apply dynamic smoothing settings, if smoothing. Type: Linear, Settling time: %dms, Interval: %.fHz ", QSTRING_CSTR(effectName), settlingTime_ms, ledUpdateFrequency_hz);

		smoothCfg = _hyperion->updateSmoothingConfig(
						SmoothingConfigID::EFFECT_DYNAMIC,
						settlingTime_ms,
						ledUpdateFrequency_hz,
						updateDelay
						);
	}

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
			Error(_log, "Effect \"%s\" not found",  QSTRING_CSTR(effectName));
			return -1;
		}

		Info( _log, "Run effect \"%s\" on channel %d", QSTRING_CSTR(effectName), priority);
		return runEffectScript(effectDefinition->script, effectName, (args.isEmpty() ? effectDefinition->args : args), priority, timeout, origin, effectDefinition->smoothCfg);
	}
	Info( _log, "Run effect \"%s\" on channel %d", QSTRING_CSTR(effectName), priority);
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
	Debug(_log, "Start the effect: name [%s]", QSTRING_CSTR(name));
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
		if (effect->getPriority() != PriorityMuxer::BG_PRIORITY && !effect->isInterruptionRequested())
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

	Info( _log, "Effect [%s] finished", QSTRING_CSTR(effect->getName()));
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
