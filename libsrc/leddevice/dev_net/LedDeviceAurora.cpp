
// Local-Hyperion includes
#include "LedDeviceAurora.h"
#include <netdb.h>
#include <assert.h>
// qt includes
#include <QtCore/qmath.h>
#include <QEventLoop>
#include <QNetworkReply>

#define ll ss

struct addrinfo vints, *serverinfo, *pt;
//char udpbuffer[1024];
int sockfp;
int update_num;
LedDevice* LedDeviceAurora::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceAurora(deviceConfig);
}

LedDeviceAurora::LedDeviceAurora(const QJsonObject &deviceConfig) {
	init(deviceConfig);
}

bool LedDeviceAurora::init(const QJsonObject &deviceConfig) {
	const QString hostname = deviceConfig["output"].toString();
	const QString key  = deviceConfig["key"].toString();

	manager = new QNetworkAccessManager();
	QString port;
	// Read Panel count and panel Ids
	QByteArray response = get(hostname, key, "panelLayout/layout");
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(response, &error);
	if (error.error != QJsonParseError::NoError)
	{
		throw std::runtime_error("No Layout found. Check hostname and auth key");
	}
	//Debug
	QString strJson(doc.toJson(QJsonDocument::Compact));
	std::cout << strJson.toUtf8().constData() << std::endl;
	
	QJsonObject json = doc.object();

	panelCount = json["numPanels"].toInt();
	std::cout << panelCount << std::endl;
	QJsonObject positionDataJson = doc.object()["positionData"].toObject();
	QJsonArray positionData = json["positionData"].toArray();
	// Loop over all children.
	foreach (const QJsonValue & value, positionData) {
		QJsonObject panelObj = value.toObject();	
		int panelId = panelObj["panelId"].toInt();
		panelIds.push_back(panelId);
	}
	
	// Check if we found enough lights.
	if (panelIds.size() != panelCount) {
		throw std::runtime_error("Not enough lights found");
	}else {
		std::cout <<  "All panel Ids found: "<< panelIds.size() << std::endl;
	}
	
	// Set Aurora to UDP Mode
	QByteArray modeResponse = changeMode(hostname, key, "effects");
	QJsonDocument configDoc = QJsonDocument::fromJson(modeResponse, &error);

	//Debug
	//QString strConf(configDoc.toJson(QJsonDocument::Compact));
	//std::cout << strConf.toUtf8().constData() << std::endl;

	if (error.error != QJsonParseError::NoError)
	{
		throw std::runtime_error("Could not change mode");
	}

	// Get UDP port
	port = QString::number(configDoc.object()["streamControlPort"].toInt());

	std::cout << "hostname " << hostname.toStdString() << " port " << port.toStdString() << std::endl;
	
	int rv;

	memset(&vints, 0, sizeof vints);
	vints.ai_family = AF_UNSPEC;
	vints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(hostname.toUtf8().constData() , port.toUtf8().constData(), &vints, &serverinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		assert(rv==0);
	}
	
	// loop through all the results and make a socket
	for(pt = serverinfo; pt != NULL; pt = pt->ai_next) {
		if ((sockfp = socket(pt->ai_family, pt->ai_socktype,
				pt->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (pt == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		assert(pt!=NULL);
	}
	std::cout << "Started successfully ";
	return true;
}

QString LedDeviceAurora::getUrl(QString host, QString token, QString route) {
	return QString("http://%1:16021/api/v1/%2/%3").arg(host).arg(token).arg(route);
}

QByteArray LedDeviceAurora::get(QString host, QString token, QString route) {
	QString url = getUrl(host, token, route);
	// Perfrom request
	QNetworkRequest request(url);
	QNetworkReply* reply = manager->get(request);
	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	loop.connect(reply, SIGNAL(finished()), SLOT(quit()));
	// Go into the loop until the request is finished.
	loop.exec();
	// Read all data of the response.
	QByteArray response = reply->readAll();
	// Free space.
	reply->deleteLater();
	// Return response
	return response;
}

QByteArray LedDeviceAurora::putJson(QString url, QString json) { 
	// Perfrom request
	QNetworkRequest request(url);
	QNetworkReply* reply = manager->put(request, json.toUtf8());
	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	loop.connect(reply, SIGNAL(finished()), SLOT(quit()));
	// Go into the loop until the request is finished.
	loop.exec();
	// Read all data of the response.
	QByteArray response = reply->readAll();
	// Free space.
	reply->deleteLater();
	// Return response
	return response;
}

QByteArray LedDeviceAurora::changeMode(QString host, QString token, QString route) {
	QString url = getUrl(host, token, route);
	QString jsondata( "{\"write\" : {\"command\" : \"display\", \"animType\" : \"extControl\"}}"); //Enable UDP Mode
	return putJson(url, jsondata);
}

LedDeviceAurora::~LedDeviceAurora()
{
	delete manager;
}

int LedDeviceAurora::write(const std::vector<ColorRgb> & ledValues)
{
    uint udpBufferSize = panelCount * 7 + 1;
	char udpbuffer[udpBufferSize];
	update_num++;
	update_num &= 0xf;

	int i=0;
	int panelCounter = 0;
	udpbuffer[i++] = panelCount;
	for (const ColorRgb& color : ledValues)
	{
		if (i<udpBufferSize) {
			udpbuffer[i++] = panelIds[panelCounter++ % panelCount];
			udpbuffer[i++] = 1; // No of Frames
			udpbuffer[i++] = color.red;
			udpbuffer[i++] = color.green;
			udpbuffer[i++] = color.blue;
			udpbuffer[i++] = 0; // W not set manually
			udpbuffer[i++] = 1; // currently fixed at value 1 which corresponds to 100ms
		}
		if(panelCounter > panelCount) {
			break;
		}
		//printf ("c.red %d sz c.red %d\n", color.red, sizeof(color.red));
	}
	sendto(sockfp, udpbuffer, i, 0, pt->ai_addr, pt->ai_addrlen);

	return 0;
}

int LedDeviceAurora::switchOff()
{
	return 0;
}
