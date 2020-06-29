#include <ssdp/SSDPDiscover.h>

// Qt includes
#include <QUdpSocket>
#include <QUrl>
#include <QRegularExpression>
#include <QJsonObject>
#include <QtNetwork>

#include <iostream>

// as per upnp spec 1.1, section 1.2.2.
static const QString UPNP_DISCOVER_MESSAGE = "M-SEARCH * HTTP/1.1\r\n"
										  "HOST: %1:%2\r\n"
                                          "MAN: \"ssdp:discover\"\r\n"
										  "MX: %3\r\n"
										  "ST: %4\r\n"
                                          "\r\n";

SSDPDiscover::SSDPDiscover(QObject* parent)
	: QObject(parent)
	  , _log(Logger::getInstance("SSDPDISCOVER"))
	  , _udpSocket(new QUdpSocket(this))
	  , _ssdpAddr(DEFAULT_SEARCH_ADDRESS)
	  , _ssdpPort(DEFAULT_SEARCH_PORT)
	  , _ssdpMaxWaitResponseTime(1)
	  , _ssdpTimeout(DEFAULT_SSDP_TIMEOUT)
	  ,_filter(DEFAULT_FILTER)
	  ,_filterHeader(DEFAULT_FILTER_HEADER)
	  ,_regExFilter(_filter)
	  ,_skipDupKeys(false)
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
	_searchTarget = st;
	_services.clear();
	Debug(_log, "Search for Service [%s], address [%s], port [%d]", QSTRING_CSTR(_searchTarget), QSTRING_CSTR(_ssdpAddr.toString()), _ssdpPort);

	// search
	sendSearch(_searchTarget);

	if ( _udpSocket->waitForReadyRead(timeout_ms) )
	{
		while (_udpSocket->waitForReadyRead(500))
		{
			QByteArray datagram;
			while (_udpSocket->hasPendingDatagrams())
			{
				datagram.resize(_udpSocket->pendingDatagramSize());
				QHostAddress sender;
				quint16 senderPort;

				_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

				QString data(datagram);

				//Debug(_log, "_data: [%s]", QSTRING_CSTR(data));

				QMap<QString,QString> headers;
				QString address;
				// parse request

				#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
					QStringList entries = data.split("\n", Qt::SkipEmptyParts);
				#else
					QStringList entries = data.split("\n", QString::SkipEmptyParts);
				#endif

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
					if(type == searchType::STY_WEBSERVER)
					{
						Debug(_log, "Found service [%s] at: %s:%d", QSTRING_CSTR(st), QSTRING_CSTR(url.host()), url.port());

						return url.host()+":"+QString::number(url.port());
					}
					else if(type == searchType::STY_FLATBUFSERVER)
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
					else if(type == searchType::STY_JSONSERVER)
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
		#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
			QStringList entries = data.split("\n", Qt::SkipEmptyParts);
		#else
			QStringList entries = data.split("\n", QString::SkipEmptyParts);
		#endif
		for(auto entry : entries)
		{
			// http header parse skip
			if(entry.contains("HTTP/1.1"))
				continue;

			// split into key:value, be aware that value field may contain also a ":"
			entry = entry.simplified();
			int pos = entry.indexOf(":");
			if(pos == -1)
				continue;

			const QString key = entry.left(pos).trimmed().toLower();
			const QString value = entry.mid(pos + 1).trimmed();
			headers[key] = value;
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
			emit newService(url.host() + ":" + QString::number(url.port()));
		}
	}
}

int SSDPDiscover::discoverServices(const QString& searchTarget, const QString& key)
{
	_searchTarget = searchTarget;
	int rc = -1;

	Debug(_log, "Search for Service [%s], address [%s], port [%d]", QSTRING_CSTR(_searchTarget), QSTRING_CSTR(_ssdpAddr.toString()), _ssdpPort);

	_services.clear();

	// search
	sendSearch(_searchTarget);

	if ( _udpSocket->waitForReadyRead( 	_ssdpTimeout ) )
	{
		while (_udpSocket->waitForReadyRead(500))
		{
			QByteArray datagram;
			while (_udpSocket->hasPendingDatagrams())
			{

				datagram.resize(_udpSocket->pendingDatagramSize());
				QHostAddress sender;
				quint16 senderPort;

				_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

				QString data(datagram);

				//Debug(_log, "_data: [%s]", QSTRING_CSTR(data));

				QMap<QString,QString> headers;
				// parse request
				#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
					QStringList entries = data.split("\n", Qt::SkipEmptyParts);
				#else
					QStringList entries = data.split("\n", QString::SkipEmptyParts);
				#endif

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

					headers[entry.left(pos).trimmed().toUpper()] = entry.mid(pos+1).trimmed();
				}

				QRegularExpressionMatch match = _regExFilter.match(headers[_filterHeader]);
				if ( match.hasMatch() )
				{
					Debug(_log,"Found target [%s], plus record [%s] matches [%s:%s]", QSTRING_CSTR(_searchTarget), QSTRING_CSTR(headers[_filterHeader]), QSTRING_CSTR(_filterHeader), QSTRING_CSTR(_filter) );
					//Debug(_log, "_data: [%s]", QSTRING_CSTR(data));

					QString mapKey = headers[key];

					SSDPService service;
					service.cacheControl = headers["CACHE-CONTROL"];
					service.location = QUrl (headers["LOCATION"]);
					service.server = headers["SERVER"];
					service.searchTarget = headers["ST"];
					service.uniqueServiceName = headers["USN"];

					headers.remove("CACHE-CONTROL");
					headers.remove("LOCATION");
					headers.remove("SERVER");
					headers.remove("ST");
					headers.remove("USN");

					service.otherHeaders = headers;

					if ( _skipDupKeys )
					{
						_services.insert(mapKey, service);
					}
					else
					{
						_services.insertMulti(mapKey, service);
					}
				}
			}
		}
	}
	_udpSocket->close();

	if ( _services.empty() )
	{
		Debug(_log,"Search target [%s], no record(s) matching [%s:%s]", QSTRING_CSTR(_searchTarget), QSTRING_CSTR(_filterHeader), QSTRING_CSTR(_filter) );
		rc = 0;
	}
	else
	{
		rc = _services.size();
		Debug(_log," [%d] service record(s) found", rc );
	}
	return rc;
}

