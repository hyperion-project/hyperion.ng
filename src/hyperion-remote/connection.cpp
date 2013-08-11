#include "connection.h"

Connection::Connection(const std::string &address, bool printJson)
{
}

Connection::~Connection()
{
}

bool Connection::setColor(QColor color, int priority, int duration)
{
    std::cout << "Set color to " << color.red() << " " << color.green() << " " << color.blue() << std::endl;
    return false;
}

bool Connection::setImage(QImage image, int priority, int duration)
{
    std::cout << "Set image has size: " << image.width() << "x" << image.height() << std::endl;
    return false;
}

bool Connection::listPriorities()
{
    std::cout << "List priority channels" << std::endl;
    return false;
}

bool Connection::clear(int priority)
{
    std::cout << "Clear priority channel " << priority << std::endl;
    return false;
}

bool Connection::clearAll()
{
    std::cout << "Clear all priority channels" << std::endl;
    return false;
}

bool Connection::setThreshold(double red, double green, double blue)
{
    return false;
}

bool Connection::setGamma(double red, double green, double blue)
{
    return false;
}

bool Connection::setBlacklevel(double red, double green, double blue)
{
    return false;
}

bool Connection::setWhitelevel(double red, double green, double blue)
{
    return false;
}

bool Connection::sendMessage(const Json::Value &message)
{
    if (_printJson)
    {
        std::cout << "Command:" << std::endl;
        std::cout << message << std::endl;
    }

    return true;
}

