// Local-Hyperion includes
#include "LedDeviceAtmoOrb.h"

// qt includes
#include <QtNetwork>

LedDeviceAtmoOrb::LedDeviceAtmoOrb(const QJsonObject &deviceConfig)
	: LedDevice()
	  , _networkmanager (nullptr)
	  , _udpSocket (nullptr)
	  , _multiCastGroupPort (49692)
	  , joinedMulticastgroup (false)
	  , _useOrbSmoothing (false)
	  , _transitiontime (0)
	  , _skipSmoothingDiff (0)
	  , _numLeds (24)

{
	_devConfig = deviceConfig;
	_deviceReady = false;
}

LedDevice* LedDeviceAtmoOrb::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceAtmoOrb(deviceConfig);
}

LedDeviceAtmoOrb::~LedDeviceAtmoOrb()
{
	_networkmanager->deleteLater();
	_udpSocket->deleteLater();
}

bool LedDeviceAtmoOrb::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = LedDevice::init(deviceConfig);

	if ( isInitOK )
	{

		_multicastGroup     = deviceConfig["output"].toString().toStdString().c_str();
		_useOrbSmoothing    = deviceConfig["useOrbSmoothing"].toBool(false);
		_transitiontime     = deviceConfig["transitiontime"].toInt(0);
		_skipSmoothingDiff  = deviceConfig["skipSmoothingDiff"].toInt(0);
		_multiCastGroupPort = static_cast<quint16>(deviceConfig["port"].toInt(49692));
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

		if ( _orbIds.size() == 0 )
		{
			this->setInError("No valid OrbIds found!");
			isInitOK = false;
		}
	}
	return isInitOK;
}

bool LedDeviceAtmoOrb::initNetwork()
{
	bool isInitOK = true;

	// TODO: Add Network-Error handling
	_networkmanager = new QNetworkAccessManager();
	_groupAddress = QHostAddress(_multicastGroup);

	_udpSocket = new QUdpSocket(this);
	_udpSocket->bind(QHostAddress::AnyIPv4, _multiCastGroupPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

	joinedMulticastgroup = _udpSocket->joinMulticastGroup(_groupAddress);
	return isInitOK;
}

int LedDeviceAtmoOrb::open()
{
	int retval = -1;
	_deviceReady = false;

	if ( init(_devConfig) )
	{
		if ( !initNetwork() )
		{
			this->setInError( "Network error!" );
		}
		else
		{
			_deviceReady = true;
			setEnable(true);
			retval = 0;
		}
	}
	return retval;
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
	int idx = 1;

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

void LedDeviceAtmoOrb::setColor(int orbId, const ColorRgb &color, int commandType)
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
