#pragma once

// QT include
#include <QString>
#include <QJsonObject>

struct EffectDefinition
{
	QString name;
	QString script;
	QString file;
	QJsonObject args;
	unsigned smoothCfg;
};
