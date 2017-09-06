// Python includes
#include <Python.h>
#undef B0

// Qt includes
#include <QResource>
#include <QMetaType>
#include <QFile>
#include <QDir>
#include <QMap>

// hyperion util includes
#include <utils/jsonschema/QJsonSchemaChecker.h>
#include <utils/FileUtils.h>
#include <utils/Components.h>

// effect engine includes
#include <effectengine/EffectEngine.h>
#include "Effect.h"
#include "HyperionConfig.h"

EffectEngine::EffectEngine(Hyperion * hyperion, const QJsonObject & jsonEffectConfig)
	: _hyperion(hyperion)
	, _effectConfig(jsonEffectConfig)
	, _availableEffects()
	, _activeEffects()
	, _mainThreadState(nullptr)
	, _log(Logger::getInstance("EFFECTENGINE"))
{
	Q_INIT_RESOURCE(EffectEngine);
	qRegisterMetaType<std::vector<ColorRgb>>("std::vector<ColorRgb>");
	qRegisterMetaType<hyperion::Components>("hyperion::Components");

	// connect the Hyperion channel clear feedback
	connect(_hyperion, SIGNAL(channelCleared(int)), this, SLOT(channelCleared(int)));
	connect(_hyperion, SIGNAL(allChannelsCleared()), this, SLOT(allChannelsCleared()));

	// read all effects
	readEffects();

	// initialize the python interpreter
	Debug(_log, "Initializing Python interpreter");
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

const std::list<ActiveEffectDefinition> &EffectEngine::getActiveEffects()
{
	_availableActiveEffects.clear();

	for (Effect * effect : _activeEffects)
	{
		ActiveEffectDefinition activeEffectDefinition;
		activeEffectDefinition.script   = effect->getScript();
		activeEffectDefinition.name     = effect->getName();
		activeEffectDefinition.priority = effect->getPriority();
		activeEffectDefinition.timeout  = effect->getTimeout();
		activeEffectDefinition.args     = effect->getArgs();
		_availableActiveEffects.push_back(activeEffectDefinition);
	}

	return _availableActiveEffects;
}

bool EffectEngine::loadEffectDefinition(const QString &path, const QString &effectConfigFile, EffectDefinition & effectDefinition)
{
	Logger * log = Logger::getInstance("EFFECTENGINE");

	QString fileName = path + QDir::separator() + effectConfigFile;
	QJsonParseError error;

	// ---------- Read the effect json config file ----------

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		Error( log, "Effect file '%s' could not be loaded", QSTRING_CSTR(fileName));
		return false;
	}

	QByteArray fileContent = file.readAll();
	QJsonDocument configEffect = QJsonDocument::fromJson(fileContent, &error);

	if (error.error != QJsonParseError::NoError)
	{
		// report to the user the failure and their locations in the document.
		int errorLine(0), errorColumn(0);

		for( int i=0, count=qMin( error.offset,fileContent.size()); i<count; ++i )
		{
			++errorColumn;
			if(fileContent.at(i) == '\n' )
			{
				errorColumn = 0;
				++errorLine;
			}
		}

		Error( log, "Error while reading effect: '%s' at Line: '%i' , Column: %i",QSTRING_CSTR( error.errorString()), errorLine, errorColumn);
	}

	file.close();

	// ---------- Read the effect json schema file ----------

	Q_INIT_RESOURCE(EffectEngine);
	QFile schema(":effect-schema");

	if (!schema.open(QIODevice::ReadOnly))
	{
		Error( log, "Schema not found:  %s", QSTRING_CSTR(schema.errorString()));
		return false;
	}

	QByteArray schemaContent = schema.readAll();
	QJsonDocument configSchema = QJsonDocument::fromJson(schemaContent, &error);

	if (error.error != QJsonParseError::NoError)
	{
		// report to the user the failure and their locations in the document.
		int errorLine(0), errorColumn(0);

		for( int i=0, count=qMin( error.offset,schemaContent.size()); i<count; ++i )
		{
			++errorColumn;
			if(schemaContent.at(i) == '\n' )
			{
				errorColumn = 0;
				++errorLine;
			}
		}

		Error( log, "ERROR: Json schema wrong: '%s' at Line: '%i' , Column: %i", QSTRING_CSTR(error.errorString()), errorLine, errorColumn);
	}

	schema.close();

	// ---------- validate effect config with effect schema ----------

	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(configSchema.object());
	if (!schemaChecker.validate(configEffect.object()).first)
	{
		const QStringList & errors = schemaChecker.getMessages();
		for (auto & error : errors)
		{
			Error( log, "Error while checking '%s':%s", QSTRING_CSTR(fileName), QSTRING_CSTR(error));
		}
		return false;
	}

	// ---------- setup the definition ----------

	effectDefinition.file = fileName;
	QJsonObject config = configEffect.object();
	QString scriptName = config["script"].toString();
	effectDefinition.name = config["name"].toString();
	if (scriptName.isEmpty())
		return false;

	QFile fileInfo(scriptName);

	if (scriptName.mid(0, 1)  == ":" )
	{
		(!fileInfo.exists())
		? effectDefinition.script = ":/effects/"+scriptName.mid(1)
		: effectDefinition.script = scriptName;
	} else
	{
		(!fileInfo.exists())
		? effectDefinition.script = path + QDir::separator() + scriptName
		: effectDefinition.script = scriptName;
	}

	effectDefinition.args = config["args"].toObject();
	effectDefinition.smoothCfg = SMOOTHING_MODE_PAUSE;
	if (effectDefinition.args["smoothing-custom-settings"].toBool())
	{
		effectDefinition.smoothCfg = _hyperion->addSmoothingConfig(
			effectDefinition.args["smoothing-time_ms"].toInt(),
			effectDefinition.args["smoothing-updateFrequency"].toDouble(),
			0 );
	}
	else
	{
		effectDefinition.smoothCfg = _hyperion->addSmoothingConfig(true);
	}
	return true;
}

