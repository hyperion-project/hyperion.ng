#pragma once

// Leddevice includes
#include <leddevice/LedDevice.h>
#include "ProviderUdp.h"

// ssdp discover
#include <ssdp/SSDPDiscover.h>

// Qt includes
#include <QString>
#include <QNetworkAccessManager>

///
/// Implementation of the LedDevice interface for sending to
/// Nanoleaf devices via network by using the 'external control' protocol.
///
class LedDeviceNanoleaf : public ProviderUdp
{
public:
    ///
    /// Constructs the LedDevice for Nanoleaf LightPanels (aka Aurora) or Canvas
    ///
    /// following code shows all config options
    /// @code
    /// "device" :
    /// {
    ///     "type"   : "nanoleaf"
    ///     "output" : "hostname or IP", // Optional. If empty, device is tried to be discovered
    ///     "token"  : "Authentication Token",
    /// },
    ///@endcode
    ///
    /// @param deviceConfig json config for nanoleaf
    ///
    LedDeviceNanoleaf(const QJsonObject &deviceConfig);

    ///
    /// Destructor of the LedDevice; closes the tcp client
    ///
    virtual ~LedDeviceNanoleaf();

    /// Constructs leddevice
    static LedDevice* construct(const QJsonObject &deviceConfig);

    /// Switch the leds on
    virtual int switchOn();

    /// Switch the leds off
    virtual int switchOff();

protected:

    ///
    /// Writes the led color values to the led-device
    ///
    /// @param ledValues The color-value per led
    /// @return Zero on succes else negative
    ///
    virtual int write(const std::vector<ColorRgb> & ledValues);

    ///
    /// Identifies a Nanoleaf device's panel configuration,
    /// sets device into External Control (UDP) mode
    ///
    /// @param deviceConfig the json device config
    /// @return true if success
    /// @exception runtime_error in case device cannot be initialised
    /// e.g. more LEDs configured than device has panels or network problems
    ///
    bool init(const QJsonObject &deviceConfig);

private:
    // QNetworkAccessManager object for sending requests.
    QNetworkAccessManager* _networkmanager;

    QString _hostname;
    QString _api_port;
    QString _auth_token;

    //Nanoleaf device details
    QString _deviceModel;
    QString _deviceFirmwareVersion;
    ushort _extControlVersion;
    /// The number of panels with leds
	uint _panelLedCount;
    /// Array of the pannel ids.
    std::vector<uint> _panelIds;

    ///
    /// Discover Nanoleaf device via SSDP identifiers
    ///
    /// @return True, if Nanoleaf device was found
    ///
    bool discoverNanoleafDevice();

    ///
    /// Change Nanoleaf device to External Control (UDP) mode
    ///
    /// @return Response from device
    ///
    QJsonDocument changeToExternalControlMode();

    ///
    /// Get command to switch Nanoleaf device on or off
    ///
    /// @param isOn True, if to switch on device
    /// @return Command to switch device on/off
    ///
    QString getOnOffRequest (bool isOn ) const;

    ///
    /// Get command as url
    ///
    /// @param host Hostname or IP
    /// @param port IP-Port
    /// @param _auth_token Authorization token
    /// @param Endpoint command for request
    /// @return Url to execute endpoint/command
    ///
    QString getUrl(QString host, QString port, QString auth_token, QString endpoint) const;

    ///
    /// Execute GET request
    ///
    /// @param url GET request for url
    /// @return Response from device
    ///
    QJsonDocument getJson(QString url) const;

    ///
    /// Execute PUT request
    ///
    /// @param Url for PUT request
    /// @param json Command for request
    /// @return Response from device
    ///
    QJsonDocument putJson(QString url, QString json) const;

    ///
    /// Handle replys for GET and PUT requests
    ///
    /// @param reply Network reply
    /// @return Response for request, if no error
    /// @exception runtime_error for network or request errors
    ///
    QJsonDocument handleReply(QNetworkReply* const &reply ) const;


	///
	/// convert vector to hex string
	///
	/// @param uint8_t vector
	/// @return vector as string of hex values
	std::string uint8_vector_to_hex_string( const std::vector<uint8_t>& buffer ) const;
};
