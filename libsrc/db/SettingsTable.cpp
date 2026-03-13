#include <db/SettingsTable.h>

#include <utils/jsonschema/QJsonFactory.h>
#include <utils/JsonUtils.h>

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>

namespace {
const char DEFAULT_INSTANCE_SETTINGS_SCHEMA_FILE[] = ":/schema-settings-instance.json";
const char GLOBAL_SETTINGS_SCHEMA_FILE[] = ":/schema-settings-global.json";
const char DEFAULT_SETTINGS_SCHEMA_FILE[] = ":/schema-settings-default.json";
const char DEFAULT_SETTINGS_CONFIGURATION_FILE[] = ":/hyperion_default.settings";
}

QVector<QString> SettingsTable::globalSettingTypes;
bool SettingsTable::areGlobalSettingTypesInitialised = false;

QVector<QString> SettingsTable::instanceSettingTypes;
bool SettingsTable::areInstanceSettingTypesInitialised = false;

QJsonObject SettingsTable::defaultSettings;
bool SettingsTable::areDefaultSettingsInitialised = false;

SettingsTable::SettingsTable(quint8 instance, QObject* parent)
	: DBManager("settings", parent)
	, _instance(instance)
	, _configVersion(DEFAULT_CONFIG_VERSION)
{
	QString const subComponent = _instance != NO_INSTANCE_ID ? "I" + QString::number(_instance) : "__";
	this->setProperty("instance", QVariant::fromValue(subComponent));
	TRACK_SCOPE_SUBCOMPONENT();
	_log = Logger::getInstance("DB-SETTINGS", subComponent);

	// create table columns
	createTable(QStringList()<<"type TEXT"<<"config TEXT"<<"hyperion_inst INTEGER"<<"updated_at TEXT");
}

const QVector<QString>& SettingsTable::getGlobalSettingTypes() const
{
	if (!areGlobalSettingTypesInitialised) {
		globalSettingTypes = initializeGlobalSettingTypes();
		areGlobalSettingTypesInitialised = true;
	}
	return globalSettingTypes;
}

QVector<QString> SettingsTable::initializeGlobalSettingTypes() const
{
	QJsonObject schemaJson;
	try
	{
		schemaJson = QJsonFactory::readSchema(GLOBAL_SETTINGS_SCHEMA_FILE);
	}
	catch (const std::runtime_error& error)
	{
		throw std::runtime_error(error.what());
	}

	const QVector<QString> types = schemaJson.value("properties").toObject().keys().toVector();
	return types;
}

bool SettingsTable::isGlobalSettingType(const QString& type) const {
	return getGlobalSettingTypes().contains(type);
}

bool SettingsTable::isInstanceSettingType(const QString& type) const {
	return getInstanceSettingTypes().contains(type);
}

const QVector<QString>& SettingsTable::getInstanceSettingTypes() const
{
	if (!areInstanceSettingTypesInitialised) {
		instanceSettingTypes = initializeInstanceSettingTypes();
		areInstanceSettingTypesInitialised = true;
	}
	return instanceSettingTypes;
}

QVector<QString> SettingsTable::initializeInstanceSettingTypes() const
{
	QJsonObject schemaJson;
	try
	{
		schemaJson = QJsonFactory::readSchema(DEFAULT_INSTANCE_SETTINGS_SCHEMA_FILE);
	}
	catch (const std::runtime_error& error)
	{
		throw std::runtime_error(error.what());
	}

	const QVector<QString> types = schemaJson.value("properties").toObject().keys().toVector();
	return types;
}

const QJsonObject& SettingsTable::getDefaultSettings() const
{
	if (!areDefaultSettingsInitialised) {
		defaultSettings = initializeDefaultSettings();
		areDefaultSettingsInitialised = true;
	}
	return defaultSettings;
}

QJsonObject SettingsTable::initializeDefaultSettings() const
{
	QJsonObject defaultConfig;
	if ( QJsonFactory::load(DEFAULT_SETTINGS_SCHEMA_FILE, DEFAULT_SETTINGS_CONFIGURATION_FILE, defaultConfig) < 0)
	{
		Error(_log,"Failed to read default config");
	}

	return defaultConfig;
}