bool EffectEngine::loadEffectSchema(const QString &path, const QString &effectSchemaFile, EffectSchema & effectSchema)
{
	Logger * log = Logger::getInstance("EFFECTENGINE");

	QString fileName = path + "schema/" + QDir::separator() + effectSchemaFile;
	QJsonParseError error;

	// ---------- Read the effect schema file ----------

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		Error( log, "Effect schema '%s' could not be loaded", QSTRING_CSTR(fileName));
		return false;
	}

	QByteArray fileContent = file.readAll();
	QJsonDocument schemaEffect = QJsonDocument::fromJson(fileContent, &error);

	if (error.error != QJsonParseError::NoError)
	{
		// report to the user the failure and their locations in the document.
		int errorLine(0), errorColumn(0);

		for( int i=0, count=qMin( error.offset,fileContent.size()); i<count; ++i )
		{
			++errorColumn;
			if(fileContent.at(i) == '\n' )
			{
				errorColumn = 0;
				++errorLine;
			}
		}

		Error( log, "Error while reading effect schema: '%s' at Line: '%i' , Column: %i", QSTRING_CSTR(error.errorString()), errorLine, errorColumn);
		return false;
	}

	file.close();

	// ---------- setup the definition ----------

	QJsonObject tempSchemaEffect = schemaEffect.object();
	QString scriptName = tempSchemaEffect["script"].toString();
	effectSchema.schemaFile = fileName;
	fileName = path + QDir::separator() + scriptName;
	QFile pyFile(fileName);

	if (scriptName.isEmpty() || !pyFile.open(QIODevice::ReadOnly))
	{
		fileName = path + "schema/" + QDir::separator() + effectSchemaFile;
		Error( log, "Python script '%s' in effect schema '%s' could not be loaded", QSTRING_CSTR(scriptName), QSTRING_CSTR(fileName));
		return false;
	}

	pyFile.close();

	effectSchema.pyFile = (scriptName.mid(0, 1)  == ":" ) ? ":/effects/"+scriptName.mid(1) : path + QDir::separator() + scriptName;
	effectSchema.pySchema = tempSchemaEffect;

	return true;
}

