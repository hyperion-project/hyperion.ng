#pragma once

// QT include
#include <QString>
#include <QJsonObject>

struct ActiveEffectDefinition
{
	QString script;
	QString name;
	int priority;
	int timeout;
	QJsonObject args;
};
