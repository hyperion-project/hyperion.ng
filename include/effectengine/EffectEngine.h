#pragma once

#include <hyperion/Hyperion.h>

// pre-declarioation
class Effect;

class EffectEngine : public QObject
{
	Q_OBJECT

public:
	EffectEngine(Hyperion * hyperion);
	virtual ~EffectEngine();

	std::list<std::string> getEffects() const;

public slots:
	/// Run the specified effect on the given priority channel and optionally specify a timeout
	int runEffect(const std::string &effectName, int priority, int timeout = -1);

	/// Clear any effect running on the provided channel
	void channelCleared(int priority);

	/// Clear all effects
	void allChannelsCleared();

private slots:
	void effectFinished(Effect * effect);

private:
	Hyperion * _hyperion;

	std::map<std::string, std::string> _availableEffects;

	std::list<Effect *> _activeEffects;
};
