// Local-Hyperion includes
#include "LedDeviceAtmoOrb.h"
#include <utils/QStringUtils.h>

// qt includes
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QUrl>
#include <QHostInfo>

#include <chrono>

// Constants
namespace {

const bool verbose = false;
const bool verbose3 = false;
const QString MULTICAST_GROUP_DEFAULT_ADDRESS = "239.255.255.250";
const quint16 MULTICAST_GROUP_DEFAULT_PORT = 49692;

constexpr std::chrono::milliseconds DEFAULT_DISCOVERY_TIMEOUT{2000};

} //End of constants

LedDeviceAtmoOrb::LedDeviceAtmoOrb(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	  , _udpSocket (nullptr)
	  , _multicastGroup(MULTICAST_GROUP_DEFAULT_ADDRESS)
	  , _multiCastGroupPort (MULTICAST_GROUP_DEFAULT_PORT)
	  , _joinedMulticastgroup (false)
	  , _useOrbSmoothing (false)
	  , _skipSmoothingDiff (0)
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
	bool isInitOK {false};

	if ( LedDevice::init(deviceConfig) )
	{
		_multicastGroup     = deviceConfig["host"].toString(MULTICAST_GROUP_DEFAULT_ADDRESS);
		_multiCastGroupPort = static_cast<quint16>(deviceConfig["port"].toInt(MULTICAST_GROUP_DEFAULT_PORT));
		_useOrbSmoothing    = deviceConfig["useOrbSmoothing"].toBool(false);
		_skipSmoothingDiff  = deviceConfig["skipSmoothingDiff"].toInt(0);
		QStringList orbIds = QStringUtils::split(deviceConfig["orbIds"].toString().simplified().remove(" "),",", QStringUtils::SplitBehavior::SkipEmptyParts);

		Debug(_log, "MulticastGroup    : %s", QSTRING_CSTR(_multicastGroup));
		Debug(_log, "MulticastGroupPort: %d", _multiCastGroupPort);
		Debug(_log, "Orb ID list       : %s", QSTRING_CSTR(deviceConfig["orbIds"].toString()));
		Debug(_log, "Use Orb Smoothing : %d", _useOrbSmoothing);
		Debug(_log, "Skip SmoothingDiff: %d", _skipSmoothingDiff);

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

		int numberOrbs = _orbIds.size();
		int configuredLedCount = this->getLedCount();

		if ( _orbIds.empty() )
		{
			this->setInError("No valid OrbIds found!");
			isInitOK = false;
		}
		else
		{
			if ( numberOrbs < configuredLedCount )
			{
				QString errorReason = QString("Not enough Orbs [%1] for configured LEDs [%2] found!")
										  .arg(numberOrbs)
										  .arg(configuredLedCount);
				this->setInError(errorReason);
				isInitOK = false;
			}
			else
			{
				if ( numberOrbs > configuredLedCount )
				{
					Info(_log, "%s: More Orbs [%d] than configured LEDs [%d].", QSTRING_CSTR(this->getActiveDeviceType()), numberOrbs, configuredLedCount );
				}

				isInitOK = true;
			}
		}
	}
	return isInitOK;
}

