
// db
#include <db/InstanceTable.h>
#include <db/SettingsTable.h>

// qt
#include <QDateTime>

InstanceTable::InstanceTable(QObject* parent)
	: DBManager(parent)
{
	// Init instance table
	setTable("instances");
	createTable(QStringList()<<"instance INTEGER"<<"friendly_name TEXT"<<"enabled INTEGER DEFAULT 0"<<"last_use TEXT");
}

bool InstanceTable::createInstance(const QString& name, quint8& inst)
{
	// check duplicate
	if(!recordExists({{"friendly_name", name}}))
	{
		QList<quint8> instanceList = getAllInstanceIDs(false);

		inst = 0;
		while (instanceList.contains(inst))
		{
			++inst;
		}

		// create
		QVariantMap data;
		data["friendly_name"] = name;
		data["instance"] = inst;
		return createRecord({}, data);
	}

	return false;
}

bool InstanceTable::deleteInstance(quint8 inst)
{
	Debug(_log,"");
	if(deleteRecord({{"instance",inst}}))
	{
		// delete settings entries
		SettingsTable settingsTable(inst);
		settingsTable.deleteInstance();
		return true;
	}
	return false;
}

bool InstanceTable::saveName(quint8 inst, const QString& name)
{
	// check duplicate
	if(!recordExists({{"friendly_name", name}}))
	{
		if(instanceExist(inst))
		{
			return updateRecord({{"instance",inst}}, {{"friendly_name", name}});
		}
	}
	return false;
}

QVector<QVariantMap> InstanceTable::getAllInstances(bool onlyEnabled)
{
	QVector<QVariantMap> results;

	VectorPair onlyEnabledCondition {};
	if (onlyEnabled)
	{
		onlyEnabledCondition = {{"enabled", true}};
	}
	getRecords(onlyEnabledCondition, results, {}, {"instance ASC"});
	return results;
}

QList<quint8> InstanceTable::getAllInstanceIDs (bool onlyEnabled)
{
	QVector<QVariantMap> instanceList = getAllInstances(onlyEnabled);
	QList<quint8> instanceIds;
	for (const QVariantMap &idx : std::as_const(instanceList))
	{
		instanceIds.append(static_cast<quint8>(idx.value("instance").toInt()));
	}

	return instanceIds;
}

bool InstanceTable::instanceExist(quint8 inst)
{
	return recordExists({{"instance",inst}});
}

QString InstanceTable::getNamebyIndex(quint8 index)
{
	QVariantMap results;
	getRecord({{"instance", index}}, results, {"friendly_name"});

	QString name = results["friendly_name"].toString();
	return name.isEmpty() ? "NOT FOUND" : name;
}

bool InstanceTable::setLastUse(quint8 inst)
{
	return updateRecord({{"instance", inst}}, {{"last_use", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}});
}

bool InstanceTable::setEnable(quint8 inst, bool newState)
{
	return updateRecord({{"instance", inst}}, {{"enabled", newState}});
}

bool InstanceTable::isEnabled(quint8 inst)
{
	QVariantMap results;
	getRecord({{"instance", inst}}, results);

	return results["enabled"].toBool();
}

void InstanceTable::createDefaultInstance()
{
	if(instanceExist(0))
	{
		setEnable(0, true);
	}
	else
	{
		if(createRecord({{"instance", 0}}, {{"friendly_name", "First LED Hardware instance"}}))
		{
			setEnable(0, true);
		}
		else
		{
			throw std::runtime_error("Failed to create Hyperion root instance in db! This should never be the case...");
		}
	}
}


