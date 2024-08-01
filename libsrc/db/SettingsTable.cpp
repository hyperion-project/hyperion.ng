#include <db/SettingsTable.h>

#include <QDateTime>
#include <QJsonDocument>

SettingsTable::SettingsTable(quint8 instance, QObject* parent)
	: DBManager(parent)
	, _hyperion_inst(instance)
{
	setTable("settings");
	// create table columns
	createTable(QStringList()<<"type TEXT"<<"config TEXT"<<"hyperion_inst INTEGER"<<"updated_at TEXT");
}

bool SettingsTable::createSettingsRecord(const QString& type, const QString& config) const
{
	QVariantMap map;
	map["config"] = config;
	map["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

	VectorPair cond;
	cond.append(CPair("type",type));
	// when a setting is not global we are searching also for the instance
	if(!isSettingGlobal(type))
	{
		cond.append(CPair("AND hyperion_inst",_hyperion_inst));
	}
	return createRecord(cond, map);
}

bool SettingsTable::recordExist(const QString& type) const
{
	VectorPair cond;
	cond.append(CPair("type",type));
	// when a setting is not global we are searching also for the instance
	if(!isSettingGlobal(type))
	{
		cond.append(CPair("AND hyperion_inst",_hyperion_inst));
	}
	return recordExists(cond);
}

QJsonDocument SettingsTable::getSettingsRecord(const QString& type) const
{
	QVariantMap results;
	VectorPair cond;
	cond.append(CPair("type",type));
	// when a setting is not global we are searching also for the instance
	if(!isSettingGlobal(type))
	{
		cond.append(CPair("AND hyperion_inst",_hyperion_inst));
	}
	getRecord(cond, results, QStringList("config"));
	return QJsonDocument::fromJson(results["config"].toByteArray());
}

QString SettingsTable::getSettingsRecordString(const QString& type) const
{
	QVariantMap results;
	VectorPair cond;
	cond.append(CPair("type",type));
	// when a setting is not global we are searching also for the instance
	if(!isSettingGlobal(type))
	{
		cond.append(CPair("AND hyperion_inst",_hyperion_inst));
	}
	getRecord(cond, results, QStringList("config"));
	return results["config"].toString();
}

void SettingsTable::deleteInstance() const
{
	deleteRecord({{"hyperion_inst",_hyperion_inst}});
}

const QVector<QString>& SettingsTable::getGlobalSettingTypes() {
	static const QVector<QString> globalSettingTypes = {
		"jsonServer",
		"protoServer",
		"flatbufServer",
		"forwarder",
		"webConfig",
		"network",
		"framegrabber",
		"grabberV4L2",
		"grabberAudio",
		"osEvents",
		"cecEvents",
		"schedEvents",
		"general",
		"logger"
	};
	return globalSettingTypes;
}

bool SettingsTable::isSettingGlobal(const QString& type)
{
	return getGlobalSettingTypes().contains(type);
}
