// qt includes
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
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
	QByteArray _ba;
	QNetworkRequest _req;
	QNetworkAccessManager _mgr;

	bool trigger(bool set = false);

private slots:
	void sendHTTP();
	void sendHTTPp();
	void resolveReply(QNetworkReply *reply);

};
