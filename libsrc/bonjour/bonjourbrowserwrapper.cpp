#include <bonjour/bonjourbrowserwrapper.h>

//qt incl
#include <QTimer>

// bonjour
#include <bonjour/bonjourservicebrowser.h>
#include <bonjour/bonjourserviceresolver.h>

BonjourBrowserWrapper* BonjourBrowserWrapper::instance = nullptr;

BonjourBrowserWrapper::BonjourBrowserWrapper(QObject * parent)
	: QObject(parent)
	, _bonjourResolver(new BonjourServiceResolver(this))
	, _timerBonjourResolver(new QTimer(this))
{
	// register meta
	qRegisterMetaType<QMap<QString,BonjourRecord>>("QMap<QString,BonjourRecord>");

	BonjourBrowserWrapper::instance = this;
	connect(_bonjourResolver, &BonjourServiceResolver::bonjourRecordResolved, this, &BonjourBrowserWrapper::bonjourRecordResolved);

	connect(_timerBonjourResolver, &QTimer::timeout, this, &BonjourBrowserWrapper::bonjourResolve);
	_timerBonjourResolver->setInterval(1000);
	_timerBonjourResolver->start();

	// browse for _hyperiond-http._tcp
	browseForServiceType(QLatin1String("_hyperiond-http._tcp"));
}

bool BonjourBrowserWrapper::browseForServiceType(const QString &serviceType)
{
	if(!_browsedServices.contains(serviceType))
	{
		BonjourServiceBrowser* newBrowser = new BonjourServiceBrowser(this);
		connect(newBrowser, &BonjourServiceBrowser::currentBonjourRecordsChanged, this, &BonjourBrowserWrapper::currentBonjourRecordsChanged);
		newBrowser->browseForServiceType(serviceType);
		_browsedServices.insert(serviceType, newBrowser);
		return true;
	}
	return false;
}

void BonjourBrowserWrapper::currentBonjourRecordsChanged(const QList<BonjourRecord> &list)
{
	_hyperionSessions.clear();
	for ( auto rec : list )
	{
		_hyperionSessions.insert(rec.serviceName, rec);
	}
}

void BonjourBrowserWrapper::bonjourRecordResolved(const QHostInfo &hostInfo, int port)
{
	if ( _hyperionSessions.contains(_bonjourCurrentServiceToResolve))
	{
		QString host   = hostInfo.hostName();
		QString domain = _hyperionSessions[_bonjourCurrentServiceToResolve].replyDomain;
		if (host.endsWith("."+domain))
		{
			host.remove(host.length()-domain.length()-1,domain.length()+1);
		}
		_hyperionSessions[_bonjourCurrentServiceToResolve].hostName = host;
		_hyperionSessions[_bonjourCurrentServiceToResolve].port     = port;
		_hyperionSessions[_bonjourCurrentServiceToResolve].address  = hostInfo.addresses().isEmpty() ? "" : hostInfo.addresses().first().toString();
		//Debug(_log, "found hyperion session: %s:%d",QSTRING_CSTR(hostInfo.hostName()), port);

		//emit change
		emit browserChange(_hyperionSessions);
	}
}

void BonjourBrowserWrapper::bonjourResolve()
{
	for(auto key : _hyperionSessions.keys())
	{
		if (_hyperionSessions[key].port < 0)
		{
			_bonjourCurrentServiceToResolve = key;
			_bonjourResolver->resolveBonjourRecord(_hyperionSessions[key]);
			break;
		}
	}
}
