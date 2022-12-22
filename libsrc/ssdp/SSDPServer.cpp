#include <ssdp/SSDPServer.h>

// utils
#include <utils/SysInfo.h>
#include <utils/QStringUtils.h>

// Hyperion
#include <HyperionConfig.h>

#include <QUdpSocket>
#include <QDateTime>

static const QHostAddress SSDP_ADDR("239.255.255.250");
static const quint16      SSDP_PORT(1900);
static const QString      SSDP_MAX_AGE("1800");

// as per upnp spec 1.1, section 1.2.2.
//  - BOOTID.UPNP.ORG
//  - CONFIGID.UPNP.ORG
//  - SEARCHPORT.UPNP.ORG (optional)
// TODO: Make IP and port below another #define and replace message below
static const QString UPNP_ALIVE_MESSAGE = "NOTIFY * HTTP/1.1\r\n"
                                          "HOST: 239.255.255.250:1900\r\n"
                                          "CACHE-CONTROL: max-age=%1\r\n"
                                          "LOCATION: %2\r\n"
                                          "NT: %3\r\n"
                                          "NTS: ssdp:alive\r\n"
                                          "SERVER: %4\r\n"
                                          "USN: uuid:%5\r\n"
#if defined(ENABLE_FLATBUF_SERVER)
                                          "HYPERION-FBS-PORT: %6\r\n"
#endif
                                          "HYPERION-JSS-PORT: %7\r\n"
                                          "HYPERION-NAME: %8\r\n"
                                          "\r\n";

// Implement ssdp:update as per spec 1.1, section 1.2.4
// and use the below define to build the message, where
// SEARCHPORT.UPNP.ORG are optional.
// TODO: Make IP and port below another #define and replace message below
static const QString UPNP_UPDATE_MESSAGE = "NOTIFY * HTTP/1.1\r\n"
                                           "HOST: 239.255.255.250:1900\r\n"
                                           "LOCATION: %1\r\n"
                                           "NT: %2\r\n"
                                           "NTS: ssdp:update\r\n"
                                           "USN: uuid:%3\r\n"
/*                                         "CONFIGID.UPNP.ORG: %4\r\n"
UPNP spec = 1.1                            "NEXTBOOTID.UPNP.ORG: %5\r\n"
                                           "SEARCHPORT.UPNP.ORG: %6\r\n"
*/                                         "\r\n";

// TODO: Add this two fields commented below in the BYEBYE MESSAGE
// as per upnp spec 1.1, section 1.2.2 and 1.2.3.
//  - BOOTID.UPNP.ORG
//  - CONFIGID.UPNP.ORG
// TODO: Make IP and port below another #define and replace message below
static const QString UPNP_BYEBYE_MESSAGE = "NOTIFY * HTTP/1.1\r\n"
                                           "HOST: 239.255.255.250:1900\r\n"
                                           "NT: %1\r\n"
                                           "NTS: ssdp:byebye\r\n"
                                           "USN: uuid:%2\r\n"
                                           "\r\n";

// TODO: Add this three fields commented below in the MSEARCH_RESPONSE
// as per upnp spec 1.1, section 1.3.3.
//  - BOOTID.UPNP.ORG
//  - CONFIGID.UPNP.ORG
//  - SEARCHPORT.UPNP.ORG (optional)
static const QString UPNP_MSEARCH_RESPONSE = "HTTP/1.1 200 OK\r\n"
                                             "CACHE-CONTROL: max-age = %1\r\n"
                                             "DATE: %2\r\n"
                                             "EXT: \r\n"
                                             "LOCATION: %3\r\n"
                                             "SERVER: %4\r\n"
                                             "ST: %5\r\n"
                                             "USN: uuid:%6\r\n"
#if defined(ENABLE_FLATBUF_SERVER)
                                             "HYPERION-FBS-PORT: %7\r\n"
#endif
                                             "HYPERION-JSS-PORT: %8\r\n"
                                             "HYPERION-NAME: %9\r\n"
                                             "\r\n";

SSDPServer::SSDPServer(QObject * parent)
	: QObject(parent)
	, _log(Logger::getInstance("SSDP"))
	, _udpSocket(nullptr)
	, _running(false)
{

}

