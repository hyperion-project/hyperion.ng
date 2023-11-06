#include <cec/CECHandler.h>
#include <utils/Logger.h>

#include <algorithm>

#include <libcec/cecloader.h>
#include <events/EventHandler.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QFile>

/* Enable to turn on detailed CEC logs */
#define VERBOSE_CEC

CECHandler::CECHandler()
	: _isEnabled(false)
{
	qRegisterMetaType<Event>("Event");

	_logger = Logger::getInstance("CEC");

	_cecCallbacks            = getCallbacks();
	_cecConfig               = getConfig();
	_cecConfig.callbacks     = &_cecCallbacks;
	_cecConfig.callbackParam = this;
}

CECHandler::~CECHandler()
{
	stop();
}

void CECHandler::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::CECEVENTS)
	{
		const QJsonObject& obj = config.object();

		_isEnabled = obj["Enable"].toBool(false);

		Debug(_logger, "_isEnabled: [%d]", _isEnabled);
	}
}

bool CECHandler::start()
{
	if (_cecAdapter)
	{
		return true;
	}

	//	std::string library = std::string("" CEC_LIBRARY);
	//	_cecAdapter = LibCecInitialise(&_cecConfig, QFile::exists(QString::fromStdString(library)) ? library.c_str() :CEC_DEVICE_TYPE_PLAYBACK_DEVICE nullptr);

	_cecAdapter = LibCecInitialise(&_cecConfig);
	if(_cecAdapter == nullptr)
	{
		Error(_logger, "Failed loading libCEC library. CEC is not supported.");
		return false;
	}

	Info(_logger, "CEC handler started");

	const auto adapters = getAdapters();
	if (adapters.isEmpty())
	{
		Error(_logger, "Failed to find any CEC adapter.");
		UnloadLibCec(_cecAdapter);
		_cecAdapter = nullptr;

		return false;
	}

	Info(_logger, "Auto detecting CEC adapter");
	bool opened = false;
	for (const auto & adapter : adapters)
	{
		printAdapter(adapter);

		if (!opened && openAdapter(adapter))
		{
			QObject::connect(this, &CECHandler::signalEvent, EventHandler::getInstance(), &EventHandler::handleEvent);

			Info(_logger, "CEC adapter '%s', type: %s initialized." , adapter.strComName, _cecAdapter->ToString(adapter.adapterType));
			opened = true;
			break;
		}
	}

	scan();

	if (!opened)
	{
		Error(_logger, "Could not initialize any CEC adapter.");
		UnloadLibCec(_cecAdapter);
		_cecAdapter = nullptr;
	}

	return opened;
}

