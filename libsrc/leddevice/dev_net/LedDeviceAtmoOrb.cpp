// Local-Hyperion includes
#include "LedDeviceAtmoOrb.h"
#include <utils/QStringUtils.h>

// qt includes
#include <QUdpSocket>

const quint16 MULTICAST_GROUPL_DEFAULT_PORT = 49692;
const int LEDS_DEFAULT_NUMBER = 24;

LedDeviceAtmoOrb::LedDeviceAtmoOrb(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	  , _udpSocket (nullptr)
	  , _multiCastGroupPort (MULTICAST_GROUPL_DEFAULT_PORT)
	  , _joinedMulticastgroup (false)
	  , _useOrbSmoothing (false)
	  , _transitiontime (0)
	  , _skipSmoothingDiff (0)
	  , _numLeds (LEDS_DEFAULT_NUMBER)

{
}

LedDevice* LedDeviceAtmoOrb::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceAtmoOrb(deviceConfig);
}

LedDeviceAtmoOrb::~LedDeviceAtmoOrb()
{
	delete _udpSocket;
}

bool LedDeviceAtmoOrb::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	if ( LedDevice::init(deviceConfig) )
	{

		_multicastGroup     = deviceConfig["output"].toString().toStdString().c_str();
		_useOrbSmoothing    = deviceConfig["useOrbSmoothing"].toBool(false);
		_transitiontime     = deviceConfig["transitiontime"].toInt(0);
		_skipSmoothingDiff  = deviceConfig["skipSmoothingDiff"].toInt(0);
		_multiCastGroupPort = static_cast<quint16>(deviceConfig["port"].toInt(MULTICAST_GROUPL_DEFAULT_PORT));
		_numLeds            = deviceConfig["numLeds"].toInt(LEDS_DEFAULT_NUMBER);

		QStringList orbIds = QStringUtils::split(deviceConfig["orbIds"].toString().simplified().remove(" "),",", QStringUtils::SplitBehavior::SkipEmptyParts);
		_orbIds.clear();

		for (auto & id_str : orbIds)
		{
			bool ok;
			int id = id_str.toInt(&ok);
			if (ok)
			{
				if ( id < 1 || id > 255 )
				{
					Warning(_log, "Skip orb id '%d'. IDs must be in range 1-255", id);
				}
				else
				{
					_orbIds.append(id);
				}
			}
			else
			{
				Error(_log, "orb id '%s' is not a number", QSTRING_CSTR(id_str));
			}
		}

		if ( _orbIds.empty() )
		{
			this->setInError("No valid OrbIds found!");
			isInitOK = false;
		}
		else
		{
			_udpSocket = new QUdpSocket(this);
			isInitOK = true;
		}
	}
	return isInitOK;
}

int LedDeviceAtmoOrb::open()
{
	int retval = -1;
	_isDeviceReady = false;

	// Try to bind the UDP-Socket
	if ( _udpSocket != nullptr )
	{
		_groupAddress = QHostAddress(_multicastGroup);
		if ( !_udpSocket->bind(QHostAddress::AnyIPv4, _multiCastGroupPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint) )
		{
			QString errortext = QString ("(%1) %2, MulticastGroup: (%3)").arg(_udpSocket->error()).arg(_udpSocket->errorString(), _multicastGroup);
			this->setInError( errortext );
		}
		else
		{
			_joinedMulticastgroup = _udpSocket->joinMulticastGroup(_groupAddress);
			if ( !_joinedMulticastgroup )
			{
				QString errortext = QString ("(%1) %2, MulticastGroup: (%3)").arg(_udpSocket->error()).arg(_udpSocket->errorString(), _multicastGroup);
				this->setInError( errortext );
			}
			else
			{
				// Everything is OK, device is ready
				_isDeviceReady = true;
				retval = 0;
			}
		}
	}
	return retval;
}

int LedDeviceAtmoOrb::close()
{
	int retval = 0;
	_isDeviceReady = false;

	if ( _udpSocket != nullptr )
	{
		// Test, if device requires closing
		if ( _udpSocket->isOpen() )
		{
			Debug(_log,"Close UDP-device: %s", QSTRING_CSTR( this->getActiveDeviceType() ) );
			_udpSocket->close();
			// Everything is OK -> device is closed
		}
	}
	return retval;
}

int LedDeviceAtmoOrb::write(const std::vector <ColorRgb> &ledValues)
{
	// If not in multicast group return
	if (!_joinedMulticastgroup)
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

	// 5 bytes command-header + 3 bytes color information
	bytes.resize(5 + 3);
	bytes.fill('\0');

	// Command identifier: C0FFEE
	bytes[0] = static_cast<char>(0xC0);
	bytes[1] = static_cast<char>(0xFF);
	bytes[2] = static_cast<char>(0xEE);

	// Command type
	bytes[3] = static_cast<char>(commandType);

	// Orb ID
	bytes[4] = static_cast<char>(orbId);

	// RED / GREEN / BLUE
	bytes[5] = static_cast<char>(color.red);
	bytes[6] = static_cast<char>(color.green);
	bytes[7] = static_cast<char>(color.blue);

	//std::cout << "Orb [" << orbId << "] Cmd [" << bytes.toHex(':').toStdString() <<"]"<< std::endl;

	sendCommand(bytes);
}

void LedDeviceAtmoOrb::sendCommand(const QByteArray &bytes)
{
	_udpSocket->writeDatagram(bytes.data(), bytes.size(), _groupAddress, _multiCastGroupPort);
}
