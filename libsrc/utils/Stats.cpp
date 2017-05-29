#include <utils/Stats.h>
#include <utils/SysInfo.h>
#include <HyperionConfig.h>
#include <leddevice/LedDevice.h>
#include <hyperion/Hyperion.h>

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
{
	// generate hash
	QString hash;
	foreach(QNetworkInterface interface, QNetworkInterface::allInterfaces())
	{
		if (!(interface.flags() & QNetworkInterface::IsLoopBack))     
		{
			_hash = QString(QCryptographicHash::hash(interface.hardwareAddress().toLocal8Bit(),QCryptographicHash::Sha1).toHex());
			break;
		}
    }
    
	// stop reporting if not found
	if(_hash.isEmpty())
	{
		Warning(_log, "No interface found, abort");
		return;
	}
    
	connect(&_mgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(resolveReply(QNetworkReply*)));
	
	// 7 days interval
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(sendHTTP()));
	timer->start(604800000);

	// check instant execution required
	if(trigger())
	{
   		QTimer::singleShot(0, this, [=]() {
    		sendHTTP();
		});
	}
}

Stats::~Stats()
{

}

void Stats::sendHTTP(bool put)
{
	QJsonObject config = Hyperion::getInstance()->getConfig();
	SysInfo::HyperionSysInfo data = SysInfo::get();

	QJsonObject system;
	system["kType"    ] = data.kernelType;
	//system["kernelVersion" ] = data.kernelVersion;
	system["arch"       ] = data.architecture;
	system["pType"      ] = data.productType;
	system["pVersion"   ] = data.productVersion;
	system["pName"      ] = data.prettyName;
	system["version"    ] = QString(HYPERION_VERSION);
	system["device"     ] = LedDevice::activeDevice();
	system["id"         ] = _hash;
	system["ledCount"   ] = QString::number(Hyperion::getInstance()->getLedCount());
	system["comp_sm"    ] = config["smoothing"].toObject().take("enable");
	system["comp_bb"    ] = config["blackborderdetector"].toObject().take("enable");
	system["comp_fw"    ] = config["forwarder"].toObject().take("enable");
	system["comp_udpl"  ] = config["udpListener"].toObject().take("enable");
	system["comp_bobl"  ] = config["boblightServer"].toObject().take("enable");
	system["comp_pc"    ] = config["framegrabber"].toObject().take("enable");
	system["comp_uc"    ] = config["grabberV4L2"].toArray().at(0).toObject().take("enable");

	QJsonDocument doc(system);
	QByteArray ba = doc.toJson();
    
	QNetworkRequest req;
   	req.setRawHeader("Content-Type", "application/json");
   	req.setRawHeader("Authorization", "Basic SHlwZXJpb25YbDQ5MlZrcXA6ZDQxZDhjZDk4ZjAwYjIw");
    
	if(put)
	{
		req.setUrl(QUrl("https://api.hyperion-project.org/api/stats/"+_hash));
		_mgr.put(req,ba);
	}
	else
	{
		req.setUrl(QUrl("https://api.hyperion-project.org/api/stats"));
		_mgr.post(req,ba);
	}
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
			//bool put = true;
   			QTimer::singleShot(0, this, [=]() {
     			sendHTTP(true);
			});
		}
	}
}

bool Stats::trigger(bool set)
{
	QString path =	QDir::homePath()+"/.hyperion/misc/";
	QDir dir;
	QFile file(path + "ts");

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
