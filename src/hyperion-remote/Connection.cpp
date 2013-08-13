#include <stdexcept>

#include "Connection.h"

Connection::Connection(const std::string & a, bool printJson) :
    _printJson(printJson),
    _socket()
{
    QString address(a.c_str());
    QStringList parts = address.split(":");
    if (parts.size() != 2)
    {
        throw std::runtime_error(QString("Wrong address: unable to parse address (%1)").arg(address).toStdString());
    }

    bool ok;
    uint16_t port = parts[1].toUShort(&ok);
    if (!ok)
    {
        throw std::runtime_error(QString("Wrong address: Unable to parse the port number (%1)").arg(parts[1]).toStdString());
    }

    _socket.connectToHost(parts[0], port);
    if (!_socket.waitForConnected())
    {
        throw std::runtime_error("Unable to connect to host");
    }
}

Connection::~Connection()
{
    _socket.close();
}

bool Connection::setColor(QColor color, int priority, int duration)
{
    std::cout << "Set color to " << color.red() << " " << color.green() << " " << color.blue() << std::endl;
    sendMessage("Set color message");
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

bool Connection::setTransform(ColorTransformValues *threshold, ColorTransformValues *gamma, ColorTransformValues *blacklevel, ColorTransformValues *whitelevel)
{
    std::cout << "Set color transforms" << std::endl;
    return false;
}

bool Connection::sendMessage(const Json::Value & message)
{
    // rpint if requested
    if (_printJson)
    {
        std::cout << "Command: " << message << std::endl;
    }

    // serialize message
    Json::FastWriter jsonWriter;
    std::string serializedMessage = jsonWriter.write(message);

    // write message
    _socket.write(serializedMessage.c_str());
    if (!_socket.waitForBytesWritten())
    {
        throw std::runtime_error("Error while writing data to host");
    }

    // receive reply
    if (!_socket.waitForReadyRead())
    {
        throw std::runtime_error("Error while reading data from host");
    }
    char data[1024 * 100];
    uint64_t count = _socket.read(data, sizeof(data));
    std::string serializedReply(data, count);
    Json::Reader jsonReader;
    Json::Value reply;
    if (!jsonReader.parse(serializedReply, reply))
    {
        throw std::runtime_error("Error while parsing reply:" + serializedReply);
    }

    if (_printJson)
    {
        std::cout << "Reply:" << reply << std::endl;
    }

    return true;
}