void EffectEngine::readEffects()
{
	// clear all lists
	_availableEffects.clear();
	_effectSchemas.clear();

	// read all effects
	const QJsonArray & paths       = _effectConfig["paths"].toArray();
	const QJsonArray & disabledEfx = _effectConfig["disable"].toArray();

	QStringList efxPathList;
	efxPathList << ":/effects/";
	QStringList disableList;

	for(auto p : paths)
	{
		efxPathList << p.toString();
	}
	for(auto efx : disabledEfx)
	{
		disableList << efx.toString();
	}

	QMap<QString, EffectDefinition> availableEffects;
	for (const QString & path : efxPathList )
	{
		QDir directory(path);
		if (!directory.exists())
		{
			if(directory.mkpath(path))
			{
				Warning(_log, "New Effect path \"%s\" created successfull", QSTRING_CSTR(path) );
			}
			else
			{
				Warning(_log, "Failed to create Effect path \"%s\", please check permissions", QSTRING_CSTR(path) );
			}
		}
		else
		{
			int efxCount = 0;
			QStringList filenames = directory.entryList(QStringList() << "*.json", QDir::Files, QDir::Name | QDir::IgnoreCase);
			for (const QString & filename : filenames)
			{
				EffectDefinition def;
				if (loadEffectDefinition(path, filename, def))
				{
					InfoIf(availableEffects.find(def.name) != availableEffects.end(), _log,
						"effect overload effect '%s' is now taken from %s'", QSTRING_CSTR(def.name), QSTRING_CSTR(path) );

					if ( disableList.contains(def.name) )
					{
						Info(_log, "effect '%s' not loaded, because it is disabled in hyperion config", QSTRING_CSTR(def.name));
					}
					else
					{
						availableEffects[def.name] = def;
						efxCount++;
					}
				}
			}
			Info(_log, "%d effects loaded from directory %s", efxCount, QSTRING_CSTR(path));

			// collect effect schemas
			efxCount = 0;
			directory = path + "schema/";
			QStringList pynames = directory.entryList(QStringList() << "*.json", QDir::Files, QDir::Name | QDir::IgnoreCase);
			for (const QString & pyname : pynames)
			{
				EffectSchema pyEffect;
				if (loadEffectSchema(path, pyname, pyEffect))
				{
					_effectSchemas.push_back(pyEffect);
					efxCount++;
				}
			}
			InfoIf(efxCount > 0, _log, "%d effect schemas loaded from directory %s", efxCount, QSTRING_CSTR((path + "schema/")));
		}
	}

	for(auto item : availableEffects)
	{
		_availableEffects.push_back(item);
	}

	ErrorIf(_availableEffects.size()==0, _log, "no effects found, check your effect directories");
}

int EffectEngine::runEffect(const QString &effectName, int priority, int timeout, const QString &origin)
{
	return runEffect(effectName, QJsonObject(), priority, timeout, "", origin);
}

int EffectEngine::runEffect(const QString &effectName, const QJsonObject &args, int priority, int timeout, const QString &pythonScript, const QString &origin, unsigned smoothCfg)
{
	Info( _log, "run effect %s on channel %d", QSTRING_CSTR(effectName), priority);

	if (pythonScript.isEmpty())
	{
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
			Error(_log, "effect %s not found",  QSTRING_CSTR(effectName));
			return -1;
		}

		return runEffectScript(effectDefinition->script, effectName, (args.isEmpty() ? effectDefinition->args : args), priority, timeout, origin, effectDefinition->smoothCfg);
	}
	return runEffectScript(pythonScript, effectName, args, priority, timeout, origin, smoothCfg);
}

int EffectEngine::runEffectScript(const QString &script, const QString &name, const QJsonObject &args, int priority, int timeout, const QString & origin, unsigned smoothCfg)
{
	// clear current effect on the channel
	channelCleared(priority);

	// create the effect
    Effect * effect = new Effect(_mainThreadState, priority, timeout, script, name, args, origin, smoothCfg);
	connect(effect, SIGNAL(setColors(int,std::vector<ColorRgb>,int,bool,hyperion::Components,const QString,unsigned)), _hyperion, SLOT(setColors(int,std::vector<ColorRgb>,int,bool,hyperion::Components,const QString,unsigned)), Qt::QueuedConnection);
	connect(effect, SIGNAL(effectFinished(Effect*)), this, SLOT(effectFinished(Effect*)));
	_activeEffects.push_back(effect);

	// start the effect
	_hyperion->registerPriority(name, priority);
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
	_hyperion->unRegisterPriority(effect->getName());
}
