#include "LedDeviceCololight.h"

#include <utils/QStringUtils.h>
#include <QUdpSocket>
#include <QHostInfo>
#include <QtEndian>
#include <QEventLoop>

#include <chrono>

// Constants
namespace {
const bool verbose = false;
const bool verbose3 = false;

// Configuration settings

const char CONFIG_HW_LED_COUNT[] = "hardwareLedCount";

// Cololight discovery service

const int API_DEFAULT_PORT = 8900;

const char DISCOVERY_ADDRESS[] = "255.255.255.255";
const quint16 DISCOVERY_PORT = 12345;
const char DISCOVERY_MESSAGE[] = "Z-SEARCH * \r\n";
constexpr std::chrono::milliseconds DEFAULT_DISCOVERY_TIMEOUT{ 5000 };
constexpr std::chrono::milliseconds DEFAULT_READ_TIMEOUT{ 1000 };
constexpr std::chrono::milliseconds DEFAULT_IDENTIFY_TIME{ 2000 };

const char COLOLIGHT_MODEL[] = "mod";
const char COLOLIGHT_MODEL_TYPE[] = "subkey";
const char COLOLIGHT_MAC[] = "sn";
const char COLOLIGHT_NAME[] = "name";

const char COLOLIGHT_MODEL_IDENTIFIER[] = "OD_WE_QUAN";

const int COLOLIGHT_BEADS_PER_MODULE = 19;
const int COLOLIGHT_MIN_STRIP_SEGMENT_SIZE = 30;

enum verbs {
	GET = 0x03,
	SET = 0x04,
	SETEEPROM = 0x07,
	SETVAR = 0x0b
};

enum commandTypes {
	STATE_OFF = 0x80,
	STATE_ON = 0x81,
	BRIGTHNESS = 0xCF,
	SETCOLOR = 0xFF
};

enum idxTypes {
	BRIGTHNESS_CONTROL = 0x01,
	COLOR_CONTROL = 0x02,
	COLOR_DIRECT_CONTROL = 0x81,
	READ_INFO_FROM_STORAGE = 0x86
};

	enum bufferMode {
		MONOCROME = 0x01,
		LIGHTBEAD = 0x02,
		};

enum ledLayout {
	STRIP_LAYOUT,
	MODLUE_LAYOUT
};

enum modelType {
	STRIP,
	PLUS
};

const uint8_t PACKET_HEADER[] =
	{
		'S', 'Z',   // Tag "SZ"
		0x30, 0x30, // Version "00"
		0x00, 0x00, // AppID, 0x0000 = TL1 command mode
		0x00, 0x00, 0x00, 0x00 // Size
};

const uint8_t PACKET_SECU[] =
	{
		0x00, 0x00, 0x00, 0x00, // Dict
		0x00, 0x00, 0x00, 0x00, // Sum
		0x00, 0x00, 0x00, 0x00, // Salt
		0x00, 0x00, 0x00, 0x00 // SN
};

const uint8_t TL1_CMD_FIXED_PART[] =
	{
		0x00, 0x00, 0x00, 0x00, // DISTID
		0x00, 0x00, 0x00, 0x00, // SRCID
		0x00, // SECU
		0x00, // VERB
		0x00, // CTAG
		0x00 // LENGTH
};
} //End of constants

LedDeviceCololight::LedDeviceCololight(const QJsonObject& deviceConfig)
	: ProviderUdp(deviceConfig)
	  , _modelType(-1)
	  , _ledLayoutType(STRIP_LAYOUT)
	  , _ledBeadCount(0)
	  , _distance(0)
	  , _sequenceNumber(1)
{
	_packetFixPart.append(reinterpret_cast<const char*>(PACKET_HEADER), sizeof(PACKET_HEADER));
	_packetFixPart.append(reinterpret_cast<const char*>(PACKET_SECU), sizeof(PACKET_SECU));
}

LedDevice* LedDeviceCololight::construct(const QJsonObject& deviceConfig)
{
	return new LedDeviceCololight(deviceConfig);
}

