// qt includes
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>

// hyperion includes
#include <utils/Logger.h>
#include <hyperion/Hyperion.h>

class Stats : public QObject
{
	Q_OBJECT

public:
	Stats();
	static Stats* getInstance() { return instance; };
	static Stats* instance;

	void handleDataUpdate(const QJsonObject& config);

private:
	friend class HyperionDaemon;
	Stats(const QJsonObject& config);
	~Stats();

private:
	Logger* _log;
	Hyperion* _hyperion;
	QString _hash = "";
	QByteArray _ba;
	QNetworkRequest _req;
	QNetworkAccessManager _mgr;

	bool trigger(bool set = false);

private slots:
	void initialExec();
	void sendHTTP();
	void sendHTTPp();
	void resolveReply(QNetworkReply *reply);

};