int LedDeviceAtmoOrb::open()
{
	int retval = -1;
	_isDeviceReady = false;

	if ( _udpSocket == nullptr )
	{
		_udpSocket = new QUdpSocket();
	}

	// Try to bind the UDP-Socket
	if ( _udpSocket != nullptr )
	{
		if ( _udpSocket->state() != QAbstractSocket::BoundState )
		{
			if ( !_udpSocket->bind(QHostAddress(QHostAddress::AnyIPv4), 0 ) )
			{
				QString errortext = QString ("Socket bind failed: (%1) %2, MulticastGroup: (%3)").arg(_udpSocket->error()).arg(_udpSocket->errorString(), _multicastGroup);
				this->setInError( errortext );
			}
			else
			{
				_groupAddress = QHostAddress(_multicastGroup);
				_joinedMulticastgroup = _udpSocket->joinMulticastGroup(_groupAddress);
				if ( !_joinedMulticastgroup )
				{
					QString errortext = QString ("Joining Multicastgroup failed: (%1) %2, MulticastGroup: (%3)").arg(_udpSocket->error()).arg(_udpSocket->errorString(), _multicastGroup);
					this->setInError( errortext );
				}
			}
		}

		if ( ! _isDeviceInError )
		{
			// Everything is OK, device is ready
			_isDeviceReady = true;
			retval = 0;
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
		if ( _udpSocket->state() == QAbstractSocket::BoundState )
		{
			_udpSocket->leaveMulticastGroup(_groupAddress);
		}

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

	ColorRgb color;
	for (int idx = 0; idx < _orbIds.size(); idx++ )
	{
		if ( idx < static_cast<int>(ledValues.size()) )
		{
			color = ledValues[idx];
		}
		else
		{
			color = ColorRgb::BLACK;
		}
		// Retrieve last send colors
		int lastRed = lastColorRedMap[idx];
		int lastGreen = lastColorGreenMap[idx];
		int lastBlue = lastColorBlueMap[idx];

		// If color difference is higher than _skipSmoothingDiff than we skip Orb smoothing (if enabled) and send it right away
		if ((_skipSmoothingDiff != 0 && _useOrbSmoothing) && (abs(color.red - lastRed) >= _skipSmoothingDiff || abs(color.blue - lastBlue) >= _skipSmoothingDiff ||
				abs(color.green - lastGreen) >= _skipSmoothingDiff))
		{
			// Skip Orb smoothing when using  (command type 4)
			commandType = 4;
		}

		// Send color
		setColor(_orbIds[idx], color, commandType);

		// Store last colors send for light id
		lastColorRedMap[idx]   = color.red;
		lastColorGreenMap[idx] = color.green;
		lastColorBlueMap[idx]  = color.blue;
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

	sendCommand(bytes);
}

void LedDeviceAtmoOrb::sendCommand(const QByteArray &bytes)
{
	DebugIf(verbose3, _log, "command: [%s] -> %s:%u", QSTRING_CSTR( QString(bytes.toHex())), QSTRING_CSTR(_groupAddress.toString()), _multiCastGroupPort );
	_udpSocket->writeDatagram(bytes.data(), bytes.size(), _groupAddress, _multiCastGroupPort);
}

QJsonObject LedDeviceAtmoOrb::discover(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );

	QJsonArray deviceList;

	_multicastGroup     = params["multiCastGroup"].toString(MULTICAST_GROUP_DEFAULT_ADDRESS);
	_multiCastGroupPort = static_cast<quint16>(params["multiCastPort"].toInt(MULTICAST_GROUP_DEFAULT_PORT));

	if ( open() == 0 )
	{
		Debug ( _log, "Send discovery requests to all AtmoOrbs listening to %s:%d", QSTRING_CSTR(_multicastGroup),_multiCastGroupPort );
		setColor(0, ColorRgb::BLACK, 8);

		if ( _udpSocket->waitForReadyRead(DEFAULT_DISCOVERY_TIMEOUT.count()) )
		{
			while (_udpSocket->waitForReadyRead(500))
			{
				QByteArray datagram;

				while (_udpSocket->hasPendingDatagrams())
				{
					datagram.resize(_udpSocket->pendingDatagramSize());
					QHostAddress senderIP;
					quint16 senderPort;

					_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderIP, &senderPort);

					if ( datagram.size() == 1 )
					{
						unsigned char orbId = datagram[0];
						if ( orbId > 0 )
						{
							Debug(_log, "Orb ID (%d) discovered at [%s]", orbId, QSTRING_CSTR(senderIP.toString()));
							_services.insert(orbId, senderIP);
						}
					}
				}
			}
		}

		close();
	}

	for (auto i = _services.begin(); i != _services.end(); ++i)
	{
		QJsonObject obj;

		obj.insert("id", i.key());
		obj.insert("ip", i.value().toString());

		QHostInfo hostInfo = QHostInfo::fromName(i.value().toString());
		if (hostInfo.error() == QHostInfo::NoError )
		{
			QString hostname = hostInfo.hostName();
			//Seems that for Windows no local domain name is resolved
			if (!hostInfo.localDomainName().isEmpty() )
			{
				obj.insert("hostname", hostname.remove("."+hostInfo.localDomainName()));
				obj.insert("domain", hostInfo.localDomainName());
			}
			else
			{
				int domainPos = hostname.indexOf('.');
				obj.insert("hostname", hostname.left(domainPos));
				obj.insert("domain", hostname.mid(domainPos+1));
			}
		}

		deviceList  << obj;
	}

	devicesDiscovered.insert("devices", deviceList);
	DebugIf(verbose, _log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	return devicesDiscovered;
}

void LedDeviceAtmoOrb::identify(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	int orbId = 0;
	if ( params["id"].isString() )
	{
		orbId = params["id"].toString().toInt();
	}
	else
	{
		orbId = params["id"].toInt();
	}

	if ( orbId >0 && orbId < 256 )
	{
		Debug (_log, "Orb ID [%d]", orbId);
		if ( open() == 0 )
		{
			setColor(orbId, ColorRgb::BLACK, 9);
			close();
		}
	}
	else
	{
		Warning(_log, "Identification of Orb with ID='%d' skipped. ID must be in range 1-255", orbId);
	}
}
