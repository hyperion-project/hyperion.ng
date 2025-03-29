#include <hyperion/HyperionIManager.h>

// hyperion
#include <hyperion/Hyperion.h>
#include <db/InstanceTable.h>
#include <db/SettingsTable.h>

// qt
#include <QThread>

HyperionIManager* HyperionIManager::HIMinstance;

HyperionIManager::HyperionIManager(QObject* parent)
	: QObject(parent)
	, _log(Logger::getInstance("HYPERION-INSTMGR"))
	, _instanceTable(new InstanceTable(this))
{
	HIMinstance = this;
	qRegisterMetaType<InstanceState>("InstanceState");

	_instanceTable->createInitialInstance();
}

HyperionIManager::~HyperionIManager()
{
}

QSharedPointer<Hyperion> HyperionIManager::getHyperionInstance(quint8 instanceId)
{
	QSharedPointer<Hyperion> pInstance {nullptr};
	if(_runningInstances.contains(instanceId))
	{
		return _runningInstances.value(instanceId);
	}

	if (!_runningInstances.isEmpty())
	{
		if (instanceId != GLOABL_INSTANCE_ID )
		{
			Warning(_log,"The requested instance index '%d' with name '%s' isn't running", instanceId, QSTRING_CSTR(_instanceTable->getNamebyIndex(instanceId)));
		}
	}
	return pInstance;
}

QVector<QVariantMap> HyperionIManager::getInstanceData() const
{
	QVector<QVariantMap> instances = _instanceTable->getAllInstances();
	for( auto & entry : instances)
	{
		// add running state
		entry["running"] = _runningInstances.contains(static_cast<quint8>(entry["instance"].toInt()));
	}
	return instances;
}

QString HyperionIManager::getInstanceName(quint8 instanceId)
{
	return _instanceTable->getNamebyIndex(instanceId);
}

QSet<quint8> HyperionIManager::getRunningInstanceIdx() const
{
	const auto runningInstances = _runningInstances.keys();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	QSet<quint8> instanceIds (runningInstances.begin(), runningInstances.end());
#else
	QSet<quint8> instanceIds;
	for (const auto &id : runningInstances) {
		instanceIds.insert(id);
	}
#endif
	return instanceIds;
}

QSharedPointer<Hyperion> HyperionIManager::getFirstRunningInstance()
{
	QSharedPointer<Hyperion> hyperion {nullptr};
	if (!_runningInstances.isEmpty())
	{
		hyperion = _runningInstances.first();
	}
	return hyperion;
}

QSet<quint8> HyperionIManager::getInstanceIds() const
{
	return _instanceTable->getAllInstanceIDs();
}

void HyperionIManager::startAll()
{
	QVector<QVariantMap> const instances = _instanceTable->getAllInstances(true);
	if (instances.isEmpty())
	{
		Error(_log, "No enabled instances found to be started");
		return;
	}

	for(const auto & entry : instances)
	{
		startInstance(static_cast<quint8>(entry["instance"].toInt()));
	}
}

void HyperionIManager::stopAll()
{
	Debug(_log, "Running instances: %d, starting instances: %d"
		  , _runningInstances.size()
		  , _startingInstances.size());

	if( _runningInstances.empty())
	{
		QMetaObject::invokeMethod(this, "handleFinished", Qt::QueuedConnection);
		return;
	}

	QMap<quint8, QSharedPointer<Hyperion>> const instCopy = _runningInstances;
	for(auto const &instance : instCopy)
	{
		QString const instanceName = _instanceTable->getNamebyIndex(instance->getInstanceIndex());
		QMetaObject::invokeMethod(instance.get(), "stop", Q_ARG(QString, instanceName));
	}
}

void HyperionIManager::handleEvent(Event event)
{
	Debug(_log,"%s Event [%d] received", eventToString(event), event);
	switch (event) {
	case Event::Suspend:
		toggleSuspend(true);
	break;

	case Event::Resume:
		toggleSuspend(false);
	break;

	case Event::Idle:
		toggleIdle(true);
	break;

	case Event::ResumeIdle:
		toggleIdle(false);
	break;

	default:
	break;
	}
}

void HyperionIManager::toggleSuspend(bool isSuspend)
{
	Info(_log,"Put all instances in %s state", isSuspend ? "suspend" : "working");
	for(const auto& instance : std::as_const(_runningInstances))
	{
		emit instance->suspendRequest(isSuspend);
	}
}

void HyperionIManager::toggleIdle(bool isIdle)
{
	Info(_log,"Put all instances in %s state", isIdle ? "idle" : "working");
	for(const auto& instance : std::as_const(_runningInstances))
	{
		emit instance->idleRequest(isIdle);
	}
}

void HyperionIManager::toggleStateAllInstances(bool enable)
{
	// copy the instances due to loop corruption, even with .erase() return next iter
	for(const auto& instance : std::as_const(_runningInstances))
	{
		emit instance->compStateChangeRequest(hyperion::COMP_ALL, enable);
	}
}