bool LedDeviceCololight::init(const QJsonObject& deviceConfig)
{
	bool isInitOK = false;

	_port = API_DEFAULT_PORT;

	if (ProviderUdp::init(deviceConfig))
	{
		// Initialise LedDevice configuration and execution environment
		Debug(_log, "DeviceType   : %s", QSTRING_CSTR(this->getActiveDeviceType()));
		Debug(_log, "ColorOrder   : %s", QSTRING_CSTR(this->getColorOrder()));
		Debug(_log, "LatchTime    : %d", this->getLatchTime());

		if (initLedsConfiguration())
		{
			initDirectColorCmdTemplate();
			isInitOK = true;
		}
	}
	return isInitOK;
}

bool LedDeviceCololight::initLedsConfiguration()
{
	bool isInitOK = false;

	if (!getInfo())
	{
		QString errorReason = QString("Cololight device (%1) not accessible to get additional properties!")
								  .arg(getAddress().toString());
		setInError(errorReason);
	}
	else
	{
		QString modelTypeText;

		switch (_modelType) {
		case 0:
			modelTypeText = "Strip";
			_ledLayoutType = STRIP_LAYOUT;
			break;
		case 1:
			_ledLayoutType = MODLUE_LAYOUT;
			modelTypeText = "Plus";
			break;
		default:
			_modelType = STRIP;
			modelTypeText = "Strip";
			_ledLayoutType = STRIP_LAYOUT;
			Info(_log, "Model not identified, assuming Cololight %s", QSTRING_CSTR(modelTypeText));
			break;
		}
		Debug(_log, "Model type   : %s", QSTRING_CSTR(modelTypeText));

		if (getLedCount() == 0)
		{
			setLedCount(static_cast<uint>(_devConfig[CONFIG_HW_LED_COUNT].toInt(0)));
		}

		if (_modelType == STRIP && (getLedCount() % COLOLIGHT_MIN_STRIP_SEGMENT_SIZE != 0))
		{
			QString errorReason = QString("Hardware LED count must be multiple of %1 for Cololight Strip!")
									  .arg(COLOLIGHT_MIN_STRIP_SEGMENT_SIZE);
			this->setInError(errorReason);
		}
		else
		{
			Debug(_log, "LedCount     : %d", getLedCount());

			uint configuredLedCount = static_cast<uint>(_devConfig["currentLedCount"].toInt(1));

			if (getLedCount() < configuredLedCount)
			{
				QString errorReason = QString("Not enough LEDs [%1] for configured LEDs in layout [%2] found!")
										  .arg(getLedCount())
										  .arg(configuredLedCount);
				this->setInError(errorReason);
			}
			else
			{
				if (getLedCount() > configuredLedCount)
				{
					Info(_log, "%s: More LEDs [%u] than configured LEDs in layout [%u].", QSTRING_CSTR(this->getActiveDeviceType()), getLedCount(), configuredLedCount);
				}
				isInitOK = true;
			}
		}
	}

	return isInitOK;
}

void LedDeviceCololight::initDirectColorCmdTemplate()
{
	int ledNumber = static_cast<int>(this->getLedCount());

	_directColorCommandTemplate.clear();

	//Packet
	_directColorCommandTemplate.append(static_cast<char>(bufferMode::LIGHTBEAD)); // idx

	int beads = 1;
	if (_ledLayoutType == MODLUE_LAYOUT)
	{
		beads = COLOLIGHT_BEADS_PER_MODULE;
	}

	for (int i = 0; i < ledNumber; ++i)
	{
		_directColorCommandTemplate.append(static_cast<char>(i * beads + 1));
		_directColorCommandTemplate.append(static_cast<char>(i * beads + beads));
		_directColorCommandTemplate.append(3, static_cast<char>(0x00));
	}
}

