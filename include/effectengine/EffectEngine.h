#pragma once

#include <hyperion/Hyperion.h>

// pre-declarioation
class Effect;
typedef struct _ts PyThreadState;

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

public:
	struct EffectDefinition
	{
		std::string script;
		std::string args;
	};

private slots:
	void effectFinished(Effect * effect);

private:
	Hyperion * _hyperion;

	std::map<std::string, EffectDefinition> _availableEffects;

	std::list<Effect *> _activeEffects;

	PyThreadState * _mainThreadState;
};
