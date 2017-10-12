#include <utils/Stats.h>
#include <utils/SysInfo.h>
#include <HyperionConfig.h>
#include <leddevice/LedDevice.h>

// qt includes
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>

Stats::Stats()
	: QObject()
	, _log(Logger::getInstance("STATS"))
	, _hyperion(Hyperion::getInstance())
{
	// generate hash
	foreach(QNetworkInterface interface, QNetworkInterface::allInterfaces())
	{
		if (!(interface.flags() & QNetworkInterface::IsLoopBack))
		{
			_hyperion->id = QString(QCryptographicHash::hash(interface.hardwareAddress().toLocal8Bit().append(_hyperion->getConfigFileName().toLocal8Bit()),QCryptographicHash::Sha1).toHex());
			_hash = QString(QCryptographicHash::hash(interface.hardwareAddress().toLocal8Bit(),QCryptographicHash::Sha1).toHex());
			break;
		}
    }

	// stop reporting if not found
	if(_hash.isEmpty())
	{
		Warning(_log, "No interface found, abort");
		// fallback id
		_hyperion->id = QString(QCryptographicHash::hash(_hyperion->getConfigFileName().toLocal8Bit(),QCryptographicHash::Sha1).toHex());
		return;
	}

	// prepare content
	QJsonObject config = _hyperion->getConfig();
	SysInfo::HyperionSysInfo data = SysInfo::get();

	QJsonObject system;
	system["kType"    ] = data.kernelType;
	system["arch"       ] = data.architecture;
	system["pType"      ] = data.productType;
	system["pVersion"   ] = data.productVersion;
	system["pName"      ] = data.prettyName;
	system["version"    ] = QString(HYPERION_VERSION);
	system["device"     ] = LedDevice::activeDevice();
	system["id"         ] = _hyperion->id;
	system["hw_id"      ] = _hash;
	system["ledCount"   ] = QString::number(Hyperion::getInstance()->getLedCount());
	system["comp_sm"    ] = config["smoothing"].toObject().take("enable");
	system["comp_bb"    ] = config["blackborderdetector"].toObject().take("enable");
	system["comp_fw"    ] = config["forwarder"].toObject().take("enable");
	system["comp_udpl"  ] = config["udpListener"].toObject().take("enable");
	system["comp_bobl"  ] = config["boblightServer"].toObject().take("enable");
	system["comp_pc"    ] = config["framegrabber"].toObject().take("enable");
	system["comp_uc"    ] = config["grabberV4L2"].toArray().at(0).toObject().take("enable");

	QJsonDocument doc(system);
	_ba = doc.toJson();

	// QNetworkRequest Header
	_req.setRawHeader("Content-Type", "application/json");
   	_req.setRawHeader("Authorization", "Basic SHlwZXJpb25YbDQ5MlZrcXA6ZDQxZDhjZDk4ZjAwYjIw");

	connect(&_mgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(resolveReply(QNetworkReply*)));

	// 7 days interval
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(sendHTTP()));
	timer->start(604800000);

	//delay initial check
	QTimer::singleShot(60000, this, SLOT(initialExec()));
}

Stats::~Stats()
{

}

void Stats::initialExec()
{
	if(trigger())
	{
		QTimer::singleShot(0,this, SLOT(sendHTTP()));
	}
}

void Stats::sendHTTP()
{
	_req.setUrl(QUrl("https://api.hyperion-project.org/api/stats"));
	_mgr.post(_req,_ba);
}

void Stats::sendHTTPp()
{
	_req.setUrl(QUrl("https://api.hyperion-project.org/api/stats/"+_hyperion->id));
	_mgr.put(_req,_ba);
}

void Stats::resolveReply(QNetworkReply *reply)
{
	if (reply->error() == QNetworkReply::NoError)
	{
		// update file timestamp
		trigger(true);
		// already created, update entry
		if(reply->readAll().startsWith("null"))
		{
			QTimer::singleShot(0, this, SLOT(sendHTTPp()));
		}
	}
}

bool Stats::trigger(bool set)
{
	QString path =	_hyperion->getRootPath()+"/misc/";
	QDir dir;
	QFile file(path + _hyperion->id);

	if(set && file.open(QIODevice::ReadWrite) )
	{
		QTextStream stream( &file );
		stream << "DO NOT DELETE" << endl;
		file.close();
	}
	else
	{
		if(!dir.exists(path))
		{
			dir.mkpath(path);
		}
		if (!file.exists())
	  	{
			if(file.open(QIODevice::ReadWrite))
			{
				file.close();
				return true;
			}
			return true;
		}

		QFileInfo info(file);
		QDateTime newDate = QDateTime::currentDateTime();
		QDateTime oldDate = info.lastModified();
		int diff = oldDate.daysTo(newDate);
		return diff >= 7 ? true : false;
	}
	return false;
}