bool HyperionIManager::startInstance(quint8 instanceId, bool block, QObject* caller, int tan)
{
	if(_instanceTable->doesInstanceExist(instanceId))
	{
		if(!_runningInstances.contains(instanceId) && !_startingInstances.contains(instanceId))
		{
			QSharedPointer<Hyperion> const hyperion(new Hyperion(instanceId), &QObject::deleteLater);

			QThread* hyperionThread = new QThread(this);
			hyperionThread->setObjectName("HyperionThread");

			hyperion->moveToThread(hyperionThread);
			// setup thread management
			connect(hyperionThread, &QThread::started, hyperion.get(), &Hyperion::start);
			connect(hyperion.get(), &Hyperion::started, this, &HyperionIManager::handleStarted);
			connect(hyperion.get(), &Hyperion::finished, this, &HyperionIManager::handleFinished);

			// setup further connections
			// from Hyperion
			connect(hyperion.get(), &Hyperion::settingsChanged, this, &HyperionIManager::settingsChanged);
			connect(hyperion.get(), &Hyperion::videoMode, this, &HyperionIManager::requestVideoMode);
			// to Hyperion
			connect(this, &HyperionIManager::newVideoMode, hyperion.get(), &Hyperion::newVideoMode);

			// add to queue and start
			_startingInstances.insert(instanceId, hyperion);
			hyperionThread->start();

			// update db
			_instanceTable->setLastUse(instanceId);
			_instanceTable->setEnable(instanceId, true);

			if(block)
			{
				while(!hyperionThread->isRunning()){}
			}

			if (!_pendingRequests.contains(instanceId) && caller != nullptr)
			{
				PendingRequests const newDef{caller, tan};
				_pendingRequests[instanceId] = newDef;
			}

			return true;
		}
		Debug(_log,"Cannot start Hyperion instance (%u) - '%s'. It is already running or queued for start.",instanceId,QSTRING_CSTR(_instanceTable->getNamebyIndex(instanceId)));
		return false;
	}
	Debug(_log,"Cannot start Hyperion instance (%u).It is not configured.", instanceId);
	return false;
}

bool HyperionIManager::stopInstance(quint8 instanceId)
{
	if(_instanceTable->doesInstanceExist(instanceId))
	{
		QString const instanceName = _instanceTable->getNamebyIndex(instanceId);
		if(_runningInstances.contains(instanceId))
		{
			// notify a ON_STOP rather sooner than later, queued signal listener should have some time to drop the pointer before it's deleted
			emit instanceStateChanged(InstanceState::H_ON_STOP, instanceId);
			QSharedPointer<Hyperion> const hyperion = _runningInstances.value(instanceId);
			QMetaObject::invokeMethod(hyperion.get(), "stop", Q_ARG(QString, instanceName));

			// update db
			_instanceTable->setEnable(instanceId, false);

			return true;
		}
		Debug(_log,"Cannot stop Hyperion instance [%u] - '%s'. It is not running.'",instanceId,QSTRING_CSTR(instanceName));
		return false;
	}
	Debug(_log,"Cannot stop Hyperion instance [%u]. It is not configured.", instanceId);
	return false;
}

bool HyperionIManager::createInstance(const QString& name, bool start)
{
	quint8 instanceId = 0;
	if(_instanceTable->createInstance(name, instanceId))
	{
		SettingsTable settingsTable(instanceId);
		settingsTable.addMissingDefaults();
		Info(_log,"New Hyperion instance [%u] created with name '%s'", instanceId, QSTRING_CSTR(name));
		emit instanceStateChanged(InstanceState::H_CREATED, instanceId, name);
		emit change();

		if(start)
		{
			startInstance(instanceId);
		}

		return true;
	}
	return false;
}

bool HyperionIManager::deleteInstance(quint8 instanceId)
{
	stopInstance(instanceId);

	QString const instanceName = _instanceTable->getNamebyIndex(instanceId);
	if(_instanceTable->deleteInstance(instanceId))
	{
		Info(_log,"Hyperion instance [%u] - '%s' was deleted.", instanceId, QSTRING_CSTR(instanceName));
		emit instanceStateChanged(InstanceState::H_DELETED, instanceId);
		emit change();

		return true;
	}
	return false;
}

bool HyperionIManager::saveName(quint8 instanceId, const QString& name)
{
	if(_instanceTable->saveName(instanceId, name))
	{
		emit change();
		return true;
	}
	return false;
}

void HyperionIManager::handleFinished(const QString& name)
{
	Hyperion* hyperion = qobject_cast<Hyperion*>(sender());
	if (hyperion != nullptr)
	{
		quint8 const instanceId = hyperion->getInstanceIndex();

		// Manually stop the thread and cleanup
		QThread* thread = hyperion->thread();
		if (thread != nullptr)
		{
			thread->quit();
			thread->wait();
		}

		Info(_log,"Hyperion instance [%u] - '%s' stopped.", instanceId, QSTRING_CSTR(name));

		_startingInstances.remove(instanceId);
		_runningInstances.remove(instanceId);

		emit instanceStateChanged(InstanceState::H_STOPPED, instanceId);
		emit change();
	}

	if ( _runningInstances.isEmpty())
	{
		Info(_log,"All Hyperion instances are stopped now.");
		emit areAllInstancesStopped();
	}
}

void HyperionIManager::handleStarted()
{
	Hyperion* const hyperion = qobject_cast<Hyperion*>(sender());
	quint8 const instanceId = hyperion->getInstanceIndex();

	if (_startingInstances.contains(instanceId))
	{
		Info(_log,"Hyperion instance [%u] - '%s' started.", instanceId, QSTRING_CSTR(_instanceTable->getNamebyIndex(instanceId)));

		QSharedPointer<Hyperion> const hyperionInstance = _startingInstances.value(instanceId);
		_runningInstances.insert(instanceId, hyperionInstance);
		_startingInstances.remove(instanceId);

		emit instanceStateChanged(InstanceState::H_STARTED, instanceId);
		emit change();

		if (_pendingRequests.contains(instanceId))
		{
			PendingRequests const def = _pendingRequests.take(instanceId);
			emit startInstanceResponse(def.caller, def.tan);
			_pendingRequests.remove(instanceId);
		}
	}
	else
	{
		Error(_log, "Cannot find instance [%u] - '%s' in the starting list.",
			  instanceId, QSTRING_CSTR(_instanceTable->getNamebyIndex(instanceId)));
	}
}
