// qt includes
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>

// hyperion includes
#include <utils/Logger.h>

class Stats : public QObject
{
	Q_OBJECT

public:
	Stats();	
	~Stats();

private:
	Logger* _log;
	QString _hash = "";
	QNetworkAccessManager _mgr;

	bool trigger(bool set = false);

private slots:
	void sendHTTP(bool put = false);
	void resolveReply(QNetworkReply *reply);


};
