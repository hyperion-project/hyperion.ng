#ifndef BGEFFECTHANDLER_H
#define BGEFFECTHANDLER_H

#include <utils/Logger.h>
#include <utils/settings.h>

#include <QObject>
#include <QJsonDocument>

class Hyperion;

///
/// @brief Handle the background Effect settings, reacts on runtime to settings changes
///
class BGEffectHandler : public QObject
{
	Q_OBJECT

public:
	explicit BGEffectHandler(const QSharedPointer<Hyperion>& hyperionInstance);
	~BGEffectHandler() override;

	///
	/// @brief Stop BGEffect Handler (disconnect from connected signals)
	/// Disconnect should be done before other priorities invoke methods
	/// during shutdown
	///
	void stop() const;

	///
	/// @brief Check, if background effect processing is enabled.
	/// @return True, background effect processing is enabled.
	///
	bool _isEnabled () const;

private slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

	///
	/// @brief Handle priority updates.
	/// In case the background effect is not current priority, stop BG-effect to save resources; otherwise start effect again.
	///
	void handlePriorityUpdate(int currentPriority);

private:
	/// Hyperion instance pointer
	QWeakPointer<Hyperion> _hyperionWeak;
	Logger * _log;

	QJsonDocument _bgEffectConfig;
	bool _isBgEffectEnabled;
};

#endif // BGEFFECTHANDLER_H
