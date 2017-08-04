#pragma once

// QT include
#include <QString>
#include <QJsonObject>

struct EffectDefinition
{
	QString name, script, file;
	QJsonObject args;
	unsigned smoothCfg;
};
