#include <cec/CECHandler.h>
#include <utils/Logger.h>

#include <algorithm>

#include <libcec/cecloader.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>

/* Enable to turn on detailed CEC logs */
// #define VERBOSE_CEC

namespace
{
Logger * LOG = Logger::getInstance("CEC");
}

CECHandler::CECHandler()
{
	qRegisterMetaType<CECEvent>("CECEvent");

	_cecCallbacks            = getCallbacks();
	_cecConfig               = getConfig();
	_cecConfig.callbacks     = &_cecCallbacks;
	_cecConfig.callbackParam = this;
}

CECHandler::~CECHandler()
{
	stop();
}

bool CECHandler::start()
{
	QMutexLocker locker(&_lock);

	_cecAdapter = LibCecInitialise(&_cecConfig);
	if(!_cecAdapter)
	{
		Error(LOG, "Failed loading libcec.so");
		return false;
	}

	auto adapters = getAdapters();
	if (adapters.isEmpty())
	{
		Error(LOG, "Failed to find CEC adapter");
		UnloadLibCec(_cecAdapter);
		return false;
	}

	Info(LOG, "Auto detecting CEC adapter");
	bool opened = false;
	for (const auto & adapter : adapters)
	{
		printAdapter(adapter);

		if (!opened && openAdapter(adapter))
		{
			Info(LOG, "CEC Handler initialized with adapter : %s", adapter.strComName);

			opened = true;
		}
	}

	if (!opened)
	{
		UnloadLibCec(_cecAdapter);
		_cecAdapter = nullptr;
	}

	return opened;
}

void CECHandler::stop()
{
	QMutexLocker locker(&_lock);

	if (_cecAdapter)
	{
		_cecAdapter->Close();
		UnloadLibCec(_cecAdapter);
		_cecAdapter = nullptr;
	}
}

CECConfig CECHandler::getConfig() const
{
	CECConfig configuration;

	const std::string name("HyperionCEC");
	name.copy(configuration.strDeviceName, std::min(name.size(), sizeof(configuration.strDeviceName)));

	configuration.deviceTypes.Add(CEC::CEC_DEVICE_TYPE_RECORDING_DEVICE);
	configuration.clientVersion = CEC::LIBCEC_VERSION_CURRENT;

	return configuration;
}

CECCallbacks CECHandler::getCallbacks() const
{
	CECCallbacks callbacks;

	callbacks.sourceActivated      = onCecSourceActivated;
	callbacks.commandReceived      = onCecCommandReceived;
	callbacks.alert                = onCecAlert;
	callbacks.logMessage           = onCecLogMessage;
	callbacks.keyPress             = onCecKeyPress;
	callbacks.configurationChanged = onCecConfigurationChanged;
	callbacks.menuStateChanged     = onCecMenuStateChanged;

	return callbacks;
}

QVector<CECAdapterDescriptor> CECHandler::getAdapters() const
{
	QMutexLocker locker(&_lock);

	if (!_cecAdapter)
		return {};

	QVector<CECAdapterDescriptor> descriptors(16);
	int8_t size = _cecAdapter->DetectAdapters(descriptors.data(), descriptors.size(), nullptr, true /*quickscan*/);
	descriptors.resize(size);

	return descriptors;
}

bool CECHandler::openAdapter(const CECAdapterDescriptor & descriptor)
{
	QMutexLocker locker(&_lock);

	if (!_cecAdapter)
		return false;

	if(!_cecAdapter->Open(descriptor.strComName))
	{
		Error(LOG, QString("Failed to open the CEC adaper on port %1")
			.arg(descriptor.strComName)
				.toLocal8Bit());

		return false;
	}
	return true;
}

void CECHandler::printAdapter(const CECAdapterDescriptor & descriptor) const
{
	Info(LOG, QString("CEC Adapter:").toLocal8Bit());
	Info(LOG, QString("\tName   : %1").arg(descriptor.strComName).toLocal8Bit());
	Info(LOG, QString("\tPath   : %1").arg(descriptor.strComPath).toLocal8Bit());
}

