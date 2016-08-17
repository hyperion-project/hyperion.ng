// Local-Hyperion includes
#include "LedDeviceAtmoOrb.h"

// qt includes
#include <QtCore/qmath.h>
#include <QEventLoop>
#include <QtNetwork>
#include <QNetworkReply>

#include <stdexcept>
#include <string>
#include <set>

AtmoOrbLight::AtmoOrbLight(unsigned int id) {
	// Not implemented
}

LedDeviceAtmoOrb::LedDeviceAtmoOrb(
		const std::string &output,
		bool useOrbSmoothing,
		int transitiontime,
		int skipSmoothingDiff,
		int port,
		int numLeds,
		std::vector<unsigned int> orbIds)
	: LedDevice()
	, _multicastGroup(output.c_str())
	, _useOrbSmoothing(useOrbSmoothing)
	, _transitiontime(transitiontime)
	, _skipSmoothingDiff(skipSmoothingDiff)
	, _multiCastGroupPort(port)
	, _numLeds(numLeds)
	, _orbIds(orbIds)
{
	_manager = new QNetworkAccessManager();
	_groupAddress = QHostAddress(_multicastGroup);

	_udpSocket = new QUdpSocket(this);
	_udpSocket->bind(QHostAddress::Any, _multiCastGroupPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

	joinedMulticastgroup = _udpSocket->joinMulticastGroup(_groupAddress);
}

int LedDeviceAtmoOrb::write(const std::vector <ColorRgb> &ledValues)
{
	// If not in multicast group return
	if (!joinedMulticastgroup)
	{
		return 0;
	}

	// Command options:
	//
	// 1 = force off
	// 2 = use lamp smoothing and validate by Orb ID
	// 4 = validate by Orb ID

	// When setting _useOrbSmoothing = true it's recommended to disable Hyperion's own smoothing as it will conflict (double smoothing)
	int commandType = 4;
	if(_useOrbSmoothing)
	{
		commandType = 2;
	}

	// Iterate through colors and set Orb color
	// Start off with idx 1 as 0 is reserved for controlling all orbs at once
	unsigned int idx = 1;

	for (const ColorRgb &color : ledValues)
	{
		// Retrieve last send colors
		int lastRed = lastColorRedMap[idx];
		int lastGreen = lastColorGreenMap[idx];
		int lastBlue = lastColorBlueMap[idx];

		// If color difference is higher than _skipSmoothingDiff than we skip Orb smoothing (if enabled) and send it right away
		if ((_skipSmoothingDiff != 0 && _useOrbSmoothing) && (abs(color.red - lastRed) >= _skipSmoothingDiff || abs(color.blue - lastBlue) >= _skipSmoothingDiff ||
				abs(color.green - lastGreen) >= _skipSmoothingDiff))
		{
			// Skip Orb smoothing when using  (command type 4)
			for (unsigned int i = 0; i < _orbIds.size(); i++)
			{
				if (_orbIds[i] == idx)
				{
					setColor(idx, color, 4);
				}
			}
		}
		else
		{
			// Send color
			for (unsigned int i = 0; i < _orbIds.size(); i++)
			{
				if (_orbIds[i] == idx)
				{
					setColor(idx, color, commandType);
				}
			}
		}

		// Store last colors send for light id
		lastColorRedMap[idx]   = color.red;
		lastColorGreenMap[idx] = color.green;
		lastColorBlueMap[idx]  = color.blue;

		// Next light id.
		idx++;
	}

	return 0;
}

void LedDeviceAtmoOrb::setColor(unsigned int orbId, const ColorRgb &color, int commandType)
{
	QByteArray bytes;
	bytes.resize(5 + _numLeds * 3);
	bytes.fill('\0');

	// Command identifier: C0FFEE
	bytes[0] = 0xC0;
	bytes[1] = 0xFF;
	bytes[2] = 0xEE;

	// Command type
	bytes[3] = commandType;

	// Orb ID
	bytes[4] = orbId;

	// RED / GREEN / BLUE
	bytes[5] = color.red;
	bytes[6] = color.green;
	bytes[7] = color.blue;

	sendCommand(bytes);
}

void LedDeviceAtmoOrb::sendCommand(const QByteArray &bytes)
{
	QByteArray datagram = bytes;
	_udpSocket->writeDatagram(datagram.data(), datagram.size(), _groupAddress, _multiCastGroupPort);
}

int LedDeviceAtmoOrb::switchOff() {
	for (unsigned int i = 0; i < _orbIds.size(); i++)
	{
		QByteArray bytes;
		bytes.resize(5 + _numLeds * 3);
		bytes.fill('\0');

		// Command identifier: C0FFEE
		bytes[0] = 0xC0;
		bytes[1] = 0xFF;
		bytes[2] = 0xEE;

		// Command type
		bytes[3] = 1;

		// Orb ID
		bytes[4] = _orbIds[i];

		// RED / GREEN / BLUE
		bytes[5] = 0;
		bytes[6] = 0;
		bytes[7] = 0;

		sendCommand(bytes);
	}
	return 0;
}

LedDeviceAtmoOrb::~LedDeviceAtmoOrb()
{
	delete _manager;
}