QJsonArray SSDPDiscover::getServicesDiscoveredJson()
{
	QJsonArray result;

	QMap<QString, SSDPService>::iterator i;
	for (i = _services.begin(); i != _services.end(); ++i)
	{
		//Debug(_log, "Device discovered at [%s]", QSTRING_CSTR( i.key() ));

		QJsonObject obj;

		obj.insert("id", i.key());

		obj.insert("cache-control", i.value().cacheControl);
		obj.insert("location", i.value().location.toString());
		obj.insert("server", i.value().server);
		obj.insert("st", i.value().searchTarget);
		obj.insert("usn", i.value().uniqueServiceName);

		QUrl url (i.value().location);
		obj.insert("ip", url.host());
		obj.insert("port", url.port());

		QHostInfo hostInfo = QHostInfo::fromName(url.host());
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

		QJsonObject objOther;
		QMap <QString,QString>::iterator o;
		for (o = i.value().otherHeaders.begin(); o != i.value().otherHeaders.end(); ++o)
		{
			objOther.insert(o.key().toLower(), o.value());
		}
		obj.insert("other", objOther);

		result  << obj;
	}

	//Debug(_log, "result: [%s]", QString(QJsonDocument(result).toJson(QJsonDocument::Compact)).toUtf8().constData() );
	return result;
}

void SSDPDiscover::sendSearch(const QString& st)
{
	const QString msg = QString(UPNP_DISCOVER_MESSAGE).arg(_ssdpAddr.toString()).arg(_ssdpPort).arg(_ssdpMaxWaitResponseTime).arg(st);

	//Debug(_log,"Search request: [%s]", QSTRING_CSTR(msg));
	_udpSocket->writeDatagram(msg.toUtf8(), _ssdpAddr, _ssdpPort);
}

bool SSDPDiscover::setSearchFilter ( const QString& filter, const QString& filterHeader)
{
	bool rc = true;
	QRegularExpression regEx( filter );
	if (!regEx.isValid()) {
		QString errorString = regEx.errorString();
		int errorOffset = regEx.patternErrorOffset();

		Error(_log,"Filtering regular expression [%s] error [%d]:[%s]",  QSTRING_CSTR(filter), errorOffset, QSTRING_CSTR(errorString) );
		rc = false;
	}
	else
	{
		_filter = filter;
		_filterHeader=filterHeader.toUpper();
		_regExFilter = regEx;
	}
	return rc;
}
