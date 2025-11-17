// proj
#include <hyperion/SettingsManager.h>

#include <db/SettingsTable.h>
#include <utils/JsonUtils.h>
#include <utils/jsonschema/QJsonFactory.h>

#include <QPair>

using namespace semver;

SettingsManager::SettingsManager(quint8 instance, QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("SETTINGSMGR", "I" + QString::number(instance)))
	, _instance(instance)
	, _sTable(new SettingsTable(instance, this))
{
	TRACK_SCOPE_SUBCOMPONENT();
	_sTable->addMissingDefaults();
}

SettingsManager::~SettingsManager()
{
	TRACK_SCOPE_SUBCOMPONENT();
}

QJsonDocument SettingsManager::getSetting(settings::type type) const
{
	return getSetting(settings::typeToString(type));
}

QJsonDocument SettingsManager::getSetting(const QString& type) const
{
	return _sTable->getSettingsRecordJson(type);
}

QJsonObject SettingsManager::getSettings(const QStringList& filteredTypes ) const
{
	return _sTable->getSettings(filteredTypes);
}

 QJsonObject SettingsManager::getSettings(const QVariant& instance, const QStringList& filteredTypes ) const
{
	return _sTable->getSettings(instance, filteredTypes);
}

QPair<bool, QStringList> SettingsManager::saveSettings(const QJsonObject& config)
{
	QStringList errorList;
	for (auto &key : config.keys())
	{
		const QJsonValue configItem = config.value(key);
		const QString data = JsonUtils::jsonValueToQString(configItem);
		if (_sTable->getSettingsRecordString(key) != data)
		{
			if (!_sTable->createSettingsRecord(key, data))
			{
				errorList.append(QString("Failed to save configuration item: %1").arg(key));
				return qMakePair (false, errorList);
			}
			emit settingsChanged(settings::stringToType(key), QJsonDocument::fromVariant(configItem.toVariant()));
		}
	}
	return qMakePair (true, errorList );
}
