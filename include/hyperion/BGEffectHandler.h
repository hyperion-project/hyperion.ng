#ifndef BGEFFECTHANDLER_H
#define BGEFFECTHANDLER_H

#include <utils/Logger.h>
#include <hyperion/Hyperion.h>
#include <utils/settings.h>
#include <effectengine/Effect.h>
#include <hyperion/PriorityMuxer.h>

///
/// @brief Handle the background Effect settings, reacts on runtime to settings changes
///
class BGEffectHandler : public QObject
{
	Q_OBJECT

public:
	BGEffectHandler(Hyperion* hyperion)
	: QObject(hyperion)
	, _hyperion(hyperion)
	, _prioMuxer(_hyperion->getMuxerInstance())
	, _isBgEffectEnabled(false)
	, _isSuspended(false)
	{
		QString subComponent = parent()->property("instance").toString();
		_log = Logger::getInstance("HYPERION", subComponent);

		// listen for config changes
		connect(_hyperion, &Hyperion::settingsChanged, this, [=] (settings::type type, const QJsonDocument& config) {
			this->handleSettingsUpdate(type, config);
		});

		connect(_prioMuxer, &PriorityMuxer::prioritiesChanged, this, [=] {
			this->handlePriorityUpdate();
		});

		// listen for suspend/resume requests, to not start a background effect when system goes into suspend mode
		connect(_hyperion, &Hyperion::suspendRequest, this, [=] (bool isSuspended) {
			_isSuspended = isSuspended;
		});

		// initialization
		handleSettingsUpdate(settings::BGEFFECT, _hyperion->getSetting(settings::BGEFFECT));
	}

	///
	/// @brief Check, if background effect processing is enabled.
	/// @return True, background effect processing is enabled.
	///
	bool _isEnabled () const { return _isBgEffectEnabled; }

private slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config)
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

	///
	/// @brief Handle priority updates.
	/// In case the background effect is not current priority, stop BG-effect to save resources; otherwise start effect again.
	///
	void handlePriorityUpdate()
	{
		if (_prioMuxer->getCurrentPriority() < PriorityMuxer::BG_PRIORITY && _prioMuxer->hasPriority(PriorityMuxer::BG_PRIORITY))
		{
			Debug(_log,"Stop background (color-) effect as it moved out of scope");
			_hyperion->clear(PriorityMuxer::BG_PRIORITY);
		}
		else if (!_isSuspended && _prioMuxer->getCurrentPriority() == PriorityMuxer::LOWEST_PRIORITY && _isBgEffectEnabled)
		{
			Debug(_log,"Start background (color-) effect as it moved in scope");
			emit handleSettingsUpdate (settings::BGEFFECT, _bgEffectConfig);
		}
	}

private:
	/// Hyperion instance pointer
	Hyperion* _hyperion;
	Logger * _log;

	/// priority muxer instance
	PriorityMuxer* _prioMuxer;

	QJsonDocument _bgEffectConfig;
	bool _isBgEffectEnabled;

	bool _isSuspended;
};

#endif // BGEFFECTHANDLER_H
