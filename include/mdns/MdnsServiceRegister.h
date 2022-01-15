#ifndef MDNSSERVICEREGISTER_H
#define MDNSSERVICEREGISTER_H

#include <QByteArray>
#include <QMap>

struct mdnsConfig
{
	QByteArray serviceType;
	QString serviceNameFilter;
};

typedef QMap<QString, mdnsConfig> MdnsServiceMap;

const MdnsServiceMap mDnsServiceMap = {
	//Hyperion
	{"jsonapi"	    , {"_hyperiond-json._tcp.local.", ".*"}},
	{"flatbuffer"	, {"_hyperiond-flatbuf._tcp.local.", ".*"}},
	{"protobuffer"	, {"_hyperiond-protobuf._tcp.local.", ".*"}},
	{"http"	        , {"_http._tcp.local.", ".*"}},
	{"https"	    , {"_https._tcp.local.", ".*"}},

	//LED Devices
	{"cololight"    , {"_hap._tcp.local.", "ColoLight.*"}},
	{"nanoleaf"	    , {"_nanoleafapi._tcp.local.", ".*"}},
	{"philipshue"   , {"_hue._tcp.local.", ".*"}},
	{"wled"		    , {"_wled._tcp.local.", ".*"}},
	{"yeelight"	    , {"_hap._tcp.local.", "Yeelight.*|YLBulb.*"}},
};

class MdnsServiceRegister {
public:
	static QByteArray getServiceType(const QString &serviceType) { return mDnsServiceMap[serviceType].serviceType; }
	static QString getServiceNameFilter(const QString &serviceType) { return mDnsServiceMap[serviceType].serviceNameFilter; }
	static MdnsServiceMap getAllConfigs () { return mDnsServiceMap; }
};

#endif // MDNSSERVICEREGISTER_H