SSDPServer::~SSDPServer()
{
	stop();
}

void SSDPServer::initServer()
{
	_udpSocket = new QUdpSocket(this);

	// get system info
	SysInfo::HyperionSysInfo data = SysInfo::get();

	// create SERVER String
	_serverHeader = QString("%1/%2 UPnP/1.0 Hyperion/%3")
				.arg(data.prettyName, data.productVersion, HYPERION_VERSION);

	connect(_udpSocket, &QUdpSocket::readyRead, this, &SSDPServer::readPendingDatagrams);
}

bool SSDPServer::start()
{
	if(!_running && _udpSocket->bind(QHostAddress::AnyIPv4, SSDP_PORT, QUdpSocket::ReuseAddressHint | QUdpSocket::ShareAddress))
	{
		_udpSocket->joinMulticastGroup(SSDP_ADDR);
		_running = true;
		return true;
	}
	return false;
}

void SSDPServer::stop()
{
	if(_running)
	{
		_udpSocket->close();
		_running = false;
	}
}

void SSDPServer::readPendingDatagrams()
{
    while (_udpSocket->hasPendingDatagrams()) {

		QByteArray datagram;
		datagram.resize(_udpSocket->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

		QString data(datagram);
		QMap<QString,QString> headers;
		// parse request
		QStringList entries = QStringUtils::split(data,"\n", QStringUtils::SplitBehavior::SkipEmptyParts);
		for(auto entry : entries)
		{
			// http header parse skip
			if(entry.contains("HTTP/1.1"))
				continue;

			// split into key:vale, be aware that value field may contain also a ":"
			entry = entry.simplified();
			int pos = entry.indexOf(":");
			if(pos == -1)
				continue;

			headers[entry.left(pos).trimmed().toLower()] = entry.mid(pos+1).trimmed();
		}

		// verify ssdp spec
		if(!headers.contains("man"))
			continue;

		if (headers.value("man") == "\"ssdp:discover\"")
		{
			emit msearchRequestReceived(headers.value("st"), headers.value("mx"), sender.toString(), senderPort);
		}
    }
}

void SSDPServer::sendMSearchResponse(const QString& st, const QString& senderIp, quint16 senderPort)
{
	QString message = UPNP_MSEARCH_RESPONSE.arg(SSDP_MAX_AGE
		, QDateTime::currentDateTimeUtc().toString("ddd, dd MMM yyyy HH:mm:ss GMT")
		, _descAddress
		, _serverHeader
		, st
		, _uuid
#if defined(ENABLE_FLATBUF_SERVER)
		, _fbsPort
#endif
		, _jssPort
		, _name );

	_udpSocket->writeDatagram(message.toUtf8(), QHostAddress(senderIp), senderPort);
}

void SSDPServer::sendByeBye(const QString& st)
{
	QString message = UPNP_BYEBYE_MESSAGE.arg(st, _uuid+"::"+st );

	// we repeat 3 times
	quint8 rep = 0;
	while(rep++ < 3) {
		_udpSocket->writeDatagram(message.toUtf8(), QHostAddress(SSDP_ADDR), SSDP_PORT);
	}
}

void SSDPServer::sendAlive(const QString& st)
{
	const QString tempUSN = (st == "upnp:rootdevice ") ? _uuid+"::"+st  : _uuid;

	QString message = UPNP_ALIVE_MESSAGE.arg(SSDP_MAX_AGE
		, _descAddress
		, st
		, _serverHeader
		, tempUSN
#if defined(ENABLE_FLATBUF_SERVER)
		, _fbsPort
#endif
		, _jssPort
		, _name );

	// we repeat 3 times
	quint8 rep = 0;
	while(rep++ < 3) {
		_udpSocket->writeDatagram(message.toUtf8(), QHostAddress(SSDP_ADDR), SSDP_PORT);
	}
}

void SSDPServer::sendUpdate(const QString& st)
{
	QString message = UPNP_UPDATE_MESSAGE.arg(_descAddress
		, st
		, _uuid+"::"+st );

	_udpSocket->writeDatagram(message.toUtf8(), QHostAddress(SSDP_ADDR), SSDP_PORT);
}