void CECHandler::stop()
{
	if (_cecAdapter != nullptr)
	{
		Info(_logger, "Stopping CEC handler");

		QObject::disconnect(this, &CECHandler::signalEvent, EventHandler::getInstance(), &EventHandler::handleEvent);

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
	configuration.bActivateSource = 0;

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
	if (!_cecAdapter)
		return {};

	QVector<CECAdapterDescriptor> descriptors(16);
	//int8_t size = _cecAdapter->DetectAdapters(descriptors.data(), static_cast<uint8_t>(descriptors.size()), nullptr, true /*quickscan*/);
	int8_t size = _cecAdapter->DetectAdapters(descriptors.data(), static_cast<uint8_t>(descriptors.size()), nullptr, false /*NO quickscan*/);
	descriptors.resize(size);

	return descriptors;
}

bool CECHandler::openAdapter(const CECAdapterDescriptor & descriptor)
{
	if (!_cecAdapter)
		return false;

	if(!_cecAdapter->Open(descriptor.strComName))
	{
		Error(_logger, "CEC adapter '%s', type: %s failed to open.", descriptor.strComName, _cecAdapter->ToString(descriptor.adapterType));
		return false;
	}
	return true;
}

void CECHandler::printAdapter(const CECAdapterDescriptor & descriptor) const
{
	Info(_logger, "CEC Adapter:");
	Info(_logger, "\tName       : %s", descriptor.strComName);
	Info(_logger, "\tPath       : %s", descriptor.strComPath);
	Info(_logger, "\tVendor   id: %04x", descriptor.iVendorId);
	Info(_logger, "\tProduct  id: %04x", descriptor.iProductId);
	Info(_logger, "\tFirmware id: %d", descriptor.iFirmwareVersion);
	if (descriptor.adapterType != CEC::ADAPTERTYPE_UNKNOWN)
	{
		Info(_logger, "\tType   : %s", _cecAdapter->ToString(descriptor.adapterType));
	}
}

QString CECHandler::scan() const
{
	if (!_cecAdapter)
		return {};

	Info(_logger, "Starting CEC scan");

	QJsonArray devices;
	CECLogicalAddresses addresses = _cecAdapter->GetActiveDevices();
	for (uint8_t address = CEC::CECDEVICE_TV; address <= CEC::CECDEVICE_BROADCAST; ++address)
	{
		if (addresses[address] != 0)
		{
			CECLogicalAddress logicalAddress = static_cast<CECLogicalAddress>(address);

			QJsonObject device;
			CECVendorId vendor = static_cast<CECVendorId>(_cecAdapter->GetDeviceVendorId(logicalAddress));
			CECPowerStatus power = _cecAdapter->GetDevicePowerStatus(logicalAddress);

			device["name"    ] = _cecAdapter->GetDeviceOSDName(logicalAddress).c_str();
			device["vendor"  ] = _cecAdapter->ToString(vendor);
			device["address" ] = _cecAdapter->ToString(logicalAddress);
			device["power"   ] = _cecAdapter->ToString(power);

			devices << device;

			Info(_logger, "%s", QSTRING_CSTR(QString("\tCECDevice: %1 / %2 / %3 / %4")
											 .arg(device["name"].toString(),
											 device["vendor"].toString(),
											 device["address"].toString(),
											 device["power"].toString())
					)
			);
		}
	}

	std::cout << "Devices: " <<  QJsonDocument(devices).toJson().toStdString() << std::endl;

	return QJsonDocument(devices).toJson(QJsonDocument::Compact);
}

void CECHandler::onCecLogMessage(void * context, const CECLogMessage * message)
{
#ifdef VERBOSE_CEC
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (!handler)
		return;

	switch (message->level)
	{
	case CEC::CEC_LOG_ERROR:
		Error(handler->_logger, "%s", message->message);
		break;
	case CEC::CEC_LOG_WARNING:
		Warning(handler->_logger, "%s", message->message);
		break;
	case CEC::CEC_LOG_TRAFFIC:
	case CEC::CEC_LOG_NOTICE:
		Info(handler->_logger, "%s", message->message);
		break;
	case CEC::CEC_LOG_DEBUG:
		Debug(handler->_logger, "%s", message->message);
		break;
	default:
		break;
	}
#endif
}

void CECHandler::onCecKeyPress(void * context, const CECKeyPress * key)
{
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (!handler)
		return;

	CECAdapter * adapter = handler->_cecAdapter;

#ifdef VERBOSE_CEC
	Debug(handler->_logger, "CECHandler::onCecKeyPress: %s", adapter->ToString(key->keycode));
#endif

	switch (key->keycode) {
	case CEC::CEC_USER_CONTROL_CODE_F1_BLUE:
			emit handler->signalEvent(Event::ToggleIdle);
		break;
	case CEC::CEC_USER_CONTROL_CODE_F2_RED:
			emit handler->signalEvent(Event::Suspend);
		break;
	case CEC::CEC_USER_CONTROL_CODE_F3_GREEN:
			emit handler->signalEvent(Event::Resume);
		break;

	case CEC::CEC_USER_CONTROL_CODE_F4_YELLOW:
			emit handler->signalEvent(Event::ToggleSuspend);
		break;
	default:
		break;
	}
}

void CECHandler::onCecAlert(void * context, const CECAlert alert, const CECParameter /* data */)
{
#ifdef VERBOSE_CEC
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (!handler)
		return;

	Error(handler->_logger, QSTRING_CSTR(QString("CECHandler::onCecAlert: %1")
										 .arg(alert)));
#endif
}

void CECHandler::onCecConfigurationChanged(void * context, const CECConfig * configuration)
{
#ifdef VERBOSE_CEC
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (!handler)
		return;

	Debug(handler->_logger, "CECHandler::onCecConfigurationChanged: %s", configuration->strDeviceName);
#endif
}

int CECHandler::onCecMenuStateChanged(void * context, const CECMenuState state)
{
#ifdef VERBOSE_CEC
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (!handler)
		return 0;

	CECAdapter * adapter = handler->_cecAdapter;

	Debug(handler->_logger, "CECHandler::onCecMenuStateChanged: %s", adapter->ToString(state));
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
	Debug(handler->_logger, "CECHandler::onCecCommandReceived: %s %s > %s)",
		  adapter->ToString(command->opcode),
		  adapter->ToString(command->initiator),
		  adapter->ToString(command->destination)
		  );
#endif
	/* We do NOT check sender */
	//if (address == CEC::CECDEVICE_TV)
	{
		switch (command->opcode) {
		case  CEC::CEC_OPCODE_STANDBY:
			Info(handler->_logger, "CEC source deactivated: %s", adapter->ToString(command->initiator));
			emit handler->signalEvent(Event::Suspend);
			break;

		case  CEC::CEC_OPCODE_SET_STREAM_PATH:
			Info(handler->_logger, "'CEC source activated: %s", adapter->ToString(command->initiator));
			emit handler->signalEvent(Event::Resume);
			break;

		default:
			break;
		}
	}
}

void CECHandler::onCecSourceActivated(void * context, const CECLogicalAddress address, const uint8_t activated)
{
	/* We use CECHandler::onCecCommandReceived for
	 * source activated/deactivated notifications. */

#ifdef VERBOSE_CEC
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (!handler)
	{
		return;
	}

	CECAdapter * adapter = handler->_cecAdapter;
	Debug(handler->_logger, QSTRING_CSTR(QString("CEC source %1 : %2")
										 .arg(activated ? "activated" : "deactivated",
											  adapter->ToString(address))
										 ));
#endif
}


