// Local-Hyperion includes
#include "LedDeviceAtmoOrb.h"

// qt includes
#include <QtCore/qmath.h>
#include <QEventLoop>
#include <QtNetwork>
#include <QNetworkReply>
#include <QTime> 

//#include <iostream>
#include <stdexcept>
#include <string>
#include <set>

AtmoOrbLight::AtmoOrbLight(unsigned int id) {
	// Not implemented
}

LedDeviceAtmoOrb::LedDeviceAtmoOrb(const std::string& output, bool switchOffOnBlack,
		int transitiontime, int port, int numLeds, std::vector<unsigned int> orbIds) :
     multicastGroup(output.c_str()), switchOffOnBlack(switchOffOnBlack), transitiontime(transitiontime),
  multiCastGroupPort(port), numLeds(numLeds), orbIds(orbIds) {
	manager = new QNetworkAccessManager();
  groupAddress = QHostAddress(multicastGroup);

  udpSocket = new QUdpSocket(this);
  udpSocket->bind(multiCastGroupPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

  if (!udpSocket->joinMulticastGroup(groupAddress))
  {
    joinedMulticastgroup = false;
  }
  else
  {
    joinedMulticastgroup = true;
  }
}

int LedDeviceAtmoOrb::write(const std::vector<ColorRgb> & ledValues) {

  // If not in multicast group return
  if (!joinedMulticastgroup)
  {
    return 0;
  }

  // Iterate through colors and set Orb color
  // Start off with idx 1 as 0 is reserved for controlling all orbs at once
  unsigned int idx = 1;
  for (const ColorRgb& color : ledValues) 
  {
    // Options parameter: 
    //
    // 1 = force off
    // 2 = use lamp smoothing and validate by Orb ID
    // 4 = validate by Orb ID
    //

    if (switchOffOnBlack && color.red == 0 && color.green == 0 && color.blue == 0) {
      // Force to black
      for (unsigned int i = 0; i < orbIds.size(); i++) {
        if (orbIds[i] == idx)
        {
          setColor(idx, color, 1);
        }
      }
    }
    else
    {
      // Default send color
      for (unsigned int i = 0; i < orbIds.size(); i++) {
        if (orbIds[i] == idx)
        {
          setColor(idx, color, 4);
        }
      }
    }

    // Next light id.
    idx++;
  }
  return 0;
}

void LedDeviceAtmoOrb::setColor(unsigned int orbId, const ColorRgb& color, int commandType) {
  QByteArray bytes;
  bytes.resize(5 + numLeds * 3);

  // Command identifier: C0FFEE
  bytes[0] = 0xC0;
  bytes[1] = 0xFF;
  bytes[2] = 0xEE;

  // Command type
  bytes[3] = 2;

  // Orb ID
  bytes[4] = orbId;

  // RED / GREEN / BLUE
  bytes[5] = color.red;
  bytes[6] = color.green;
  bytes[7] = color.blue;

  sendCommand(bytes);
}

void LedDeviceAtmoOrb::sendCommand(const QByteArray & bytes) {
  QByteArray datagram = bytes;
  udpSocket->writeDatagram(datagram.data(), datagram.size(),
    groupAddress, multiCastGroupPort);
}

int LedDeviceAtmoOrb::switchOff() {

  // Default send color
  for (unsigned int i = 0; i < orbIds.size(); i++) {
    QByteArray bytes;
    bytes.resize(5 + numLeds * 3);

    // Command identifier: C0FFEE
    bytes[0] = 0xC0;
    bytes[1] = 0xFF;
    bytes[2] = 0xEE;

    // Command type
    bytes[3] = 1;

    // Orb ID
    bytes[4] = orbIds[i];

    // RED / GREEN / BLUE
    bytes[5] = 0;
    bytes[6] = 0;
    bytes[7] = 0;

    sendCommand(bytes);
  }
  return 0;
}
void LedDeviceAtmoOrb::switchOn(unsigned int nLights) {
  // Not implemented
}

LedDeviceAtmoOrb::~LedDeviceAtmoOrb() {
  delete manager;
}