#include <hyperion/HyperionIManager.h>

// hyperion
#include <hyperion/Hyperion.h>
#include <db/InstanceTable.h>

// qt
#include <QThread>

HyperionIManager* HyperionIManager::HIMinstance;

HyperionIManager::HyperionIManager(const QString& rootPath, QObject* parent, bool readonlyMode)
	: QObject(parent)
	, _log(Logger::getInstance("HYPERION-INSTMGR"))
	, _instanceTable( new InstanceTable(rootPath, this, readonlyMode) )
	, _rootPath( rootPath )
	, _readonlyMode(readonlyMode)
{
	HIMinstance = this;
	qRegisterMetaType<InstanceState>("InstanceState");
}

Hyperion* HyperionIManager::getHyperionInstance(quint8 instance)
{
	Hyperion* pInstance {nullptr};
	if(_runningInstances.contains(instance))
		return _runningInstances.value(instance);

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
		entry["running"] = _runningInstances.contains(entry["instance"].toInt());
	}
	return instances;
}

void HyperionIManager::startAll()
{
	for(const auto & entry : _instanceTable->getAllInstances(true))
	{
		startInstance(entry["instance"].toInt());
	}
}

void HyperionIManager::stopAll()
{
	// copy the instances due to loop corruption, even with .erase() return next iter
	QMap<quint8, Hyperion*> instCopy = _runningInstances;
	for(const auto instance : instCopy)
	{
		instance->stop();
	}
}

void HyperionIManager::suspend()
{
	Info(_log,"Suspend all instances and enabled components");
	QMap<quint8, Hyperion*> instCopy = _runningInstances;
	for(const auto instance : instCopy)
	{
		emit instance->suspendRequest(true);
	}
}

void HyperionIManager::resume()
{
	Info(_log,"Resume all instances and enabled components");
	QMap<quint8, Hyperion*> instCopy = _runningInstances;
	for(const auto instance : instCopy)
	{
		emit instance->suspendRequest(false);
	}
}

void HyperionIManager::toggleIdle(bool isIdle)
{
	Info(_log,"Put all instances in %s state", isIdle ? "idle" : "working");
	QMap<quint8, Hyperion*> instCopy = _runningInstances;
	for(const auto instance : instCopy)
	{
		emit instance->idleRequest(isIdle);
	}
}

void HyperionIManager::toggleStateAllInstances(bool enable)
{
	// copy the instances due to loop corruption, even with .erase() return next iter
	QMap<quint8, Hyperion*> instCopy = _runningInstances;
	for(const auto instance : instCopy)
	{
		emit instance->compStateChangeRequest(hyperion::COMP_ALL, enable);
	}
}

bool HyperionIManager::startInstance(quint8 inst, bool block, QObject* caller, int tan)
{
	if(_instanceTable->instanceExist(inst))
	{
		if(!_runningInstances.contains(inst) && !_startQueue.contains(inst))
		{
			QThread* hyperionThread = new QThread();
			hyperionThread->setObjectName("HyperionThread");
			Hyperion* hyperion = new Hyperion(inst, _readonlyMode);
			hyperion->moveToThread(hyperionThread);
			// setup thread management
			connect(hyperionThread, &QThread::started, hyperion, &Hyperion::start);
			connect(hyperion, &Hyperion::started, this, &HyperionIManager::handleStarted);
			connect(hyperion, &Hyperion::finished, this, &HyperionIManager::handleFinished);
			connect(hyperion, &Hyperion::finished, hyperionThread, &QThread::quit, Qt::DirectConnection);

			// setup further connections
			// from Hyperion
			connect(hyperion, &Hyperion::settingsChanged, this, &HyperionIManager::settingsChanged);
			connect(hyperion, &Hyperion::videoMode, this, &HyperionIManager::requestVideoMode);
			// to Hyperion
			connect(this, &HyperionIManager::newVideoMode, hyperion, &Hyperion::newVideoMode);

			// add to queue and start
			_startQueue << inst;
			hyperionThread->start();

			// update db
			_instanceTable->setLastUse(inst);
			_instanceTable->setEnable(inst, true);

			if(block)
			{
				while(!hyperionThread->isRunning()){};
			}

			if (!_pendingRequests.contains(inst) && caller != nullptr)
			{
				PendingRequests newDef{caller, tan};
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
	// inst 0 can't be stopped
	if(!isInstAllowed(inst))
		return false;

	if(_instanceTable->instanceExist(inst))
	{
		if(_runningInstances.contains(inst))
		{
			// notify a ON_STOP rather sooner than later, queued signal listener should have some time to drop the pointer before it's deleted
			emit instanceStateChanged(InstanceState::H_ON_STOP, inst);
			Hyperion* hyperion = _runningInstances.value(inst);
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
	quint8 inst;
	if(_instanceTable->createInstance(name, inst))
	{
		Info(_log,"New Hyperion instance created with name '%s'",QSTRING_CSTR(name));
		emit instanceStateChanged(InstanceState::H_CREATED, inst, name);
		emit change();

		if(start)
			startInstance(inst);
		return true;
	}
	return false;
}

bool HyperionIManager::deleteInstance(quint8 inst)
{
	// inst 0 can't be deleted
	if(!isInstAllowed(inst))
		return false;

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
	quint8 instance = hyperion->getInstanceIndex();

	Info(_log,"Hyperion instance '%s' has been stopped", QSTRING_CSTR(_instanceTable->getNamebyIndex(instance)));

	_runningInstances.remove(instance);
	hyperion->thread()->deleteLater();
	hyperion->deleteLater();
	emit instanceStateChanged(InstanceState::H_STOPPED, instance);
	emit change();
}

void HyperionIManager::handleStarted()
{
	Hyperion* hyperion = qobject_cast<Hyperion*>(sender());
	quint8 instance = hyperion->getInstanceIndex();

	Info(_log,"Hyperion instance '%s' has been started", QSTRING_CSTR(_instanceTable->getNamebyIndex(instance)));

	_startQueue.removeAll(instance);
	_runningInstances.insert(instance, hyperion);
	emit instanceStateChanged(InstanceState::H_STARTED, instance);
	emit change();

	if (_pendingRequests.contains(instance))
	{
		PendingRequests def = _pendingRequests.take(instance);
		emit startInstanceResponse(def.caller, def.tan);
		_pendingRequests.remove(instance);
	}
}