bool SettingsTable::createSettingsRecord(const QString& type, const QString& config) const
{
	VectorPair cond;
	cond.append(CPair("type",type));

	QVariant instance {};
	if(!isGlobalSettingType(type))
	{
		instance = _instance;
	}
	cond.append(CPair("AND hyperion_inst", instance));

	QVariantMap map;
	map.insert("config", config);
	map.insert("updated_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
	map.insert("hyperion_inst", instance);

	return createRecord(cond, map);
}

bool SettingsTable::recordExist(const QString& type) const
{
	VectorPair cond;
	cond.append(CPair("type",type));

	QVariant instance {};
	if(!isGlobalSettingType(type))
	{
		instance = _instance;
	}
	cond.append(CPair("AND hyperion_inst", instance));

	return recordExists(cond);
}

QVariant SettingsTable::getSettingsRecord(const QString& type) const
{
	QVariantMap results;
	VectorPair cond;
	cond.append(CPair("type",type));

	QVariant instance {};
	if(!isGlobalSettingType(type))
	{
		instance.setValue(static_cast<int>(_instance));
	}
	cond.append(CPair("AND hyperion_inst", instance));

	getRecord(cond, results, QStringList("config"));
	return results.value("config");
}

QJsonDocument SettingsTable::getSettingsRecordJson(const QString& type) const
{
	return QJsonDocument::fromJson(getSettingsRecord(type).toByteArray());
}

QString SettingsTable::getSettingsRecordString(const QString& type) const
{
	return getSettingsRecord(type).toString();
}

QJsonObject SettingsTable::getSettings(const QString& filteredType) const
{
	return getSettings(_instance, {filteredType});
}

QJsonObject SettingsTable::getSettings(const QStringList& filteredTypes ) const
{
	return getSettings(_instance, filteredTypes);
}

QJsonObject SettingsTable::getSettings(const QVariant& instance, const QStringList& filteredTypes ) const
{
	if (filteredTypes.contains("__none__"))
	{
		return {};
	}

	QJsonObject settingsObject;
	QStringList settingsKeys({ "type", "config" });
	QString settingsCondition;
	QVariantList conditionValues;

	if (instance.isNull() || instance == NO_INSTANCE_ID )
	{
		settingsCondition = "hyperion_inst IS NULL";
	}
	else
	{
		settingsCondition = "hyperion_inst = ?";
		conditionValues.append(instance);
	}

	if (!filteredTypes.isEmpty())
	{
		QStringList seletedSettingTypes;
		for (const auto &type : filteredTypes) {
			seletedSettingTypes << QString("%1=?").arg("type");
			conditionValues.append(type);
		}
		settingsCondition += QString (" AND (%1)").arg(seletedSettingTypes.join(" OR "));
	}

	QVector<QVariantMap> settingsList;
	if (getRecords(settingsCondition, conditionValues, settingsList, settingsKeys))
	{
		for (const QVariantMap &setting : std::as_const(settingsList))
		{
			QString type = setting.value("type").toString();
			QByteArray configObject = setting.value("config").toByteArray();
			QJsonDocument jsonDoc = QJsonDocument::fromJson(configObject);

			if (!jsonDoc.isNull())
			{
				QJsonValue config;

				if (jsonDoc.isArray())
				{
					config = jsonDoc.array();
				}
				else if (jsonDoc.isObject())
				{
					config = jsonDoc.object();
				}
				settingsObject.insert(type, config);
			}
		}
	}
	return settingsObject;
}

QStringList SettingsTable::nonExtingTypes() const
{
	QStringList testTypes;
	QString condition {"hyperion_inst"};
	if(_instance == NO_INSTANCE_ID)
	{
		condition += " IS NULL";
		testTypes = getGlobalSettingTypes().toList();
	}
	else
	{
		condition += QString(" = %1").arg(_instance);
		testTypes = getInstanceSettingTypes().toList();
	}

	QVariantList testTypesList;
	testTypesList.reserve(testTypes.size());

	for (const QString &str : std::as_const(testTypes)) {
		testTypesList.append(QVariant(str));
	}

	QStringList nonExistingRecs;
	recordsNotExisting(testTypesList, "type", nonExistingRecs, condition );

	return nonExistingRecs;
}

QPair<bool, QStringList> SettingsTable::addMissingDefaults()
{
	QStringList errorList;

	QJsonObject defaultSettings;
	if (_instance == NO_INSTANCE_ID)
	{
		defaultSettings = getDefaultSettings().value("global").toObject();
	}
	else
	{
		defaultSettings = getDefaultSettings().value("instance").toObject();
	}

	const QStringList missingTypes = nonExtingTypes();
	if (missingTypes.empty())
	{
		Debug(_log, "%s settings: No missing configuration items identified", _instance == NO_INSTANCE_ID ? "Global" : QSTRING_CSTR(QString("Instance [%1]").arg(_instance)) );
		return qMakePair (true, errorList );
	}

	QSqlDatabase idb = getDB();
	if (!startTransaction(idb, errorList))
	{
		return qMakePair(false, errorList);
	}


	bool errorOccured {false};

	Info(_log, "%s settings: Add default settings for %d missing configuration items", _instance == NO_INSTANCE_ID ? "Global" : QSTRING_CSTR(QString("Instance [%1]").arg(_instance)), missingTypes.size() );
	for (const auto &missingType: missingTypes)
	{
		if (!createSettingsRecord(missingType, JsonUtils::jsonValueToQString(defaultSettings.value(missingType))))
		{
			errorOccured = true;
		}
	}

	if (errorOccured)
	{
		QString errorText = "Errors occured while adding missing settings to configuration items";
		Error(_log, "'%s'", QSTRING_CSTR(errorText));
		errorList.append(errorText);

		if (!idb.rollback())
		{
			errorText = QString("Could not create a database transaction. Error: %1").arg(idb.lastError().text());
			Error(_log, "'%s'", QSTRING_CSTR(errorText));
			errorList.append(errorText);
		}
	}

	commiTransaction(idb, errorList);

	if(errorList.isEmpty())
	{
		Debug(_log, "%s settings: Successfully defaulted settings for %d missing configuration items", _instance == NO_INSTANCE_ID ? "Global" : QSTRING_CSTR(QString("Instance [%1]").arg(_instance)), missingTypes.size() );
	}

	return qMakePair (errorList.isEmpty(), errorList );
}

void SettingsTable::deleteInstance() const
{
	deleteRecord({{"hyperion_inst",_instance}});
}

QString SettingsTable::fixVersion(const QString& version)
{
	QString newVersion;

	// Use a static QRegularExpression to avoid re-creating it every time
	static const QRegularExpression regEx(
				"(\\d+\\.\\d+\\.\\d+-?[a-zA-Z-\\d]*\\.?[\\d]*)",
				QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption
				);

	// Try fixing version number, remove dot-separated pre-release identifiers not supported
	QRegularExpressionMatch match = regEx.match(version);

	if (match.hasMatch())
	{
		newVersion = match.captured(1);
	}

	return newVersion;
}

bool SettingsTable::resolveConfigVersion()
{
	QJsonObject generalConfig = getSettingsRecordJson({"general"}).object();
	return resolveConfigVersion(generalConfig);
}

bool SettingsTable::resolveConfigVersion(QJsonObject generalConfig)
{
	bool isValid = false;

	QString configVersion = generalConfig["configVersion"].toString();
	if (!configVersion.isEmpty())
	{
		isValid = _configVersion.setVersion(configVersion.toStdString());
		if (!isValid)
		{
			isValid = _configVersion.setVersion(fixVersion(configVersion).toStdString());
			if (isValid)
			{
				Info(_log, "Invalid config version [%s] fixed. Updated to [%s]", QSTRING_CSTR(configVersion), _configVersion.getVersion().c_str());
			}
		}
	}
	else
	{
		isValid = true;
	}

	return isValid;
}

QString SettingsTable::getConfigVersionString()
{
	return _configVersion.getVersion().data();
}

semver::version SettingsTable::getConfigVersion()
{
	return _configVersion;
}
