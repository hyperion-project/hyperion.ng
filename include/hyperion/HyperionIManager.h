#pragma once

// util
#include <utils/Logger.h>
#include <utils/VideoMode.h>
#include <utils/settings.h>
#include <utils/Components.h>
#include <events/EventEnum.h>
#include <db/InstanceTable.h>

// qt
#include <QMap>
#include <QSharedPointer>
#include <QScopedPointer>

class Hyperion;
class InstanceTable;

enum class InstanceState{
	H_STARTING,
	H_STARTED,
	H_ON_STOP,
	H_STOPPED,
	H_CREATED,
	H_DELETED
};

///
/// @brief HyperionInstanceManager manages the instances of the the Hyperion class
///
class HyperionIManager : public QObject
{
	Q_OBJECT

public:
	struct PendingRequests
	{
		QObject *caller;
		int     tan;
	};

	// global instance pointer
	static HyperionIManager* getInstance();
	static QSharedPointer<HyperionIManager>& instanceRef();
	static QWeakPointer<HyperionIManager> getInstanceWeak() { return _instance.toWeakRef(); }
	static void createInstance(QObject* parent = nullptr);
	static void destroyInstance();
	static bool isValid();

	~HyperionIManager() override;

public slots:
	///
	/// @brief Is given instance running?
	/// @param instanceId  The instance to check
	/// @return  True when running else false
	///
	bool isInstanceRunning(quint8 instanceId) const { return _runningInstances.contains(instanceId); }

	///
	/// @brief Is given instance a configred one?
	/// @param instanceId  The instance to check
	/// @return  True when existing else false
	///
	bool doesInstanceExist(quint8 instanceId) const { return _instanceTable->doesInstanceExist(instanceId); }

	///
	/// @brief Get a Hyperion instance by index
	/// @param instanceId  the index
	/// @return Hyperion instance, if index is not found returns instance 0
	///
	QSharedPointer<Hyperion> getHyperionInstance(quint8 instance = 0);

	///
	/// @brief Get instance data of all instances in db + running state
	///
	QVector<QVariantMap> getInstanceData() const;

	QString getInstanceName(quint8 instanceId = 0);

	///
	/// @brief Get all instance indicies of running instances
	/// @return All running instance Ids, returns and empty set if no instance is runnning
	///
	QSet<quint8> getRunningInstanceIdx() const;

	///
	/// @brief Get the first running instance's ID
	/// @return First instance ID, eturns NO_INSTANCE_ID (255) if none is runnning
	///
	quint8 getFirstRunningInstanceIdx() const;

	///
	/// @brief Get the first running Hyperion instance
	/// @return Hyperion instance, if none is runnning returns a nullptr
	///
	QSharedPointer<Hyperion> getFirstRunningInstance();

	///
	/// @brief Get all instance indicies configured
	///
	QSet<quint8> getInstanceIds() const;

	///
	/// @brief Start a Hyperion instance
	/// @param instanceId   Instance index
	/// @param block        If true return when thread has been started
	/// @return Return true on success, false if not found in db
	///
	bool startInstance(quint8 instanceId, bool block = false, QObject *caller = nullptr, int tan = 0);

	///
	/// @brief Stop a Hyperion instance
	/// @param instanceId  Instance index
	/// @return Return true on success, false if not found in db
	///
	bool stopInstance(quint8 instanceId);

	///
	/// @brief Handle an Hyperion Event
	/// @param event Event to be processed
	///
	void handleEvent(Event event);

	///
	/// @brief Toggle the state of all Hyperion instances
	/// @param enable, If false all instances toggle to pause, else to resume
	///
	void toggleStateAllInstances(bool enable = false);

	///
	/// @brief Create a new Hyperion instance entry in db
	/// @param name  The friendly name of the instance
	/// @param start     If true it will be started after creation (async)
	/// @return Return true on success false if name is already in use or a db error occurred
	///
	bool createInstance(const QString& name, bool start = false);

	///
	/// @brief Delete Hyperion instance entry in db. Cleanup also all associated table data for this instance
	/// @param instanceId  The instance index
	/// @return Return true on success, false if not found or not allowed
	///
	bool deleteInstance(quint8 instanceId);

	///
	/// @brief Assign a new name to the given instance
	/// @param instanceId  The instance index
	/// @param name        The instance name index
	/// @return Return true on success, false if not found
	///
	bool saveName(quint8 instanceId, const QString& name);

signals:
	///
	/// @brief Emits whenever the state of a instance changes according to enum instanceState
	/// @param instaneState  A state from enum
	/// @param instanceId    The index of instance
	/// @param name          The name of the instance, just available with H_CREATED
	///
	void instanceStateChanged(InstanceState state, quint8 instanceId, const QString& name = QString());
	void started(InstanceState state, quint8 instanceId, const QString& name = QString());

	///
	/// @brief Emits whenever something changes, the lazy version of instanceStateChanged (- H_ON_STOP) + saveName() emit
	///
	void change();

	///
	/// @brief Emits when the user has requested to start a instance
	/// @param  caller  The origin caller instance who requested
	/// @param  tan     The tan that was part of the request
	///
	void startInstanceResponse(QObject *caller, const int &tan);

	///
	/// @brief Emits when all instances were stopped
	///
	void areAllInstancesStopped();

signals:
	///////////////////////////////////////
	/// FROM HYPERIONDAEMON TO HYPERION ///
	///////////////////////////////////////

	///
	/// @brief PIPE videoMode back to Hyperion
	///
	void newVideoMode(VideoMode mode);

	///////////////////////////////////////
	/// FROM HYPERION TO HYPERIONDAEMON ///
	///////////////////////////////////////

	///
	/// @brief PIPE settings events from Hyperion
	///
	void settingsChanged(settings::type type, const QJsonDocument& data);

	///
	/// @brief PIPE videoMode request changes from Hyperion to HyperionDaemon
	///
	void requestVideoMode(VideoMode mode);

	///
	/// @brief PIPE component state changes from Hyperion to HyperionDaemon
	///
	void compStateChangeRequest(hyperion::Components component, bool enable);

private slots:
	///
	/// @brief handle started signal of Hyperion instances
	///
	void handleStarted();

	///
	/// @brief handle finished signal of Hyperion instances
	/// @param name  The instance's name for information
	///
	void handleFinished(const QString& name = "");

	///
	/// @brief Toggle the state of all Hyperion instances for a suspend sceanrio (user is not interacting with the system)
	/// @param isSuspend, If true all instances toggle to suspend, else to resume
	///
	void toggleSuspend(bool isSuspend);

	///
	/// @brief Toggle the state of all Hyperion instances for an idle sceanrio
	/// @param isIdle, If true all instances toggle to idle, else to resume
	///
	void toggleIdle(bool isIdle);

private:
	friend class HyperionDaemon;
	///
	/// @brief Construct the Manager
	///
	HyperionIManager(QObject* parent = nullptr);
	// Singleton storage
	static QSharedPointer<HyperionIManager> _instance;

	///
	/// @brief Start all instances that are marked as enabled in db. Non blocking
	///
	void startAll();

	///
	/// @brief Stop all instances, used from hyperiond
	///
	void stopAll();

private:
	QSharedPointer<Logger> _log;
	QScopedPointer<InstanceTable> _instanceTable;
	QMap<quint8, QSharedPointer<Hyperion>> _runningInstances;
	QMap<quint8, QSharedPointer<Hyperion>> _startingInstances;

	/// All pending requests
	QMap<quint8, PendingRequests> _pendingRequests;
};
