#ifndef LEDDEVICEMDNSRREGISTER_H
#define LEDDEVICEMDNSRREGISTER_H

#include <QByteArray>
#include <QMap>


struct mdnsConfig
{
	QByteArray serviceType;
	QString serviceNameFilter;
};

typedef QMap<QString, mdnsConfig> MdnsConfigMap;

const MdnsConfigMap ledDeviceMdnsConfigMap = {
	{"cololight"  , {"_hap._tcp.local.", "ColoLight.*"}},
	{"nanoleaf"	  , {"_nanoleafapi._tcp.local.", ".*"}},
	{"philipshue" , {"_hue._tcp.local.", ".*"}},
	{"wled"		  , {"_wled._tcp.local.", ".*"}},
	{"yeelight"	  , {"_hap._tcp.local.", "Yeelight.*|YLBulb.*"}},
};

class LedDeviceMdnsRegister {
public:
	static QByteArray getServiceType(const QString &deviceType) { return ledDeviceMdnsConfigMap[deviceType].serviceType; }
	static QString getServiceNameFilter(const QString &deviceType) { return ledDeviceMdnsConfigMap[deviceType].serviceNameFilter; }
	static MdnsConfigMap getAllConfigs () { return ledDeviceMdnsConfigMap; }
};

#endif // LEDDEVICEMDNSRREGISTER_H
