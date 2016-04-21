#pragma once

// STL includes
#include <string>

// Qt includes
#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QHostAddress>

// Leddevice includes
#include <leddevice/LedDevice.h>

class QUdpSocket;

class AtmoOrbLight {
public:
    unsigned int id;

    ///
    /// Constructs the light.
    ///
    /// @param id the orb id
    AtmoOrbLight(unsigned int id);
};

/**
 * Implementation for the AtmoOrb
 *
 * To use set the device to "atmoorb".
 *
 * @author RickDB (github)
 */
class LedDeviceAtmoOrb : public QObject, public LedDevice {
    Q_OBJECT
public:
    // Last send color map
    QMap<int, int> lastColorRedMap;
    QMap<int, int> lastColorGreenMap;
    QMap<int, int> lastColorBlueMap;

    // Multicast status
    bool joinedMulticastgroup;

    ///
    /// Constructs the device.
    ///
    /// @param output is the multicast address of Orbs
    ///
    /// @param transitiontime is optional and not used at the moment
    ///
    /// @param useOrbSmoothing use Orbs own (external) smoothing algorithm (default: false)
    ///
    /// @param skipSmoothingDiff  minimal color (0-255) difference to override smoothing so that if current and previously received colors are higher than set dif we override smoothing
    ///
    /// @param port is the multicast port.
    ///
    /// @param numLeds is the total amount of leds per Orb
    ///
    /// @param array containing orb ids
    ///
    LedDeviceAtmoOrb(const std::string &output, bool useOrbSmoothing =
    false, int transitiontime = 0, int skipSmoothingDiff = 0, int port = 49692, int numLeds = 24,
                     std::vector<unsigned int> orbIds = std::vector < unsigned int>());

    ///
    /// Destructor of this device
    ///
    virtual ~LedDeviceAtmoOrb();

    ///
    /// Sends the given led-color values to the Orbs
    ///
    /// @param ledValues The color-value per led
    ///
    /// @return Zero on success else negative
    ///
    virtual int write(const std::vector <ColorRgb> &ledValues);

    virtual int switchOff();

private:
    /// QNetworkAccessManager object for sending requests.
    QNetworkAccessManager *manager;

    /// String containing multicast group IP address
    QString multicastGroup;

    /// use Orbs own (external) smoothing algorithm
    bool useOrbSmoothing;

    /// Transition time between colors (not implemented)
    int transitiontime;

    // Maximum allowed color difference, will skip Orb (external) smoothing once reached
    int skipSmoothingDiff;

    /// Multicast port to send data to
    int multiCastGroupPort;

    /// Number of leds in Orb, used to determine buffer size
    int numLeds;

    /// QHostAddress object of multicast group IP address
    QHostAddress groupAddress;

    /// QUdpSocket object used to send data over
    QUdpSocket *udpSocket;

    /// Array of the orb ids.
    std::vector<unsigned int> orbIds;

    ///
    /// Set Orbcolor
    ///
    /// @param orbId the orb id
    ///
    /// @param color which color to set
    ///
    ///
    /// @param commandType which type of command to send (off / smoothing / etc..)
    ///
    void setColor(unsigned int orbId, const ColorRgb &color, int commandType);

    ///
    /// Send Orb command
    ///
    /// @param bytes the byte array containing command to send over multicast
    ///
    void sendCommand(const QByteArray &bytes);
};