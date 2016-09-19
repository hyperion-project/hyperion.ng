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
#include <utils/FileUtils.h>

// effect engine includes
#include <effectengine/EffectEngine.h>
#include "Effect.h"
#include "HyperionConfig.h"

EffectEngine::EffectEngine(Hyperion * hyperion, const Json::Value & jsonEffectConfig)
	: _hyperion(hyperion)
	, _availableEffects()
	, _activeEffects()
	, _mainThreadState(nullptr)
	, _log(Logger::getInstance("EFFECTENGINE"))
{
	Q_INIT_RESOURCE(EffectEngine);
	qRegisterMetaType<std::vector<ColorRgb>>("std::vector<ColorRgb>");

	// connect the Hyperion channel clear feedback
	connect(_hyperion, SIGNAL(channelCleared(int)), this, SLOT(channelCleared(int)));
	connect(_hyperion, SIGNAL(allChannelsCleared()), this, SLOT(allChannelsCleared()));

	// read all effects
	const Json::Value & paths       = jsonEffectConfig["paths"];
	const Json::Value & disabledEfx = jsonEffectConfig["disable"];

	QStringList efxPathList;
	efxPathList << ":/effects/";
	for (Json::UInt i = 0; i < paths.size(); ++i)
	{
		efxPathList << QString::fromStdString(paths[i].asString());
	}

	QStringList disableList;
	for (Json::UInt i = 0; i < disabledEfx.size(); ++i)
	{
		disableList << QString::fromStdString(disabledEfx[i].asString());
	}
	
	std::map<std::string, EffectDefinition> availableEffects;
	foreach (const QString & path, efxPathList )
	{
		QDir directory(path);
		if (directory.exists())
		{
			int efxCount = 0;
			QStringList filenames = directory.entryList(QStringList() << "*.json", QDir::Files, QDir::Name | QDir::IgnoreCase);
			foreach (const QString & filename, filenames)
			{
				EffectDefinition def;
				if (loadEffectDefinition(path, filename, def))
				{
					if (availableEffects.find(def.name) != availableEffects.end())
					{
						Info(_log, "effect overload effect '%s' is now taken from %s'", def.name.c_str(), path.toUtf8().constData() );
					}

					if ( disableList.contains(QString::fromStdString(def.name)) )
					{
						Info(_log, "effect '%s' not loaded, because it is disabled in hyperion config", def.name.c_str());
					}
					else
					{
						availableEffects[def.name] = def;
						efxCount++;
					}
				}
			}
			Info(_log, "%d effects loaded from directory %s", efxCount, path.toUtf8().constData());
		}
	}

	foreach(auto item,  availableEffects)
	{
		_availableEffects.push_back(item.second);
	}
	
	if (_availableEffects.size() == 0)
	{
		Error(_log, "no effects found, check your effect directories");
	}

	// initialize the python interpreter
	Debug(_log,"Initializing Python interpreter");
    Effect::registerHyperionExtensionModule();
	Py_InitializeEx(0);
	PyEval_InitThreads(); // Create the GIL
	_mainThreadState = PyEval_SaveThread();
}

EffectEngine::~EffectEngine()
{
	// clean up the Python interpreter
	Debug(_log, "Cleaning up Python interpreter");
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
		activeEffectDefinition.script = effect->getScript().toStdString();
		activeEffectDefinition.name = effect->getName().toStdString();
		activeEffectDefinition.priority = effect->getPriority();
		activeEffectDefinition.timeout = effect->getTimeout();
		activeEffectDefinition.args = effect->getArgs();
		_availableActiveEffects.push_back(activeEffectDefinition);
	}
  
	return _availableActiveEffects;
}

bool EffectEngine::loadEffectDefinition(const QString &path, const QString &effectConfigFile, EffectDefinition & effectDefinition)
{
	QString fileName = path + QDir::separator() + effectConfigFile;
	QFile file(fileName);

	Logger * log = Logger::getInstance("EFFECTENGINE");
	if (!file.open(QIODevice::ReadOnly))
	{
		Error( log, "Effect file '%s' could not be loaded", fileName.toUtf8().constData());
		return false;
	}
	QByteArray fileContent = file.readAll();
	// Read the json config file
	Json::Reader jsonReader;
	Json::Value config;
	const char* fileContent_cStr = reinterpret_cast<const char *>(fileContent.constData());
	
	if (! Json::Reader().parse(fileContent_cStr, fileContent_cStr+fileContent.size(), config, false) )
	{
		Error( log, "Error while reading effect '%s': %s", fileName.toUtf8().constData(), jsonReader.getFormattedErrorMessages().c_str());
		return false;
	}
	file.close();

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
			Error( log, "Error while checking '%s':%s", fileName.toUtf8().constData(), error.c_str());
		}
		return false;
	}

	// setup the definition
	std::string scriptName = config["script"].asString();
	effectDefinition.name = config["name"].asString();
	if (scriptName.empty())
		return false;

	if (scriptName[0] == ':' )
		effectDefinition.script = ":/effects/"+scriptName.substr(1);
	else
		effectDefinition.script = path.toStdString() + QDir::separator().toLatin1() + scriptName;
		
	effectDefinition.args = config["args"];

	return true;
}

int EffectEngine::runEffect(const std::string &effectName, int priority, int timeout)
{
	return runEffect(effectName, Json::Value(Json::nullValue), priority, timeout);
}

int EffectEngine::runEffect(const std::string &effectName, const Json::Value &args, int priority, int timeout)
{
	Info( _log, "run effect %s on channel %d", effectName.c_str(), priority);

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
		Error(_log, "effect %s not found",  effectName.c_str());
		return -1;
	}

	return runEffectScript(effectDefinition->script, effectName, args.isNull() ? effectDefinition->args : args, priority, timeout);
}

int EffectEngine::runEffectScript(const std::string &script, const std::string &name, const Json::Value &args, int priority, int timeout)
{
	// clear current effect on the channel
	channelCleared(priority);

	// create the effect
    Effect * effect = new Effect(_mainThreadState, priority, timeout, QString::fromStdString(script), QString::fromStdString(name), args);
	connect(effect, SIGNAL(setColors(int,std::vector<ColorRgb>,int,bool)), _hyperion, SLOT(setColors(int,std::vector<ColorRgb>,int,bool)), Qt::QueuedConnection);
	connect(effect, SIGNAL(effectFinished(Effect*)), this, SLOT(effectFinished(Effect*)));
	_activeEffects.push_back(effect);

	// start the effect
	_hyperion->registerPriority("EFFECT: "+name, priority);
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

	Info( _log, "effect finished");
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
	_hyperion->unRegisterPriority("EFFECT: " + effect->getName().toStdString());
}
