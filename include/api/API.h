#pragma once


// hyperion includes
#include <utils/Logger.h>
#include <utils/Components.h>
#include <hyperion/Hyperion.h>
#include <hyperion/HyperionIManager.h>
// auth manager
#include <hyperion/AuthManager.h>

#include <utils/ColorRgb.h>
#include <utils/ColorSys.h>

// qt includes
#include <QString>

class QTimer;
class JsonCB;
class HyperionIManager;

const QString NO_AUTH = "No Authorization";

///
/// @brief API for Hyperion to be inherted from a child class with specific protocol implementations
/// Workflow:
/// 1. create the class
/// 2. connect the forceClose signal, as the api might to close the connection for security reasons
/// 3. call Initialize()
/// 4. proceed as usual
///

class API : public QObject
{
	Q_OBJECT

public:
	#include <api/apiStructs.h>
	// workaround Q_ARG std::map template issues
	typedef std::map<int,registerData> MapRegister;
	typedef QMap<QString, AuthManager::AuthDefinition> MapAuthDefs;

	///
	/// Constructor
	///
	///@ param The parent Logger
	/// @param localConnection Is this a local network connection? Use utils/NetOrigin to check that
	/// @param parent          Parent QObject
	///
	API(Logger* log, const bool& localConnection, QObject* parent);

protected:
	///
	/// @brief Initialize the API
	/// This call is REQUIRED!
	///
	void init(void);

	///
	/// @brief Set a single color
	/// @param[in] priority The priority of the written color
	/// @param[in] ledColor The color to write to the leds
	/// @param[in] timeout_ms The time the leds are set to the given color [ms]
	/// @param[in] origin   The setter
	///
	void setColor(const int &priority, const std::vector<uint8_t>& ledColors, const int &timeout_ms = -1, const QString& origin = "API", const hyperion::Components& callerComp = hyperion::COMP_INVALID);

	///
	/// @brief Set a image
	/// @param[in]  data      The command data
	/// @param[in]  comp      The component that should be used
	/// @param[out] replyMsg  The replyMsg on failure
	/// @param      callerComp The HYPERION COMPONENT that calls this function! e.g. PROT/FLATBUF
	/// @return True on success
	///
	bool setImage(ImageCmdData& data, hyperion::Components comp, QString &replyMsg, const hyperion::Components& callerComp = hyperion::COMP_INVALID);

	///
	/// @brief Clear a priority in the Muxer, if -1 all priorities are cleared
	/// @param priority   The priority to clear
	/// @param replyMsg   the message on failure
	/// @param callerComp The HYPERION COMPONENT that calls this function! e.g. PROT/FLATBUF
	/// @return  True on success
	///
	bool clearPriority(const int& priority, QString& replyMsg, const hyperion::Components& callerComp = hyperion::COMP_INVALID);

	///
	/// @brief Set a new component state
	/// @param comp       The component name
	/// @param compState  The new state of the comp
	/// @param replyMsg   The reply on failure
	/// @param callerComp The HYPERION COMPONENT that calls this function! e.g. PROT/FLATBUF
	/// @ return True on success
	///
	bool setComponentState(const QString& comp, bool& compState, QString& replyMsg, const hyperion::Components& callerComp = hyperion::COMP_INVALID);

	///
	/// @brief Set a ledToImageMapping type
	/// @param type       mapping type string
	/// @param callerComp The HYPERION COMPONENT that calls this function! e.g. PROT/FLATBUF
	///
	void setLedMappingType(const int& type, const hyperion::Components& callerComp = hyperion::COMP_INVALID);

	///
	/// @brief Set the 2D/3D modes type
	/// @param mode       The VideoMode
	/// @param callerComp The HYPERION COMPONENT that calls this function! e.g. PROT/FLATBUF
	///
	void setVideoMode(const VideoMode& mode, const hyperion::Components& callerComp = hyperion::COMP_INVALID);

	///
	/// @brief Set an effect
	/// @param dat        The effect data
	/// @param callerComp The HYPERION COMPONENT that calls this function! e.g. PROT/FLATBUF
	/// REQUIRED dat fields: effectName, priority, duration, origin
	///
	void setEffect(const EffectCmdData& dat, const hyperion::Components& callerComp = hyperion::COMP_INVALID);

	///
	/// @brief Set source auto select enabled or disabled
	/// @param sate       The new state
	/// @param callerComp The HYPERION COMPONENT that calls this function! e.g. PROT/FLATBUF
	///
	void setSourceAutoSelect(const bool state, const hyperion::Components& callerComp = hyperion::COMP_INVALID);

	///
	/// @brief Set the visible priority to given priority
	/// @param priority   The priority to set
	/// @param callerComp The HYPERION COMPONENT that calls this function! e.g. PROT/FLATBUF
	///
	void setVisiblePriority(const int& priority, const hyperion::Components& callerComp = hyperion::COMP_INVALID);

	///
	/// @brief Register a input or update the meta data of a previous register call
	/// ATTENTION: Check unregisterInput() method description !!!
	/// @param[in] priority    The priority of the channel
	/// @param[in] component   The component of the channel
	/// @param[in] origin      Who set the channel (CustomString@IP)
	/// @param[in] owner       Specific owner string, might be empty
	/// @param[in] callerComp  The component that call this (e.g. PROTO/FLAT)
	///
	void registerInput(const int& priority, const hyperion::Components& component, const QString& origin, const QString& owner, const hyperion::Components& callerComp);

