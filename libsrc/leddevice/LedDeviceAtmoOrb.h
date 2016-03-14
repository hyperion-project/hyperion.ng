#pragma once

// STL includes
#include <string>

// Qt includes
#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QHostAddress>
#include <QTime> 

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
  // Last color sent
  int lastRed;
  int lastGreen;
  int lastBlue;

  // Last command sent timer
  QTime timer;

  // Multicast status
  bool joinedMulticastgroup;

  ///
  /// Constructs the device.
  ///
  /// @param output is the multicast address of Orbs
  ///
  /// @param switchOffOnBlack turn off Orbs on black (default: false)
  ///
  /// @param transitiontime is optional and not used at the moment
  ///
  /// @param port is the multicast port.
  ///
  /// @param numLeds is the total amount of leds per Orb
  ///
  /// @param orb ids to control
  ///
  LedDeviceAtmoOrb(const std::string& output, bool switchOffOnBlack =
    false, int transitiontime = 0, int port = 49692, int numLeds =  24, std::vector<unsigned int> orbIds = std::vector<unsigned int>());
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
  virtual int write(const std::vector<ColorRgb> & ledValues);
  virtual int switchOff();
private:
  /// Array to save the lamps.
  std::vector<AtmoOrbLight> lights;

  /// QNetworkAccessManager object for sending requests.
  QNetworkAccessManager* manager;

  QString multicastGroup;
  bool switchOffOnBlack;
  int transitiontime;
  int multiCastGroupPort;
  int numLeds;
  QHostAddress groupAddress;
  QUdpSocket *udpSocket;

  /// Array of the light ids.
  std::vector<unsigned int> orbIds;

  ///
  /// Switches the leds on.
  ///
  /// @param nLights the number of lights
  ///
  void switchOn(unsigned int nLights);

  // Set color
  void setColor(unsigned int orbId, const ColorRgb& color, int commandType);

  // Send color command
  void sendCommand(const QByteArray & bytes);
};