bool LedDeviceCololight::getInfo()
{
	bool isCmdOK = false;

	QByteArray command;

	const quint8 packetSize = 2;
	int  fixPartsize = sizeof(TL1_CMD_FIXED_PART);

	command.resize(sizeof(TL1_CMD_FIXED_PART) + packetSize);
	command.fill('\0');

	command[fixPartsize - 3] = static_cast<char>(SETVAR); // verb
	command[fixPartsize - 2] = static_cast<char>(_sequenceNumber); // ctag
	command[fixPartsize - 1] = static_cast<char>(packetSize); // length

	//Packet
	command[fixPartsize] = static_cast<char>(READ_INFO_FROM_STORAGE); // idx
	command[fixPartsize + 1] = static_cast<char>(0x01); // idx

	if (sendRequest(TL1_CMD, command))
	{
		QByteArray response;
		if (readResponse(response))
		{
			DebugIf(verbose, _log, "#[0x%x], Data returned: [%s]", _sequenceNumber, QSTRING_CSTR(toHex(response)));

			quint16 ledNum = qFromBigEndian<quint16>(response.data() + 1);

			if (ledNum != 0xFFFF)
			{
				_ledBeadCount = ledNum;
				if (ledNum % COLOLIGHT_BEADS_PER_MODULE == 0)
				{
					_modelType = MODLUE_LAYOUT;
					_distance = ledNum / COLOLIGHT_BEADS_PER_MODULE;
					setLedCount(static_cast<uint>(_distance));
				}
			}
			else
			{
				_modelType = STRIP;
				setLedCount(0);
			}

			Debug(_log, "#LEDs found [0x%x], [%u], distance [%d]", _ledBeadCount, _ledBeadCount, _distance);

			isCmdOK = true;
		}
	}

	return isCmdOK;
}

bool LedDeviceCololight::setEffect(const effect effect)
{
	return setColor(static_cast<uint32_t>(effect));
}

bool LedDeviceCololight::setColor(const ColorRgb colorRgb)
{
	uint32_t color = colorRgb.blue | (colorRgb.green << 8) | (colorRgb.red << 16) | (0x00 << 24);

	return setColor(color);
}

bool LedDeviceCololight::setColor(const uint32_t color)
{
	bool isCmdOK = false;

	QByteArray command;

	const quint8 packetSize = 6;
	int  fixPartsize = sizeof(TL1_CMD_FIXED_PART);

	command.resize(sizeof(TL1_CMD_FIXED_PART) + packetSize);
	command.fill('\0');

	command[fixPartsize - 3] = static_cast<char>(SET); // verb
	command[fixPartsize - 2] = static_cast<char>(_sequenceNumber); // ctag
	command[fixPartsize - 1] = static_cast<char>(packetSize); // length

	//Packet
	command[fixPartsize] = static_cast<char>(0x02); // idx
	command[fixPartsize + 1] = static_cast<char>(0xff); // set color or dynamic effect

	qToBigEndian<quint32>(color, command.data() + fixPartsize + 2);

	if (sendRequest(TL1_CMD, command))
	{
		QByteArray response;
		if (readResponse(response))
		{
			DebugIf(verbose, _log, "#[0x%x], Data returned: [%s]", _sequenceNumber, QSTRING_CSTR(toHex(response)));
			isCmdOK = true;
		}
	}

	return isCmdOK;
}

bool LedDeviceCololight::setState(bool isOn)
{
	bool isCmdOK = false;

	quint8 type = isOn ? STATE_ON : STATE_OFF;

	QByteArray command;

	const quint8 packetSize = 3;
	int  fixPartsize = sizeof(TL1_CMD_FIXED_PART);

	command.resize(sizeof(TL1_CMD_FIXED_PART) + packetSize);
	command.fill('\0');

	command[fixPartsize - 3] = static_cast<char>(SET); // verb
	command[fixPartsize - 2] = static_cast<char>(_sequenceNumber); // ctag
	command[fixPartsize - 1] = static_cast<char>(packetSize); // length

	//Packet
	command[fixPartsize] = static_cast<char>(BRIGTHNESS_CONTROL); // idx
	command[fixPartsize + 1] = static_cast<char>(type);	// type
	command[fixPartsize + 2] = static_cast<char>(isOn);	// value

	if (sendRequest(TL1_CMD, command))
	{
		QByteArray response;
		if (readResponse(response))
		{
			DebugIf(verbose, _log, "#[0x%x], Data returned: [%s]", _sequenceNumber, QSTRING_CSTR(toHex(response)));
			isCmdOK = true;
		}
	}

	return isCmdOK;
}

bool LedDeviceCololight::setStateDirect(bool isOn)
{
	bool isCmdOK = false;

	QByteArray command;

	//Packet
	command.append(static_cast<char>(0x04)); // idx
	command.append(static_cast<char>(isOn)); // idx
	command.append(static_cast<char>(0xd7)); // idx

	if (sendRequest(DIRECT_CONTROL, command))
	{
		QByteArray response;
		if (readResponse(response))
		{
			DebugIf(verbose, _log, "#[0x%x], Data returned: [%s]", _sequenceNumber, QSTRING_CSTR(toHex(response)));
			isCmdOK = true;
		}
	}

	return isCmdOK;
}

