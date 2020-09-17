#pragma once

// util
#include <utils/Logger.h>
#include <utils/VideoMode.h>
#include <utils/settings.h>
#include <utils/Components.h>

// qt
#include <QMap>

class Hyperion;
class InstanceTable;

enum class InstanceState{
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
	// global instance pointer
	static HyperionIManager* getInstance() { return HIMinstance; }
	static HyperionIManager* HIMinstance;

public slots:
	///
	/// @brief Is given instance running?
	/// @param inst  The instance to check
	/// @return  True when running else false
	///
	bool IsInstanceRunning(quint8 inst) const { return _runningInstances.contains(inst); }

	///
	/// @brief Get a Hyperion instance by index
	/// @param intance  the index
	/// @return Hyperion instance, if index is not found returns instance 0
	///
	Hyperion* getHyperionInstance(quint8 instance = 0);

	///
	/// @brief Get instance data of all instaces in db + running state
	///
	QVector<QVariantMap> getInstanceData() const;

	///
	/// @brief Start a Hyperion instance
	/// @param instance  Instance index
	/// @param block     If true return when thread has been started
	/// @return Return true on success, false if not found in db
	///
	bool startInstance(quint8 inst, bool block = false);

	///
	/// @brief Stop a Hyperion instance
	/// @param instance  Instance index
	/// @return Return true on success, false if not found in db
	///
	bool stopInstance(quint8 inst);

	///
	/// @brief Toggle the state of all Hyperion instances
	/// @param pause If true all instances toggle to pause, else to resume
	///
	void toggleStateAllInstances(bool pause = false);

	///
	/// @brief Create a new Hyperion instance entry in db
	/// @param name  The friendly name of the instance
	/// @param start     If true it will be started after creation (async)
	/// @return Return true on success false if name is already in use or a db error occurred
	///
	bool createInstance(const QString& name, bool start = false);

	///
	/// @brief Delete Hyperion instance entry in db. Cleanup also all associated table data for this instance
	/// @param inst  The instance index
	/// @return Return true on success, false if not found or not allowed
	///
	bool deleteInstance(quint8 inst);

	///
	/// @brief Assign a new name to the given instance
	/// @param inst  The instance index
	/// @param name  The instance name index
	/// @return Return true on success, false if not found
	///
	bool saveName(quint8 inst, const QString& name);

signals:
	///
	/// @brief Emits whenever the state of a instance changes according to enum instanceState
	/// @param instaneState  A state from enum
	/// @param instance      The index of instance
	/// @param name          The name of the instance, just available with H_CREATED
	///
	void instanceStateChanged(InstanceState state, quint8 instance, const QString& name = QString());

	///
	/// @brief Emits whenever something changes, the lazy version of instanceStateChanged (- H_ON_STOP) + saveName() emit
	///
	void change();

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
	///
	void handleFinished();

private:
	friend class HyperionDaemon;
	///
	/// @brief Construct the Manager
	/// @param The root path of all userdata
	///
	HyperionIManager(const QString& rootPath, QObject* parent = nullptr);

	///
	/// @brief Start all instances that are marked as enabled in db. Non blocking
	///
	void startAll();

	///
	/// @brief Stop all instances, used from hyperiond
	///
	void stopAll();

	///
	/// @brief check if a instance is allowed for management. Instance 0 represents the root instance
	/// @apram inst The instance to check
	///
	bool isInstAllowed(quint8 inst) const { return (inst > 0); }

private:
	Logger* _log;
	InstanceTable* _instanceTable;
	const QString _rootPath;
	QMap<quint8, Hyperion*> _runningInstances;
	QList<quint8> _startQueue;
};
