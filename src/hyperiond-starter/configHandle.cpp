#include "configHandle.h"

configHandle::configHandle(QString configPath, QString daemonPath)
	: QObject()
	, _configPath(configPath)
	, _process(configPath,daemonPath)
{
	// check if config dir exists
	QDir dir(configPath);
	dir.mkpath(configPath);

	// create main config
	QString configName("hyperion_main.json");
	QString prettyName("My Main Configuration");
	if(!configExists(configName))
	{
		createConfig(configName, prettyName);
	}

	// create main process instance & start
	_process.createProcess(configName);
	_process.startProcessByCName(configName);

	// listen for hyperion_main.json edits
	if(_fWatch.addPath(configPath+configName))
	{
		connect(&_fWatch, &QFileSystemWatcher::fileChanged, this, &configHandle::fileChanged);
	}
	else
	{
		qDebug() << "File Observer failed! Instance changes won't be applied until next start";
	}

	// read ports from all configs in config dir
	readAllPorts();

	// initial process handling
	processConfigChange(_configPath+configName);
}

configHandle::~configHandle()
{

}

QString configHandle::readFile(QString cfile)
{
	QFile file(cfile);
	file.open(QFile::ReadOnly);

	QTextStream in(&file);
	QString config = in.readAll();
	file.close();

	return config;
}

void configHandle::readAllPorts(void)
{
	QDir dir(_configPath);
	QStringList filters("*.json");
	dir.setNameFilters(filters);
	QStringList allConfigs = dir.entryList(QDir::Files);

	for (auto cfg : allConfigs)
	{
		QString config = readFile(_configPath+cfg.toLocal8Bit().constData());
		QJsonDocument doc = QJsonDocument::fromJson(config.toUtf8());
		QJsonObject obj = doc.object();
		quint16 jsonPort = obj["jsonServer"].toObject().take("port").toInt();
		quint16 protoPort = obj["protoServer"].toObject().take("port").toInt();
		quint16 webPort = obj["webConfig"].toObject().take("port").toInt();

		_usedPorts << jsonPort << protoPort << webPort;
	}
}

void configHandle::processConfigChange(QString configPath)
{
	QString config = readFile(configPath);
	qDebug() << "processConfigChange" << configPath;
	QJsonDocument doc = QJsonDocument::fromJson(config.toUtf8());
	QJsonArray arr = doc.object()["instances"].toArray();

	for(int i = 0; i<arr.size(); ++i)
	{
		QString pName = arr[i].toObject().take("instName").toString();
		QString cName = convertToCName(pName);

		if(!configExists(cName))
		{
			if(createConfig(cName,pName))
			{
				_process.createProcess(cName);
				if(arr[i].toObject().take("enabled").toBool())
					_process.startProcessByCName(cName);
			}
		}
		else
		{
			_process.createProcess(cName);
			if(arr[i].toObject().take("enable").toBool())
			{
				_process.startProcessByCName(cName);
			}
			else
			{
				_process.stopProcessByCName(cName);
			}
		}
	}
}

QString configHandle::convertToCName(QString pName)
{
	pName = pName.trimmed();
	while(pName.startsWith("."))
		pName.remove(0,1);

	if(!pName.endsWith(".json"))
		pName.append(".json");

	return pName;
}

void configHandle::fileChanged(const QString path)
{
	processConfigChange(path);
}

quint16 configHandle::checkPort(quint16 port, bool incOne)
{
	QTcpServer server;
	while (_usedPorts.indexOf(port) != -1 || !server.listen(QHostAddress::Any, port))
	{
		port += incOne ? 1 : 2;
	}
	server.close();
	return port;
}

bool configHandle::configExists(QString configName)
{
	QFile tryFile(_configPath+configName);
	return tryFile.exists();
}

bool configHandle::createConfig(QString configName, QString pName)
{
	// init default config
	Q_INIT_RESOURCE(resource);

	// get default config for edits
	QString config = readFile(":/hyperion_default.config");

	QJsonDocument doc = QJsonDocument::fromJson(config.toUtf8());
	QJsonObject obj = doc.object();
	quint16 jsonPort = obj["jsonServer"].toObject().take("port").toInt();
	quint16 protoPort = obj["protoServer"].toObject().take("port").toInt();
	quint16 webPort = obj["webConfig"].toObject().take("port").toInt();

	jsonPort = checkPort(jsonPort);
	protoPort = checkPort(protoPort);
	webPort = checkPort(webPort, true);

	_usedPorts << jsonPort << protoPort << webPort;

	QJsonUtils::modify(obj, QStringList() << "jsonServer" << "port", jsonPort);
	QJsonUtils::modify(obj, QStringList() << "protoServer" << "port", protoPort);
	QJsonUtils::modify(obj, QStringList() << "webConfig" << "port", webPort);
	QJsonUtils::modify(obj, QStringList() << "general" << "name", pName);

	doc.setObject(obj);

	// write the final doc
	QFile tFile(_configPath+configName);
	if(tFile.open(QFile::WriteOnly))
	{
		tFile.write(doc.toJson());
		tFile.close();
		qDebug() << "config creation success:" << configName;
		return true;
	}
	else
	{
		qDebug() << "config creation FAILED:" << configName;
		return false;
	}
}