bool LedDeviceCololight::setColor(const std::vector<ColorRgb>& ledValues)
{
	int ledNumber = static_cast<int>(ledValues.size());

	QByteArray command = _directColorCommandTemplate;

	//Update LED values, start from offset (mode + first start/stop pair) = 3
	for (int i = 0; i < ledNumber; ++i)
	{
		command[3 + i * 5] = static_cast<char>(ledValues[i].red);
		command[3 + i * 5 + 1] = static_cast<char>(ledValues[i].green);
		command[3 + i * 5 + 2] = static_cast<char>(ledValues[i].blue);
	}

	bool isCmdOK = sendRequest(DIRECT_CONTROL, command);

	return isCmdOK;
}

bool LedDeviceCololight::setTL1CommandMode(bool isOn)
{
	bool isCmdOK = false;

	quint8 type = isOn ? STATE_ON : STATE_OFF;

	QByteArray command;

	const quint8 packetSize = 2;
	int  fixPartsize = sizeof(TL1_CMD_FIXED_PART);

	command.resize(sizeof(TL1_CMD_FIXED_PART) + packetSize);
	command.fill('\0');

	command[fixPartsize - 3] = static_cast<char>(SETEEPROM); // verb
	command[fixPartsize - 2] = static_cast<char>(_sequenceNumber); // ctag
	command[fixPartsize - 1] = static_cast<char>(packetSize); // length

	//Packet
	command[fixPartsize] = static_cast<char>(COLOR_CONTROL); // idx
	command[fixPartsize + 1] = static_cast<char>(type);	// type

	if (sendRequest(TL1_CMD, command))
	{
		QByteArray response;
		if (readResponse(response))
		{
			DebugIf(verbose, _log, "#[0x%x], Data returned: [%s]", _sequenceNumber, QSTRING_CSTR(toHex(response)));
			isCmdOK = true;
		}
	}

	return isCmdOK;
}

bool LedDeviceCololight::sendRequest(const appID appID, const QByteArray& command)
{
	bool isSendOK = true;
	QByteArray packet(_packetFixPart);
	packet.append(static_cast<char>(_sequenceNumber));
	packet.append(command);

	quint32 size = sizeof(PACKET_SECU) + 1 + command.size();

	qToBigEndian<quint16>(appID, packet.data() + 4);

	qToBigEndian<quint32>(size, packet.data() + 6);

	++_sequenceNumber;

	DebugIf(verbose3, _log, "packet: ([0x%x], [%u])[%s]", size, size, QSTRING_CSTR(toHex(packet, 64)));

	if (writeBytes(packet) < 0)
	{
		isSendOK = false;
	}

	return isSendOK;
}

bool LedDeviceCololight::readResponse()
{
	QByteArray response;
	return readResponse(response);
}

