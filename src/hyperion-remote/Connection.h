#pragma once

#include <string>

#include <QColor>
#include <QImage>
#include <QTcpSocket>

#include <json/json.h>

#include "ColorTransformValues.h"

class Connection
{
public:
    Connection(const std::string & address, bool printJson);
    ~Connection();

    bool setColor(QColor color, int priority, int duration);
    bool setImage(QImage image, int priority, int duration);
    bool listPriorities();
    bool clear(int priority);
    bool clearAll();
    bool setTransform(ColorTransformValues * threshold, ColorTransformValues * gamma, ColorTransformValues * blacklevel, ColorTransformValues * whitelevel);

private:
    bool sendMessage(const Json::Value & message);

private:
    bool _printJson;

    QTcpSocket _socket;
};
