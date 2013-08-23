// Qt includes
#include <QUrl>

#include <xbmcvideochecker/XBMCVideoChecker.h>

XBMCVideoChecker::XBMCVideoChecker(QString address, uint64_t interval_ms, Hyperion * hyperion, int priority) :
	QObject(),
	_timer(),
	_networkManager(),
	_request(QUrl(QString("http://%1/jsonrpc?request={\"jsonrpc\":\"2.0\",\"method\":\"Player.GetActivePlayers\",\"id\":1}").arg(address))),
	_hyperion(hyperion),
	_priority(priority)
{
	// setup timer
	_timer.setSingleShot(false);
	_timer.setInterval(interval_ms);
	connect(&_timer, SIGNAL(timeout()), this, SLOT(sendRequest()));

	//setup network manager
	connect(&_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(receiveReply(QNetworkReply*)));
}

void XBMCVideoChecker::start()
{
	_timer.start();
}

void XBMCVideoChecker::sendRequest()
{
	_networkManager.get(_request);
}

void XBMCVideoChecker::receiveReply(QNetworkReply * reply)
{
	if (reply->error() != QNetworkReply::NoError)
	{
		std::cerr << "Error while requesting XBMC video info: " << reply->errorString().toStdString() << std::endl;
	}
	else
	{
		const QString jsonReply(reply->readAll());
		if (jsonReply.contains("playerid"))
		{
			// something is playing. check for "video" to check if a video is playing
			// clear our priority channel to allow the grabbed vido colors to be shown
			_hyperion->clear(_priority);
		}
		else
		{
			// Nothing is playing. set our priority channel completely to black
			// The timeout is used to have the channel cleared after 30 seconds of connection problems...
			_hyperion->setColor(_priority, RgbColor::BLACK, 30000);
		}
	}

	// close reply
	reply->close();
}

