// Qt includes
#include <QMetaType>

// Python includes
#include <Python.h>

// effect engine includes
#include <effectengine/EffectEngine.h>
#include "Effect.h"

//static PyThreadState *_mainThreadState = 0;

EffectEngine::EffectEngine(Hyperion * hyperion) :
	_hyperion(hyperion),
	_availableEffects(),
	_activeEffects(),
	_mainThreadState(nullptr)
{
	qRegisterMetaType<std::vector<ColorRgb>>("std::vector<ColorRgb>");

	// connect the Hyperion channel clear feedback
	connect(_hyperion, SIGNAL(channelCleared(int)), this, SLOT(channelCleared(int)));
	connect(_hyperion, SIGNAL(allChannelsCleared()), this, SLOT(allChannelsCleared()));

	// read all effects
	_availableEffects["test"] = "test.py";

	// initialize the python interpreter
	std::cout << "Initializing Python interpreter" << std::endl;
	Py_InitializeEx(0);
	PyEval_InitThreads(); // Create the GIL
	PyRun_SimpleString("print 'test'");
	_mainThreadState = PyEval_SaveThread();
}

EffectEngine::~EffectEngine()
{
	// clean up the Python interpreter
	std::cout << "Cleaning up Python interpreter" << std::endl;
	PyEval_RestoreThread(_mainThreadState);
	Py_Finalize();
}

std::list<std::string> EffectEngine::getEffects() const
{
	std::list<std::string> effectNames;
	foreach (auto entry, _availableEffects) {
		effectNames.push_back(entry.first);
	}
	return effectNames;
}

int EffectEngine::runEffect(const std::string &effectName, int priority, int timeout)
{
	std::cout << "run effect " << effectName << " on channel " << priority << std::endl;

	// clear current effect on the channel
	channelCleared(priority);

	// create the effect
	Effect * effect = new Effect(priority, timeout);
	connect(effect, SIGNAL(setColors(int,std::vector<ColorRgb>,int)), _hyperion, SLOT(setColors(int,std::vector<ColorRgb>,int)), Qt::QueuedConnection);
	connect(effect, SIGNAL(effectFinished(Effect*)), this, SLOT(effectFinished(Effect*)));
	_activeEffects.push_back(effect);

	// start the effect
	effect->start();

	return 0;
}

void EffectEngine::channelCleared(int priority)
{
	std::cout << "clear effect on channel " << priority << std::endl;
	for (Effect * effect : _activeEffects)
	{
		if (effect->getPriority() == priority)
		{
			effect->abort();
		}
	}
}

void EffectEngine::allChannelsCleared()
{
	std::cout << "clear effect on every channel" << std::endl;
	for (Effect * effect : _activeEffects)
	{
		effect->abort();
	}
}

void EffectEngine::effectFinished(Effect *effect)
{
	std::cout << "effect finished" << std::endl;
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