	///
	/// @brief Revoke a registerInput() call by priority. We maintain all registered priorities in this scope
	/// ATTENTION: This is MANDATORY if you change (priority change) or stop(clear/timeout) DURING lifetime. If this class destructs it's not needed
	/// @param priority  The priority to unregister
	///
	void unregisterInput(const int& priority);

	///
	/// @brief Handle the instance switching
	/// @param inst  The requested instance
	/// @return True on success else false
	///
	bool setHyperionInstance(const quint8& inst);

	///
	/// @brief Get all contrable components and their state
	///
	std::map<hyperion::Components, bool> getAllComponents();

	///
	/// @brief Check if Hyperion ist enabled
	/// @return True when enabled else false
	///
	bool isHyperionEnabled();

	//////////////////////////////////
	/// AUTH / ADMINISTRATION METHODS
	//////////////////////////////////

	///
	/// @brief Delete an effect. Requires ADMIN ACCESS
	/// @param name The effect name
	/// @return  True on success else false
	///
	QString deleteEffect(const QString& name);

	///
	/// @brief Delete an effect. Requires ADMIN ACCESS
	/// @param name The effect name
	/// @return  True on success else false
	///
	QString saveEffect(const QJsonObject& data);

	///
	/// @brief Save settings object. Requires ADMIN ACCESS
	/// @param data  The data object
	///
	void saveSettings(const QJsonObject& data);

	///
	/// @brief Test if we are authorized to use the interface
	/// @return The result
	///
	bool isAuthorized(){ return _authorized; };

	///
	/// @brief Test if we are authorized to use the admin interface
	/// @return The result
	///
	bool isAdminAuthorized(){ return _adminAuthorized; };

	///
	/// @brief Update the Password of Hyperion. Requires ADMIN ACCESS
	/// @param password    Old password
	/// @param newPassword New password
	/// @return True on success else false
	///
	bool updateHyperionPassword(const QString& password, const QString& newPassword);

	///
	/// @brief Get a new token from AuthManager. Requires ADMIN ACCESS
	/// @param comment   The comment of the request
	/// @param def       The final definition
	/// @return True on success else false
	///
	bool createToken(const QString& comment, AuthManager::AuthDefinition& def);

	///
	/// @brief Delete a token by given id. Requires ADMIN ACCESS
	/// @param id  The id of the token
	/// @return True on succes
	///
	bool deleteToken(const QString& id);

	///
	/// @brief Set a new token request
	/// @param comment  The comment
	/// @param id       The id
	///
	void setNewTokenRequest(const QString& comment, const QString& id);

	///
	/// @brief Cancel new token request
	/// @param comment  The comment
	/// @param id       The id
	///
	void cancelNewTokenRequest(const QString& comment, const QString& id);

	///
	/// @brief Handle a pending token request. Requires ADMIN ACCESS
	/// @param id     The id fo the request
	/// @param accept True when it should be accepted, else false
	/// @return True on success
	bool handlePendingTokenRequest(const QString& id, const bool accept);

	///
	/// @brief Get the current List of Tokens. Requires ADMIN ACCESS
	/// @param def returns the defintions
	/// @return True on success
	///
	bool getTokenList(QVector<AuthManager::AuthDefinition>& def);

	///
	/// @brief Get all current pending token requests. Requires ADMIN ACCESS
	/// @return True on success
	///
	bool getPendingTokenRequests(QMap<QString, AuthManager::AuthDefinition>& map);

	///
	/// @brief Is User Token Authorized. On success this will grant acces to API and ADMIN API
	/// @param userToken   The user Token
	/// @return True on succes
	///
	bool isUserTokenAuthorized(const QString& userToken);

	///
	/// @brief Get the current User Token (session token). Requires ADMIN ACCESS
	/// @param userToken   The user Token
	/// @return True on success
	///
	bool getUserToken(QString& userToken);

	///
	/// @brief Is a token authrized. On success this will grant acces to the API (NOT ADMIN API)
	/// @param token   The user Token
	/// @return True on succes
	///
	bool isTokenAuthorized(const QString& token);

	///
	/// @brief Is User authorized. On success this will grant acces to the API and ADMIN API
	/// @param password  The password of the User
	/// @return True if authorized
	///
	bool isUserAuthorized(const QString& password);

	///
	/// @brief Test if Hyperion has the default PW
	/// @return The result
	///
	bool hasHyperionDefaultPw();

	///
	/// @brief Logout revokes all authorizations
	///
	void logout();

	/// Reflect auth status of this client
	bool _authorized;
	bool _adminAuthorized;

	/// Is this a local connection
	bool _localConnection;

	// current instance index
	quint8 _currInstanceIndex;

	Logger* _log;
	AuthManager* _authManager;
	HyperionIManager* _instanceManager;
	Hyperion* _hyperion;

signals:
	///
	/// @brief The API might decide to block connections for security reasons, this emitter should close the socket
	///
	void forceClose();

private slots:
	///
	/// @brief Is called whenever a Hyperion instance wants the current register list
	/// @param callerInstance  The instance should be returned in the answer call
	///
	void requestActiveRegister(QObject* callerInstance);

private:
	void stopDataConnections();

	// Contains all active register call data
	std::map<int,registerData> _activeRegisters;
};
