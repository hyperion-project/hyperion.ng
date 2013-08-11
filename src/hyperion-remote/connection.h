#pragma once

#include <string>

#include <QColor>
#include <QImage>

#include <json/json.h>

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
    bool setThreshold(double red, double green, double blue);
    bool setGamma(double red, double green, double blue);
    bool setBlacklevel(double red, double green, double blue);
    bool setWhitelevel(double red, double green, double blue);

private:
    bool sendMessage(const Json::Value & message);

private:
    bool _printJson;
};
