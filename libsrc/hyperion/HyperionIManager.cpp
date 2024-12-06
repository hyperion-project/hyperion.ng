#include <hyperion/HyperionIManager.h>

// hyperion
#include <hyperion/Hyperion.h>
#include <db/InstanceTable.h>

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
	_instanceTable->createDefaultInstance();
}

HyperionIManager::~HyperionIManager()
{
}

QSharedPointer<Hyperion> HyperionIManager::getHyperionInstance(quint8 instance)
{
	QSharedPointer<Hyperion> pInstance {nullptr};
	if(_runningInstances.contains(instance))
	{
		return _runningInstances.value(instance);
	}

	if (!_runningInstances.isEmpty())
	{
		Warning(_log,"The requested instance index '%d' with name '%s' isn't running, return main instance", instance, QSTRING_CSTR(_instanceTable->getNamebyIndex(instance)));
		pInstance = _runningInstances.value(0);
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

QString HyperionIManager::getInstanceName(quint8 inst)
{
	return _instanceTable->getNamebyIndex(inst);
}

QList<quint8> HyperionIManager::getRunningInstanceIdx() const
{
	return _runningInstances.keys();
}

QList<quint8> HyperionIManager::getInstanceIds() const
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

	QMap<quint8, QSharedPointer<Hyperion>> const instCopy = _runningInstances;
	for(auto const &instance : instCopy)
	{
		 QMetaObject::invokeMethod(instance.get(), "stop", Qt::QueuedConnection);
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

bool HyperionIManager::startInstance(quint8 inst, bool block, QObject* caller, int tan)
{
	if(_instanceTable->instanceExist(inst))
	{
		if(!_runningInstances.contains(inst) && !_startingInstances.contains(inst))
		{
			QSharedPointer<Hyperion> const hyperion(new Hyperion(inst), &QObject::deleteLater);

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
			_startingInstances.insert(inst, hyperion);
			hyperionThread->start();

			// update db
			_instanceTable->setLastUse(inst);
			_instanceTable->setEnable(inst, true);

			if(block)
			{
				while(!hyperionThread->isRunning()){}
			}

			if (!_pendingRequests.contains(inst) && caller != nullptr)
			{
				PendingRequests const newDef{caller, tan};
				_pendingRequests[inst] = newDef;
			}

			return true;
		}
		Debug(_log,"Can't start Hyperion instance index '%d' with name '%s' it's already running or queued for start", inst, QSTRING_CSTR(_instanceTable->getNamebyIndex(inst)));
		return false;
	}
	Debug(_log,"Can't start Hyperion instance index '%d' it doesn't exist in DB", inst);
	return false;
}

bool HyperionIManager::stopInstance(quint8 inst)
{
	// instance 0 cannot be stopped
	if(!isInstAllowed(inst))
	{
		return false;
	}

	if(_instanceTable->instanceExist(inst))
	{
		if(_runningInstances.contains(inst))
		{
			// notify a ON_STOP rather sooner than later, queued signal listener should have some time to drop the pointer before it's deleted
			emit instanceStateChanged(InstanceState::H_ON_STOP, inst);
			QSharedPointer<Hyperion> const hyperion = _runningInstances.value(inst);
			hyperion->stop();

			// update db
			_instanceTable->setEnable(inst, false);

			return true;
		}
		Debug(_log,"Can't stop Hyperion instance index '%d' with name '%s' it's not running'", inst, QSTRING_CSTR(_instanceTable->getNamebyIndex(inst)));
		return false;
	}
	Debug(_log,"Can't stop Hyperion instance index '%d' it doesn't exist in DB", inst);
	return false;
}

bool HyperionIManager::createInstance(const QString& name, bool start)
{
	quint8 inst = 0;
	if(_instanceTable->createInstance(name, inst))
	{
		Info(_log,"New Hyperion instance [%d] created with name '%s'", inst, QSTRING_CSTR(name));
		emit instanceStateChanged(InstanceState::H_CREATED, inst, name);
		emit change();

		if(start)
		{
			startInstance(inst);
		}

		return true;
	}
	return false;
}

bool HyperionIManager::deleteInstance(quint8 inst)
{
	// inst 0 can't be deleted
	if(!isInstAllowed(inst))
	{
		return false;
	}

	// stop it if required as blocking and wait
	stopInstance(inst);

	if(_instanceTable->deleteInstance(inst))
	{
		Info(_log,"Hyperion instance with index '%d' has been deleted", inst);
		emit instanceStateChanged(InstanceState::H_DELETED, inst);
		emit change();

		return true;
	}
	return false;
}

bool HyperionIManager::saveName(quint8 inst, const QString& name)
{
	if(_instanceTable->saveName(inst, name))
	{
		emit change();
		return true;
	}
	return false;
}

void HyperionIManager::handleFinished()
{
	Hyperion* hyperion = qobject_cast<Hyperion*>(sender());
	if (hyperion != nullptr)
	{
		quint8 const instance = hyperion->getInstanceIndex();

		// Manually stop the thread and cleanup
		QThread* thread = hyperion->thread();
		if (thread != nullptr)
		{
			thread->quit();
			thread->wait();
		}

		Info(_log,"Hyperion instance '%s' stopped", QSTRING_CSTR(_instanceTable->getNamebyIndex(instance)));

		_startingInstances.remove(instance);
		_runningInstances.remove(instance);

		emit instanceStateChanged(InstanceState::H_STOPPED, instance);
		emit change();

		if ( _runningInstances.size() == 0 )
		{
			Info(_log,"All Hyperion instances are stopped");
			emit areAllInstancesStopped();
		}
	}
}

void HyperionIManager::handleStarted()
{
	Hyperion* hyperion = qobject_cast<Hyperion*>(sender());
	quint8 const instanceId = hyperion->getInstanceIndex();

	if (_startingInstances.contains(instanceId))
	{
		Info(_log,"Hyperion instance '%s' has been started", QSTRING_CSTR(_instanceTable->getNamebyIndex(instanceId)));

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
		Error(_log, "Could not find instance '%s (index: %i)' in the starting list",
			  QSTRING_CSTR(_instanceTable->getNamebyIndex(instanceId)), instanceId);
	}

}
