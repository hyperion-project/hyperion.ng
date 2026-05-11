#pragma once

// QT include
#include <QString>
#include <QJsonObject>

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(effect);

struct EffectDefinition
{
	QString name;
	QString script;
	QString file;
	QJsonObject args;
	unsigned smoothCfg;
};
