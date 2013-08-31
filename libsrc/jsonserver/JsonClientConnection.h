#pragma once

// stl includes
#include <string>

// Qt includes
#include <QByteArray>
#include <QTcpSocket>

// jsoncpp includes
#include <json/json.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

// util includes
#include <utils/jsonschema/JsonSchemaChecker.h>

class ImageProcessor;

/// @brief The Connection object created by \a JsonServer when a new connection is establshed
///
class JsonClientConnection : public QObject
{
	Q_OBJECT

public:
	/// @brief Constructor
	/// @param socket The Socket object for this connection
	/// @param hyperion The Hyperion server
	JsonClientConnection(QTcpSocket * socket, Hyperion * hyperion);

	/// @brief Destructor
	~JsonClientConnection();

signals:
	/// @brief Signal which is emitted when the connection is being closed
	/// @param connection This connection object
	void connectionClosed(JsonClientConnection * connection);

private slots:
	/// @brief Slot called when new data has arrived
	void readData();

	/// @brief Slot called when this connection is being closed
	void socketClosed();

private:
	/// @brief Handle an incoming JSON message
	/// @param message the incoming message as string
	void handleMessage(const std::string & message);

	/// @brief Handle an incoming JSON Color message
	/// @param message the incoming message
	void handleColorCommand(const Json::Value & message);

	/// @brief Handle an incoming JSON Image message
	/// @param message the incoming message
	void handleImageCommand(const Json::Value & message);

	/// @brief Handle an incoming JSON Server info message
	/// @param message the incoming message
	void handleServerInfoCommand(const Json::Value & message);

	/// @brief Handle an incoming JSON Clear message
	/// @param message the incoming message
	void handleClearCommand(const Json::Value & message);

	/// @brief Handle an incoming JSON Clearall message
	/// @param message the incoming message
	void handleClearallCommand(const Json::Value & message);

	/// @brief Handle an incoming JSON Transform message
	/// @param message the incoming message
	void handleTransformCommand(const Json::Value & message);

	/// @brief Handle an incoming JSON message of unknown type
	/// @param message the incoming message
	void handleNotImplemented();

	/// @brief Send a message to the connected client
	/// @param message The JSON message to send
	void sendMessage(const Json::Value & message);

	/// @brief Send a standard reply indicating success
	void sendSuccessReply();

	/// @brief Send an error message back to the client
	/// @param error String describing the error
	void sendErrorReply(const std::string & error);

private:
	/// @brief Check if a JSON messag is valid according to a given JSON schema
	/// @param message JSON message which need to be checked
	/// @param schemaResource Qt esource identifier with the JSON schema
	/// @param errors Output error message
	/// @return true if message conforms the given JSON schema
	bool checkJson(const Json::Value & message, const QString &schemaResource, std::string & errors);

private:
	QTcpSocket * _socket;

	ImageProcessor * _imageProcessor;

	Hyperion * _hyperion;

	QByteArray _receiveBuffer;
};
