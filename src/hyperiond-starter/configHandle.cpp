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

	for (auto cfg : AllConfigs)
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

//Snippet from : https://forum.qt.io/post/393109
void configHandle::modifyJsonValue(QJsonValue& destValue, const QString& path, const QJsonValue& newValue)
{
	const int indexOfDot = path.indexOf('.');
	const QString dotPropertyName = path.left(indexOfDot);
	const QString dotSubPath = indexOfDot > 0 ? path.mid(indexOfDot + 1) : QString();

	const int indexOfSquareBracketOpen = path.indexOf('[');
	const int indexOfSquareBracketClose = path.indexOf(']');

	const int arrayIndex = path.mid(indexOfSquareBracketOpen + 1, indexOfSquareBracketClose - indexOfSquareBracketOpen - 1).toInt();

	const QString squareBracketPropertyName = path.left(indexOfSquareBracketOpen);
	const QString squareBracketSubPath = indexOfSquareBracketClose > 0 ? (path.mid(indexOfSquareBracketClose + 1)[0] == '.' ? path.mid(indexOfSquareBracketClose + 2) : path.mid(indexOfSquareBracketClose + 1)) : QString();

    // determine what is first in path. dot or bracket
	bool useDot = (indexOfDot >= 0 && indexOfSquareBracketOpen >= 0) ? (indexOfDot <= indexOfSquareBracketOpen) : (indexOfSquareBracketOpen < 0);

	QString usedPropertyName = useDot ? dotPropertyName : squareBracketPropertyName;
	QString usedSubPath = useDot ? dotSubPath : squareBracketSubPath;

	QJsonValue subValue;
	if (destValue.isArray())
	{
		subValue = destValue.toArray()[usedPropertyName.toInt()];
	}
	else if (destValue.isObject())
	{
		subValue = destValue.toObject()[usedPropertyName];
	}
	else
	{
		qDebug() << "oh, what should i do now with the following value?! " << destValue;
	}

	if(usedSubPath.isEmpty())
	{
		subValue = newValue;
	}
	else
	{
		if (subValue.isArray())
		{
			QJsonArray arr = subValue.toArray();
			QJsonValue arrEntry = arr[arrayIndex];
			modifyJsonValue(arrEntry,usedSubPath,newValue);
			arr[arrayIndex] = arrEntry;
			subValue = arr;
		}
		else if (subValue.isObject())
		{
			modifyJsonValue(subValue,usedSubPath,newValue);
		}
		else
		{
			subValue = newValue;
		}
	}

	if (destValue.isArray())
	{
		QJsonArray arr = destValue.toArray();
		arr[arrayIndex] = subValue;
		destValue = arr;
	}
	else if (destValue.isObject())
	{
		QJsonObject obj = destValue.toObject();
		obj[usedPropertyName] = subValue;
		destValue = obj;
	}
	else
	{
		destValue = newValue;
	}
}

void configHandle::modifyJsonValue(QJsonDocument& doc, const QString& path, const QJsonValue& newValue)
{
	QJsonValue val;
	val = doc.isArray() ? doc.array() : doc.object();

	modifyJsonValue(val,path,newValue);

	doc = QJsonDocument((val.isArray() ? val.toArray() : val.toObject() );
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

	modifyJsonValue(doc, "jsonServer.port", jsonPort);
	modifyJsonValue(doc, "protoServer.port", protoPort);
	modifyJsonValue(doc, "webConfig.port", webPort);
	modifyJsonValue(doc, "general.name", pName);

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
