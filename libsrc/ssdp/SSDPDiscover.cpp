#include <ssdp/SSDPDiscover.h>

// qt inc
#include <QUdpSocket>
#include <QUrl>

static const QHostAddress SSDP_ADDR("239.255.255.250");
static const quint16      SSDP_PORT(1900);

// as per upnp spec 1.1, section 1.2.2.
// TODO: Make IP and port below another #define and replace message below
static const QString UPNP_DISCOVER_MESSAGE = "M-SEARCH * HTTP/1.1\r\n"
                                          "HOST: 239.255.255.250:1900\r\n"
                                          "MAN: \"ssdp:discover\"\r\n"
										  "MX: 1\r\n"
                                          "ST: %1\r\n"
                                          "\r\n";

SSDPDiscover::SSDPDiscover(QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("SSDPDISCOVER"))
	, _udpSocket(new QUdpSocket(this))
{

}

void SSDPDiscover::searchForService(const QString& st)
{
	_searchTarget = st;
	_usnList.clear();
	// setup socket
	connect(_udpSocket, &QUdpSocket::readyRead, this, &SSDPDiscover::readPendingDatagrams, Qt::UniqueConnection);

	sendSearch(st);
}

const QString SSDPDiscover::getFirstService(const searchType& type, const QString& st, const int& timeout_ms)
{
	Debug(_log, "Search for Service [%s]", QSTRING_CSTR(st));
	_searchTarget = st;

	// search
	sendSearch(st);

	_udpSocket->waitForReadyRead(timeout_ms);

    while (_udpSocket->hasPendingDatagrams())
	{
		QByteArray datagram;
		datagram.resize(_udpSocket->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

		QString data(datagram);

        Debug(_log, "_data: [%s]", QSTRING_CSTR(data));

		QMap<QString,QString> headers;
		QString address;
		// parse request
		QStringList entries = data.split("\n", QString::SkipEmptyParts);
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
		if(!headers.contains("st"))
			continue;

		// usn duplicates
		if (_usnList.contains(headers.value("usn")))
			continue;

		if (headers.value("st") == _searchTarget)
		{
			_usnList << headers.value("usn");
			QUrl url(headers.value("location"));
			//Debug(_log, "Received msearch response from '%s:%d'. Search target: %s",QSTRING_CSTR(sender.toString()), senderPort, QSTRING_CSTR(headers.value("st")));
			if(type == STY_WEBSERVER)
			{
				Debug(_log, "Found service [%s] at: %s:%d", QSTRING_CSTR(st), QSTRING_CSTR(url.host()), url.port());

				return url.host()+":"+QString::number(url.port());
			}
			else if(type == STY_FLATBUFSERVER)
			{
				const QString fbsport = headers.value("hyperion-fbs-port");
				if(fbsport.isEmpty())
				{
					continue;
				}
				else
				{
					Debug(_log, "Found service [%s] at: %s:%s", QSTRING_CSTR(st), QSTRING_CSTR(url.host()), QSTRING_CSTR(fbsport));
					return url.host()+":"+fbsport;
				}
			}
			else if(type == STY_JSONSERVER)
			{
				const QString jssport = headers.value("hyperion-jss-port");
				if(jssport.isEmpty())
				{
					continue;
				}
				else
				{
					Debug(_log, "Found service at: %s:%s", QSTRING_CSTR(url.host()), QSTRING_CSTR(jssport));
					return url.host()+":"+jssport;
				}
			}
		}
	}
	Debug(_log,"Search timeout, service [%s] not found", QSTRING_CSTR(st) );
	return QString();
}

void SSDPDiscover::readPendingDatagrams()
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
		QStringList entries = data.split("\n", QString::SkipEmptyParts);
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
		if(!headers.contains("st"))
			continue;

		// usn duplicates
		if (_usnList.contains(headers.value("usn")))
			continue;

		if (headers.value("st") == _searchTarget)
		{
			_usnList << headers.value("usn");
			//Debug(_log, "Received msearch response from '%s:%d'. Search target: %s",QSTRING_CSTR(sender.toString()), senderPort, QSTRING_CSTR(headers.value("st")));
			QUrl url(headers.value("location"));
			emit newService(url.host()+":"+QString::number(url.port()));
		}
	}
}

void SSDPDiscover::sendSearch(const QString& st)
{
	const QString msg = UPNP_DISCOVER_MESSAGE.arg(st);

    _udpSocket->writeDatagram(msg.toUtf8(),
							 QHostAddress(SSDP_ADDR),
							 SSDP_PORT);
}
