
#pragma once

// system includes
#include <cstdint>
#include <string>

// QT includes
#include <QTimer>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

// Hyperion includes
#include <hyperion/Hyperion.h>

class XBMCVideoChecker : public QObject
{
Q_OBJECT

public:
	XBMCVideoChecker(QString address, uint64_t interval, Hyperion * hyperion, int priority);

	void start();

private slots:
	void sendRequest();

	void receiveReply(QNetworkReply * reply);

private:
	QTimer _timer;

	QNetworkAccessManager _networkManager;

	QNetworkRequest _request;

	Hyperion * _hyperion;

	int _priority;
};

