// Don't use debug Python APIs on Windows (GitHub Actions only)
#if defined(GITHUB_ACTIONS) && defined(_MSC_VER) && defined(_DEBUG)
#if _MSC_VER >= 1930
#include <corecrt.h>
#endif
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#undef B0

// Qt includes
#include <QResource>

#include <hyperion/Hyperion.h>

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

	// Stop all effects when instance is disabled, restart the same when enabled
	connect(_hyperion, &Hyperion::compStateChangeRequest, this, [=](hyperion::Components component, bool enable) {
		if (component == hyperion::COMP_ALL)
		{
			if (enable)
			{
				startCachedEffects();
			}
			else
			{
				cacheRunningEffects();
				stopAllEffects();
			}
		}
	});

	// register smooth cfgs and fill available effects
	handleUpdatedEffectList();
}

EffectEngine::~EffectEngine()
{
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
	_activeEffects.push_back(effect);

	// start the effect
	Debug(_log, "Start the effect: \"%s\"", QSTRING_CSTR(name));
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

void EffectEngine::stopAllEffects()
{
	_remainingEffects = _activeEffects.size();

	for (auto it = _activeEffects.begin(); it != _activeEffects.end();)
	{
		Effect* effect = *it;

		Debug(_log, "Currently active effects: %d", _remainingEffects);

		if (effect)
		{
			connect(effect, &Effect::finished, this, &EffectEngine::onEffectFinished, Qt::QueuedConnection);
			QMetaObject::invokeMethod(effect, "stop", Qt::QueuedConnection);

			effect->requestInterruption();

			// Wait for the thread to finish
			if (effect->isRunning())
			{
				effect->wait();
			}

			// Check if the effect has already been cleaned up in effectFinished
			if (std::find(_activeEffects.begin(), _activeEffects.end(), effect) != _activeEffects.end())
			{
				// Remove from list, cleanup handled in effectFinished
				it = _activeEffects.erase(it);
			}
			else
			{
				++it; // Effect was already handled, move to the next
			}
		}
		else
		{
			++it;
		}
	}

	// Wait until all instances have finished
	waitForEffectsToStop();

	Debug(_log, "All effects are stopped");
}

void EffectEngine::effectFinished()
{
	Effect* effect = qobject_cast<Effect*>(sender());
	if (!effect)
		return;

	Info(_log, "Effect \"%s\" finished", QSTRING_CSTR(effect->getName()));

	auto it = std::find(_activeEffects.begin(), _activeEffects.end(), effect);
	if (it != _activeEffects.end())
	{
		_activeEffects.erase(it);
		effect->deleteLater();
	}
}

void EffectEngine::onEffectFinished()
{
	// Decrement the count of remaining effects
	if (--_remainingEffects == 0)
	{
		// All effects have finished, exit the loop
		_eventLoop.quit();

	}

	emit isStopCompleted();
}

void EffectEngine::waitForEffectsToStop()
{
	// Only wait if there are instances to wait for
	if (_remainingEffects > 0)
	{
		Debug(_log, "%d effect(s) still running. Wait for completion...", _remainingEffects);
		_eventLoop.exec();  // Blocks until all effects are done
	}
	else
	{
		Debug(_log, "No effects currently running");
		emit isStopCompleted();
	}
}
