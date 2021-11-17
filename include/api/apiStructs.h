#pragma once

#include <QString>
#include <QByteArray>

struct ImageCmdData
{
    int priority;
    QString origin;
    int64_t duration;
    int width;
    int height;
    int scale;
    QString format;
    QString imgName;
    QByteArray data;
};

struct EffectCmdData
{
    int priority;
    int duration;
    QString pythonScript;
    QString origin;
    QString effectName;
    QString data;
    QJsonObject args;
};

struct registerData
{
	hyperion::Components component;
	QString origin;
	QString owner;
	hyperion::Components callerComp;
};
