// Local-Hyperion includes
#include "LedDeviceAtmoOrb.h"

// qt includes
#include <QtCore/qmath.h>
#include <QEventLoop>
#include <QtNetwork>
#include <QNetworkReply>
#include <QStringList>

AtmoOrbLight::AtmoOrbLight(unsigned int id)
{
	// Not implemented
}

LedDeviceAtmoOrb::LedDeviceAtmoOrb(const QJsonObject &deviceConfig)
	: LedDevice()
{
	init(deviceConfig);
	_manager = new QNetworkAccessManager();
	_groupAddress = QHostAddress(_multicastGroup);

	_udpSocket = new QUdpSocket(this);
	_udpSocket->bind(QHostAddress::AnyIPv4, _multiCastGroupPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

	joinedMulticastgroup = _udpSocket->joinMulticastGroup(_groupAddress);
}

bool LedDeviceAtmoOrb::init(const QJsonObject &deviceConfig)
{
	_multicastGroup     = deviceConfig["output"].toString().toStdString().c_str();
	_useOrbSmoothing    = deviceConfig["useOrbSmoothing"].toBool(false);
	_transitiontime     = deviceConfig["transitiontime"].toInt(0);
	_skipSmoothingDiff  = deviceConfig["skipSmoothingDiff"].toInt(0);
	_multiCastGroupPort = deviceConfig["port"].toInt(49692);
	_numLeds            = deviceConfig["numLeds"].toInt(24);
	
	const QStringList orbIds = deviceConfig["orbIds"].toString().simplified().remove(" ").split(",", QString::SkipEmptyParts);
	_orbIds.clear();

	foreach(auto & id_str, orbIds)
	{
		bool ok;
		int id = id_str.toInt(&ok);
		if (ok)
			_orbIds.append(id);
		else
			Error(_log, "orb id '%s' is not a number", QSTRING_CSTR(id_str));
	}

	return _orbIds.size() > 0;
}

LedDevice* LedDeviceAtmoOrb::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceAtmoOrb(deviceConfig);
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
			for (int i = 0; i < _orbIds.size(); i++)
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
			for (int i = 0; i < _orbIds.size(); i++)
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

int LedDeviceAtmoOrb::switchOff()
{
	for (auto orbId : _orbIds)
	{
		setColor(orbId, ColorRgb::BLACK, 1);
	}
	return 0;
}

LedDeviceAtmoOrb::~LedDeviceAtmoOrb()
{
	delete _manager;
}