QString CECHandler::scan() const
{
	Info(LOG, "Starting CEC scan");

	QMutexLocker locker(&_lock);

	if (!_cecAdapter)
		return {};

	QJsonArray devices;
	CECLogicalAddresses addresses = _cecAdapter->GetActiveDevices();
	for (int address = CEC::CECDEVICE_TV; address < CEC::CECDEVICE_BROADCAST; ++address)
	{
		if (addresses[address])
		{
			QJsonObject device;
			CECVendorId vendor = (CECVendorId)_cecAdapter->GetDeviceVendorId((CECLogicalAddress)address);
			CECPowerStatus power = _cecAdapter->GetDevicePowerStatus((CECLogicalAddress)address);

			device["name"    ] = _cecAdapter->GetDeviceOSDName((CECLogicalAddress)address).c_str();
			device["vendor"  ] = _cecAdapter->ToString((CECVendorId)vendor);
			device["address" ] = _cecAdapter->ToString((CECLogicalAddress)address);
			device["power"   ] = _cecAdapter->ToString(power);

			devices << device;

			Info(LOG, QString("\tCECDevice: %1 / %2 / %3 / %4")
				.arg(device["name"].toString())
				.arg(device["vendor"].toString())
				.arg(device["address"].toString())
				.arg(device["power"].toString())
				.toLocal8Bit());
		}
	}

	return QJsonDocument(devices).toJson(QJsonDocument::Compact);
}

void CECHandler::onCecLogMessage(void * context, const CECLogMessage * message)
{
#ifdef VERBOSE_CEC
	Debug(LOG, QString("%1")
		.arg(message->message)
			.toLocal8Bit());
#endif
}

void CECHandler::onCecKeyPress(void * context, const CECKeyPress * key)
{
#ifdef VERBOSE_CEC
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (!handler)
		return;

	CECAdapter * adapter = handler->_cecAdapter;

	Debug(LOG, QString("CECHandler::onCecKeyPress: %1")
		.arg(adapter->ToString(key->keycode))
			.toLocal8Bit());
#endif
}

void CECHandler::onCecAlert(void * context, const CECAlert alert, const CECParameter data)
{
#ifdef VERBOSE_CEC
	Error(LOG, QString("CECHandler::onCecAlert: %1")
		.arg(alert)
			.toLocal8Bit());
#endif
}

void CECHandler::onCecConfigurationChanged(void * context, const CECConfig * configuration)
{
#ifdef VERBOSE_CEC
	Debug(LOG, QString("CECHandler::onCecConfigurationChanged: %1")
		.arg(configuration->strDeviceName)
			.toLocal8Bit());
#endif
}

int CECHandler::onCecMenuStateChanged(void * context, const CECMenuState state)
{
#ifdef VERBOSE_CEC
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (!handler)
		return 0;

	CECAdapter * adapter = handler->_cecAdapter;

	Debug(LOG, QString("CECHandler::onCecMenuStateChanged: %1")
		.arg(adapter->ToString(state))
			.toLocal8Bit());
#endif
	return 0;
}

void CECHandler::onCecCommandReceived(void * context, const CECCommand * command)
{
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (!handler)
		return;

	CECAdapter * adapter = handler->_cecAdapter;

#ifdef VERBOSE_CEC
	Debug(LOG, QString("CECHandler::onCecCommandReceived: %1 (%2 > %3)")
		.arg(adapter->ToString(command->opcode))
		.arg(adapter->ToString(command->initiator))
		.arg(adapter->ToString(command->destination))
			.toLocal8Bit());
#endif
	if (command->opcode == CEC::CEC_OPCODE_STANDBY)
	{
		Info(LOG, QString("CEC source deactivated: #%1")
			.arg(adapter->ToString(command->initiator))
				.toLocal8Bit());

		if (command->initiator == CEC::CECDEVICE_TV)
		{
			emit handler->cecEvent(CECEvent::Off);
		}
	}
}

void CECHandler::onCecSourceActivated(void * context, const CECLogicalAddress address, const uint8_t activated)
{
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (!handler)
		return;

	CECAdapter * adapter = handler->_cecAdapter;

	Debug(LOG, QString("CEC source activated: %1")
		.arg(adapter->ToString(address))
			.toLocal8Bit());

	if (activated && address == CEC::CECDEVICE_TV)
	{
		emit handler->cecEvent(CECEvent::On);
	}
}
