#include <hyperion/BGEffectHandler.h>

#include <effectengine/Effect.h>

BGEffectHandler::BGEffectHandler(Hyperion* hyperion)
	: QObject(hyperion)
	, _hyperion(hyperion)
	, _isBgEffectEnabled(false)
	, _isSuspended(false)
{
	QString subComponent = parent()->property("instance").toString();
	_log = Logger::getInstance("HYPERION", subComponent);

	QObject::connect(_hyperion, &Hyperion::settingsChanged, this, &BGEffectHandler::handleSettingsUpdate);
	QObject::connect(_hyperion->getMuxerInstance(), &PriorityMuxer::prioritiesChanged, this, &BGEffectHandler::handlePriorityUpdate);

	// listen for suspend/resume requests, to not start a background effect when system goes into suspend mode
	connect(_hyperion, &Hyperion::suspendRequest, this, [=] (bool isSuspended) {
		_isSuspended = isSuspended;
	});

	// initialization
	handleSettingsUpdate(settings::BGEFFECT, _hyperion->getSetting(settings::BGEFFECT));
}

void BGEffectHandler::disconnect()
{
	QObject::disconnect(_hyperion->getMuxerInstance(), &PriorityMuxer::prioritiesChanged, this, &BGEffectHandler::handlePriorityUpdate);
	QObject::disconnect(_hyperion, &Hyperion::settingsChanged, this, &BGEffectHandler::handleSettingsUpdate);
}

bool BGEffectHandler::_isEnabled () const
{
	return _isBgEffectEnabled;
}

void BGEffectHandler::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::BGEFFECT)
	{
		_bgEffectConfig = config;

		const QJsonObject& BGEffectConfig = _bgEffectConfig.object();
#define BGCONFIG_ARRAY bgColorConfig.toArray()
		// clear background priority
		if (_hyperion->getCurrentPriority() == PriorityMuxer::BG_PRIORITY)
		{
			_hyperion->clear(PriorityMuxer::BG_PRIORITY);
		}
		_isBgEffectEnabled = BGEffectConfig["enable"].toBool(true);

		// initial background effect/color
		if (_isBgEffectEnabled)
		{
#if defined(ENABLE_EFFECTENGINE)
			const QString bgTypeConfig = BGEffectConfig["type"].toString("effect");
			const QString bgEffectConfig = BGEffectConfig["effect"].toString("Warm mood blobs");
#else
			const QString bgTypeConfig = "color";
#endif
			const QJsonValue bgColorConfig = BGEffectConfig["color"];
			if (bgTypeConfig.contains("color"))
			{
				std::vector<ColorRgb> bg_color = {
					ColorRgb {
						static_cast<uint8_t>(BGCONFIG_ARRAY.at(0).toInt(0)),
						static_cast<uint8_t>(BGCONFIG_ARRAY.at(1).toInt(0)),
						static_cast<uint8_t>(BGCONFIG_ARRAY.at(2).toInt(0))
					}
				};
				_hyperion->setColor(PriorityMuxer::BG_PRIORITY, bg_color);
				Info(_log,"Initial background color set (%d %d %d)",bg_color.at(0).red, bg_color.at(0).green, bg_color.at(0).blue);
			}
#if defined(ENABLE_EFFECTENGINE)
			else
			{
				int result = _hyperion->setEffect(bgEffectConfig, PriorityMuxer::BG_PRIORITY, PriorityMuxer::ENDLESS);
				Info(_log,"Initial background effect '%s' %s", QSTRING_CSTR(bgEffectConfig), ((result == 0) ? "started" : "failed"));
			}
#endif
		}
#undef BGCONFIG_ARRAY
	}
}

void BGEffectHandler::handlePriorityUpdate(int currentPriority)
{
	if (currentPriority < PriorityMuxer::BG_PRIORITY && _hyperion->getMuxerInstance()->hasPriority(PriorityMuxer::BG_PRIORITY))
	{
		Debug(_log,"Stop background (color-) effect as it moved out of scope");
		_hyperion->clear(PriorityMuxer::BG_PRIORITY);
	}
	else if (!_isSuspended && currentPriority == PriorityMuxer::LOWEST_PRIORITY && _isBgEffectEnabled)
	{
		Debug(_log,"Start background (color-) effect as it moved in scope");
		emit handleSettingsUpdate (settings::BGEFFECT, _bgEffectConfig);
	}
}
