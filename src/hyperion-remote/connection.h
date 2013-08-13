#pragma once

#include <string>

#include <QColor>
#include <QImage>
#include <QTcpSocket>

#include <json/json.h>

#include "colortransform.h"

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
    bool setTransform(ColorTransform * threshold, ColorTransform * gamma, ColorTransform * blacklevel, ColorTransform * whitelevel);

private:
    bool sendMessage(const Json::Value & message);

private:
    bool _printJson;

    QTcpSocket _socket;
};