bool LedDeviceCololight::readResponse(QByteArray& response)
{
	bool isRequestOK = false;
	if (_udpSocket->waitForReadyRead(DEFAULT_READ_TIMEOUT.count()))
	{
		while (_udpSocket->waitForReadyRead(200))
		{
			QByteArray datagram;

			while (_udpSocket->hasPendingDatagrams())
			{
				datagram.resize(static_cast<int>(_udpSocket->pendingDatagramSize()));
				QHostAddress senderIP;
				quint16 senderPort;

				_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderIP, &senderPort);

				if (datagram.size() >= 10)
				{
					DebugIf(verbose3, _log, "response: [%s]", QSTRING_CSTR(toHex(datagram, 64)));

					quint16 appID = qFromBigEndian<quint16>(datagram.mid(4, sizeof(appID)));

					if (verbose && appID == 0x8000)
					{
						QString tagVersion = datagram.left(2);
						quint32 packetSize = qFromBigEndian<quint32>(datagram.mid(sizeof(PACKET_HEADER) - sizeof(packetSize)));

						Debug(_log, "Response HEADER: tagVersion [%s], appID: [0x%.2x][%u], packet size: [0x%.4x][%u]", QSTRING_CSTR(tagVersion), appID, appID, packetSize, packetSize);

						quint32 dictionary = qFromBigEndian<quint32>(datagram.mid(sizeof(PACKET_HEADER)));
						quint32 checkSum = qFromBigEndian<quint32>(datagram.mid(sizeof(PACKET_HEADER) + sizeof(dictionary)));
						quint32 salt = qFromBigEndian<quint32>(datagram.mid(sizeof(PACKET_HEADER) + sizeof(dictionary) + sizeof(checkSum), sizeof(salt)));
						quint32 sequenceNumber = qFromBigEndian<quint32>(datagram.mid(sizeof(PACKET_HEADER) + sizeof(dictionary) + sizeof(checkSum) + sizeof(salt)));

						Debug(_log, "Response SECU  : Dict: [0x%.4x][%u], Sum: [0x%.4x][%u], Salt: [0x%.4x][%u], SN: [0x%.4x][%u]", dictionary, dictionary, checkSum, checkSum, salt, salt, sequenceNumber, sequenceNumber);

						quint8 packetSN = static_cast<quint8>(datagram.at(sizeof(PACKET_HEADER) + sizeof(PACKET_SECU)));
						Debug(_log, "Response packSN: [0x%.4x][%u]", packetSN, packetSN);
					}

					quint8 errorCode = static_cast<quint8>(datagram.at(sizeof(PACKET_HEADER) + sizeof(PACKET_SECU) + 1));

					int dataPartStart = sizeof(PACKET_HEADER) + sizeof(PACKET_SECU) + sizeof(TL1_CMD_FIXED_PART);

					if (errorCode != 0)
					{
						quint8 originalVerb = static_cast<quint8>(datagram.at(dataPartStart - 2) - 0x80);
						quint8 originalRequestPacketSN = static_cast<quint8>(datagram.at(dataPartStart - 1));

						if (errorCode == 16)
						{
							//TL1 Command failure
							Error(_log, "Request [0x%x] failed =with error [%u], appID [%u], originalVerb [0x%x]", originalRequestPacketSN, errorCode, appID, originalVerb);
						}
						else
						{
							Error(_log, "Request [0x%x] failed with error [%u], appID [%u]", originalRequestPacketSN, errorCode, appID);
						}
					}
					else
					{
						// TL1 Protocol
						if (appID == 0x8000)
						{
							if (dataPartStart < datagram.size())
							{
								quint8 dataLength = static_cast<quint8>(datagram.at(dataPartStart));

								response = datagram.mid(dataPartStart + 1, dataLength);
								if (verbose)
								{
									quint8 originalVerb = static_cast<quint8>(datagram.at(dataPartStart - 2) - 0x80);
									Debug(_log, "Cmd [0x%x], Data returned: [%s]", originalVerb, QSTRING_CSTR(toHex(response)));
								}
							}
							else
							{
								DebugIf(verbose, _log, "No additional data returned");
							}
						}
						isRequestOK = true;
					}
				}
			}
		}
	}
	return isRequestOK;
}

int LedDeviceCololight::write(const std::vector<ColorRgb>& ledValues)
{
	int rc = -1;

	if (setColor(ledValues))
	{
		rc = 0;
	}

	return rc;
}

bool LedDeviceCololight::powerOn()
{
	bool on = true;
	if (_isDeviceReady)
	{
		if (!setState(false) || !setTL1CommandMode(false))
		{
			on = false;
		}
	}
	return on;
}

bool LedDeviceCololight::powerOff()
{
	bool off = true;
	if (_isDeviceReady)
	{
		writeBlack();
		off = setStateDirect(false);
		setTL1CommandMode(false);
	}
	return off;
}

