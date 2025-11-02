#include <hyperion/BGEffectHandler.h>

#include <hyperion/Hyperion.h>
#include <hyperion/PriorityMuxer.h>

#include <effectengine/Effect.h>

BGEffectHandler::BGEffectHandler(const QSharedPointer<Hyperion>& hyperionInstance)
	: QObject()
	, _hyperionWeak(hyperionInstance)
	, _isBgEffectEnabled(false)
{
	QString subComponent{ "__" };
	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if (hyperion)
	{
		 subComponent = hyperion->property("instance").toString();
	}
	_log = Logger::getInstance("HYPERION", subComponent);
	TRACK_SCOPE_SUBCOMPONENT;

	if (hyperion)
	{
		QObject::connect(hyperion.get(), &Hyperion::settingsChanged, this, &BGEffectHandler::handleSettingsUpdate);
		QObject::connect(hyperion->getMuxerInstance().get(), &PriorityMuxer::prioritiesChanged, this, &BGEffectHandler::handlePriorityUpdate);
	}

	// initialization
	handleSettingsUpdate(settings::BGEFFECT, hyperion->getSetting(settings::BGEFFECT));
}

BGEffectHandler::~BGEffectHandler()
{
	TRACK_SCOPE_SUBCOMPONENT;
}

void BGEffectHandler::stop() const
{
	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if (hyperion)
	{
		QObject::disconnect(hyperion->getMuxerInstance().get(), &PriorityMuxer::prioritiesChanged, this, &BGEffectHandler::handlePriorityUpdate);
		QObject::disconnect(hyperion.get(), &Hyperion::settingsChanged, this, &BGEffectHandler::handleSettingsUpdate);
	}
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

		QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
		const QJsonObject& BGEffectConfig = _bgEffectConfig.object();

#define BGCONFIG_ARRAY bgColorConfig.toArray()
		// clear background priority
		if (hyperion->getCurrentPriority() == PriorityMuxer::BG_PRIORITY)
		{
			hyperion->clear(PriorityMuxer::BG_PRIORITY);
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
				QVector<ColorRgb> bg_color = {
					ColorRgb {
						static_cast<uint8_t>(BGCONFIG_ARRAY.at(0).toInt(0)),
						static_cast<uint8_t>(BGCONFIG_ARRAY.at(1).toInt(0)),
						static_cast<uint8_t>(BGCONFIG_ARRAY.at(2).toInt(0))
					}
				};
				hyperion->setColor(PriorityMuxer::BG_PRIORITY, bg_color);
				Info(_log,"Initial background color set (%d %d %d)",bg_color.at(0).red, bg_color.at(0).green, bg_color.at(0).blue);
			}
#if defined(ENABLE_EFFECTENGINE)
			else
			{
				int result = hyperion->setEffect(bgEffectConfig, PriorityMuxer::BG_PRIORITY, PriorityMuxer::ENDLESS);
				Info(_log,"Initial background effect '%s' %s", QSTRING_CSTR(bgEffectConfig), ((result == 0) ? "started" : "failed"));
			}
#endif
		}
#undef BGCONFIG_ARRAY
	}
}

void BGEffectHandler::handlePriorityUpdate(int currentPriority)
{
	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if (currentPriority < PriorityMuxer::BG_PRIORITY && hyperion->getMuxerInstance()->hasPriority(PriorityMuxer::BG_PRIORITY))
	{
		Debug(_log,"Stop background (color-) effect as it moved out of scope");
		hyperion->clear(PriorityMuxer::BG_PRIORITY);
	}
	// Do not start a background effect when the overall instance is disabled
	else if (hyperion->isComponentEnabled(hyperion::COMP_ALL))
	{
		if (currentPriority == PriorityMuxer::LOWEST_PRIORITY && _isBgEffectEnabled)
		{
			Debug(_log, "Start background (color-) effect as it moved in scope");
			emit handleSettingsUpdate(settings::BGEFFECT, _bgEffectConfig);
		}
	}
}
