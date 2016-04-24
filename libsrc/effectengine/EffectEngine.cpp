// Python includes
#include <Python.h>

// Stl includes
#include <fstream>

// Qt includes
#include <QResource>
#include <QMetaType>
#include <QFile>
#include <QDir>

// hyperion util includes
#include <utils/jsonschema/JsonSchemaChecker.h>

// effect engine includes
#include <effectengine/EffectEngine.h>
#include "Effect.h"

EffectEngine::EffectEngine(Hyperion * hyperion, const Json::Value & jsonEffectConfig) :
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
	const Json::Value & paths = jsonEffectConfig["paths"];
	for (Json::UInt i = 0; i < paths.size(); ++i)
	{
		const std::string & path = paths[i].asString();
		QDir directory(QString::fromStdString(path));
		if (!directory.exists())
		{
			std::cerr << "EFFECTENGINE ERROR: Effect directory can not be loaded: " << path << std::endl;
			continue;
		}

		QStringList filenames = directory.entryList(QStringList() << "*.json", QDir::Files, QDir::Name | QDir::IgnoreCase);
		foreach (const QString & filename, filenames)
		{
			EffectDefinition def;
			if (loadEffectDefinition(path, filename.toStdString(), def))
			{
				_availableEffects.push_back(def);
			}
		}
	}

	// initialize the python interpreter
	std::cout << "EFFECTENGINE INFO: Initializing Python interpreter" << std::endl;
    Effect::registerHyperionExtensionModule();
	Py_InitializeEx(0);
	PyEval_InitThreads(); // Create the GIL
	_mainThreadState = PyEval_SaveThread();
}

EffectEngine::~EffectEngine()
{
	// clean up the Python interpreter
	std::cout << "EFFECTENGINE INFO: Cleaning up Python interpreter" << std::endl;
	PyEval_RestoreThread(_mainThreadState);
	Py_Finalize();
}

const std::list<EffectDefinition> &EffectEngine::getEffects() const
{
	return _availableEffects;
}

const std::list<ActiveEffectDefinition> &EffectEngine::getActiveEffects()
{
	_availableActiveEffects.clear();
	
	for (Effect * effect : _activeEffects)
	{
		ActiveEffectDefinition activeEffectDefinition;
		activeEffectDefinition.script = effect->getScript();
		activeEffectDefinition.priority = effect->getPriority();
		activeEffectDefinition.timeout = effect->getTimeout();
		activeEffectDefinition.args = effect->getArgs();
		_availableActiveEffects.push_back(activeEffectDefinition);
	}
  
	return _availableActiveEffects;
}

bool EffectEngine::loadEffectDefinition(const std::string &path, const std::string &effectConfigFile, EffectDefinition & effectDefinition)
{
#ifdef ENABLE_QT5
	std::string fileName = path + QDir::separator().toLatin1() + effectConfigFile;
#else
	std::string fileName = path + QDir::separator().toAscii() + effectConfigFile;
#endif
	std::ifstream file(fileName.c_str());

	if (!file.is_open())
	{
		std::cerr << "EFFECTENGINE ERROR: Effect file '" << fileName << "' could not be loaded" << std::endl;
		return false;
	}

	// Read the json config file
	Json::Reader jsonReader;
	Json::Value config;
	if (!jsonReader.parse(file, config, false))
	{
		std::cerr << "EFFECTENGINE ERROR: Error while reading effect '" << fileName << "': " << jsonReader.getFormattedErrorMessages() << std::endl;
		return false;
	}

	// Read the json schema file
	QResource schemaData(":effect-schema");
	JsonSchemaChecker schemaChecker;
	Json::Value schema;
	Json::Reader().parse(reinterpret_cast<const char *>(schemaData.data()), reinterpret_cast<const char *>(schemaData.data()) + schemaData.size(), schema, false);
	schemaChecker.setSchema(schema);
	if (!schemaChecker.validate(config))
	{
		const std::list<std::string> & errors = schemaChecker.getMessages();
		foreach (const std::string & error, errors) {
			std::cerr << "EFFECTENGINE ERROR: Error while checking '" << fileName << "':" << error << std::endl;
		}
		return false;
	}

	// setup the definition
	effectDefinition.name = config["name"].asString();
#ifdef ENABLE_QT5
	effectDefinition.script = path + QDir::separator().toLatin1() + config["script"].asString();
#else
	effectDefinition.script = path + QDir::separator().toAscii() + config["script"].asString();
#endif
	effectDefinition.args = config["args"];

	// return succes //BLACKLIST OUTPUT TO LOG (Spam). This is more a effect development thing and the list gets longer and longer
//	std::cout << "EFFECTENGINE INFO: Effect loaded: " + effectDefinition.name << std::endl;
	return true;
}

int EffectEngine::runEffect(const std::string &effectName, int priority, int timeout)
{
	return runEffect(effectName, Json::Value(Json::nullValue), priority, timeout);
}

int EffectEngine::runEffect(const std::string &effectName, const Json::Value &args, int priority, int timeout)
{
	std::cout << "EFFECTENGINE INFO: run effect " << effectName << " on channel " << priority << std::endl;

	const EffectDefinition * effectDefinition = nullptr;
	for (const EffectDefinition & e : _availableEffects)
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
		std::cerr << "EFFECTENGINE ERROR: effect " << effectName << " not found" << std::endl;
		return -1;
	}

	return runEffectScript(effectDefinition->script, args.isNull() ? effectDefinition->args : args, priority, timeout);
}

int EffectEngine::runEffectScript(const std::string &script, const Json::Value &args, int priority, int timeout)
{
	// clear current effect on the channel
	channelCleared(priority);

	// create the effect
    Effect * effect = new Effect(_mainThreadState, priority, timeout, script, args);
	connect(effect, SIGNAL(setColors(int,std::vector<ColorRgb>,int,bool)), _hyperion, SLOT(setColors(int,std::vector<ColorRgb>,int,bool)), Qt::QueuedConnection);
	connect(effect, SIGNAL(effectFinished(Effect*)), this, SLOT(effectFinished(Effect*)));
	_activeEffects.push_back(effect);

	// start the effect
	effect->start();

	return 0;
}

void EffectEngine::channelCleared(int priority)
{
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
	for (Effect * effect : _activeEffects)
	{
		effect->abort();
	}
}

void EffectEngine::effectFinished(Effect *effect)
{
	if (!effect->isAbortRequested())
	{
		// effect stopped by itself. Clear the channel
		_hyperion->clear(effect->getPriority());
	}

	std::cout << "EFFECTENGINE INFO: effect finished" << std::endl;
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
