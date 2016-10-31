#pragma once

// QT include
#include <QString>
#include <QJsonObject>

struct EffectSchema
{
	QString pyFile, schemaFile;
	QJsonObject pySchema;
};