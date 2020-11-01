#pragma once
// qt incl
#include <QObject>
#include <QMap>
#include <QHostInfo>

#include <bonjour/bonjourrecord.h>

class BonjourServiceBrowser;
class BonjourServiceResolver;
class QTimer;

class BonjourBrowserWrapper : public QObject
{
	Q_OBJECT
private:
	friend class HyperionDaemon;
	///
	/// @brief Browse for hyperion services in bonjour, constructed from HyperionDaemon
	///        Searching for hyperion http service by default
	///
	BonjourBrowserWrapper(QObject * parent = nullptr);

public:

	///
	/// @brief Browse for a service
	///
	bool browseForServiceType(const QString &serviceType);
	///
	/// @brief Get all available sessions
	///
	QMap<QString, BonjourRecord> getAllServices() { return _hyperionSessions; }

	static BonjourBrowserWrapper* instance;
	static BonjourBrowserWrapper *getInstance()	{ return instance; }

signals:
	///
	/// @brief Emits whenever a change happend
	///
	void browserChange( const QMap<QString, BonjourRecord> &bRegisters );

private:
	/// map of service names and browsers
	QMap<QString, BonjourServiceBrowser *> _browsedServices;
	/// Resolver
	BonjourServiceResolver *_bonjourResolver;

	// contains all current active service sessions
	QMap<QString, BonjourRecord> _hyperionSessions;

	QString _bonjourCurrentServiceToResolve;
	/// timer to resolve changes
	QTimer *_timerBonjourResolver;

private slots:
	///
	/// @brief is called whenever a BonjourServiceBrowser emits change
	void currentBonjourRecordsChanged( const QList<BonjourRecord> &list );
	/// @brief new record resolved
	void bonjourRecordResolved( const QHostInfo &hostInfo, int port );

	///
	/// @brief timer slot which updates regularly entries
	///
	void bonjourResolve();
};