QJsonObject LedDeviceCololight::discover()
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType);

	QJsonArray deviceList;

	QUdpSocket udpSocket;

	udpSocket.writeDatagram(QString(DISCOVERY_MESSAGE).toUtf8(), QHostAddress(DISCOVERY_ADDRESS), DISCOVERY_PORT);

	if (udpSocket.waitForReadyRead(DEFAULT_DISCOVERY_TIMEOUT.count()))
	{
		while (udpSocket.waitForReadyRead(500))
		{
			QByteArray datagram;

			while (udpSocket.hasPendingDatagrams())
			{
				datagram.resize(static_cast<int>(udpSocket.pendingDatagramSize()));
				QHostAddress senderIP;
				quint16 senderPort;

				udpSocket.readDatagram(datagram.data(), datagram.size(), &senderIP, &senderPort);

				QString data(datagram);

				QMap<QString, QString> headers;
				// parse request
				QStringList entries = QStringUtils::split(data, "\n", QStringUtils::SplitBehavior::SkipEmptyParts);
				for (auto entry : entries)
				{
					// split into key=value, be aware that value field may contain also a "="
					entry = entry.simplified();
					int pos = entry.indexOf("=");
					if (pos == -1)
					{
						continue;
					}

					const QString key = entry.left(pos).trimmed().toLower();
					const QString value = entry.mid(pos + 1).trimmed();
					headers[key] = value;
				}

				if (headers.value("mod") == COLOLIGHT_MODEL_IDENTIFIER)
				{
					QString ipAddress = QHostAddress(senderIP.toIPv4Address()).toString();
					_services.insert(ipAddress, headers);

					Debug(_log, "Cololight discovered at [%s]", QSTRING_CSTR(ipAddress));
					DebugIf(verbose3, _log, "_data: [%s]", QSTRING_CSTR(data));
				}
			}
		}
	}

	QMap<QString, QMap <QString, QString>>::iterator i;
	for (i = _services.begin(); i != _services.end(); ++i)
	{
		QJsonObject obj;

		obj.insert("ip", i.key());
		obj.insert("model", i.value().value(COLOLIGHT_MODEL));
		obj.insert("type", i.value().value(COLOLIGHT_MODEL_TYPE));
		obj.insert("mac", i.value().value(COLOLIGHT_MAC));
		obj.insert("name", i.value().value(COLOLIGHT_NAME));

		QHostInfo hostInfo = QHostInfo::fromName(i.key());
		if (hostInfo.error() == QHostInfo::NoError)
		{
			QString hostname = hostInfo.hostName();
			//Seems that for Windows no local domain name is resolved
			if (!QHostInfo::localDomainName().isEmpty())
			{
				obj.insert("hostname", hostname.remove("." + QHostInfo::localDomainName()));
				obj.insert("domain", QHostInfo::localDomainName());
			}
			else
			{
				int domainPos = hostname.indexOf('.');
				obj.insert("hostname", hostname.left(domainPos));
				obj.insert("domain", hostname.mid(domainPos + 1));
			}
		}

		deviceList << obj;
	}

	devicesDiscovered.insert("devices", deviceList);
	DebugIf(verbose, _log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return devicesDiscovered;
}

QJsonObject LedDeviceCololight::getProperties(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());
	QJsonObject properties;

	QString apiHostname = params["host"].toString("");
	quint16 apiPort = static_cast<quint16>(params["port"].toInt(API_DEFAULT_PORT));

	if (!apiHostname.isEmpty())
	{
		QJsonObject deviceConfig;

		deviceConfig.insert("host", apiHostname);
		deviceConfig.insert("port", apiPort);
		if (ProviderUdp::init(deviceConfig))
		{
			if (getInfo())
			{
				QString modelTypeText;

				switch (_modelType) {
				case 1:
					modelTypeText = "Plus";
					break;
				default:
					modelTypeText = "Strip";
					break;
				}
				properties.insert("modelType", modelTypeText);
				properties.insert("ledCount", static_cast<int>(getLedCount()));
				properties.insert("ledBeadCount", _ledBeadCount);
				properties.insert("distance", _distance);
			}
		}
	}

	DebugIf(verbose, _log, "properties: [%s]", QString(QJsonDocument(properties).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return properties;
}

void LedDeviceCololight::identify(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QString apiHostname = params["host"].toString("");
	quint16 apiPort = static_cast<quint16>(params["port"].toInt(API_DEFAULT_PORT));

	if (!apiHostname.isEmpty())
	{
		QJsonObject deviceConfig;

		deviceConfig.insert("host", apiHostname);
		deviceConfig.insert("port", apiPort);
		if (ProviderUdp::init(deviceConfig))
		{
			if (setStateDirect(false) && setState(true))
			{
				setEffect(THE_CIRCUS);

				QEventLoop loop;
				QTimer::singleShot(DEFAULT_IDENTIFY_TIME.count(), &loop, &QEventLoop::quit);
				loop.exec();

				setColor(ColorRgb::BLACK);
			}
		}
	}
}
