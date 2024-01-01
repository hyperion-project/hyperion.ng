#include <cec/CECHandler.h>
#include <utils/Logger.h>

#include <algorithm>

#include <libcec/cecloader.h>
#include <events/EventHandler.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

/* Enable to turn on detailed CEC logs */
#define NOVERBOSE_CEC

CECHandler::CECHandler(const QJsonDocument& config, QObject * parent)
	: QObject(parent)
	, _config(config)
	, _isInitialised(false)
	, _isOpen(false)
	, _isEnabled(false)
	, _buttonReleaseDelayMs(CEC_BUTTON_TIMEOUT)
	, _buttonRepeatRateMs(0)
	, _doubleTapTimeoutMs(CEC_DOUBLE_TAP_TIMEOUT_MS)
	, _cecEventActionMap()
{
	qRegisterMetaType<Event>("Event");

	_logger = Logger::getInstance("EVENTS-CEC");

	_cecCallbacks            = getCallbacks();
	_cecConfig               = getConfig();
	_cecConfig.callbacks     = &_cecCallbacks;
	_cecConfig.callbackParam = this;
}

CECHandler::~CECHandler()
{
}

void CECHandler::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::CECEVENTS)
	{
		if (_isInitialised)
		{
			const QJsonObject& obj = config.object();

			_isEnabled = obj["enable"].toBool(false);
			Debug(_logger, "CEC Event handling is %s", _isEnabled? "enabled" : "disabled");

			if (_isEnabled)
			{
				_buttonReleaseDelayMs = obj["buttonReleaseDelayMs"].toInt(CEC_BUTTON_TIMEOUT);
				_buttonRepeatRateMs = obj["buttonRepeatRateMs"].toInt(0);
				_doubleTapTimeoutMs = obj["doubleTapTimeoutMs"].toInt(CEC_DOUBLE_TAP_TIMEOUT_MS);

				Debug(_logger, "Remote button press release time           : %dms",_buttonReleaseDelayMs);
				Debug(_logger, "Remote button press repeat rate            : %dms",_buttonRepeatRateMs);
				Debug(_logger, "Remote button press delay before repeating : %dms",_doubleTapTimeoutMs);

				_cecConfig.iButtonReleaseDelayMs = static_cast<uint32_t>(_buttonReleaseDelayMs);
				_cecConfig.iButtonRepeatRateMs = static_cast<uint32_t>(_buttonRepeatRateMs);
				_cecConfig.iDoubleTapTimeoutMs = static_cast<uint32_t>(_doubleTapTimeoutMs);

				_cecEventActionMap.clear();
				const QJsonArray actionItems = obj["actions"].toArray();
				if (!actionItems.isEmpty())
				{
					for (const QJsonValue &item : actionItems)
					{
						QString cecEvent = item.toObject().value("event").toString();
						QString action = item.toObject().value("action").toString();
						_cecEventActionMap.insert(cecEvent, stringToEvent(action));
						Debug(_logger, "CEC-Event : \"%s\" linked to action \"%s\"", QSTRING_CSTR(cecEvent), QSTRING_CSTR(action));
					}
				}

				if (!_cecEventActionMap.isEmpty())
				{
					enable();
				}
				else
				{
					Warning(_logger, "No CEC events to listen to are configured currently.");
				}
			}
			else
			{
				disable();
			}
		}
	}
}

bool CECHandler::start()
{
	_isInitialised = false;
	if (_cecAdapter == nullptr)
	{
		_cecAdapter = LibCecInitialise(&_cecConfig);
		if(_cecAdapter == nullptr)
		{
			Error(_logger, "Failed loading libCEC library. CEC is not supported.");
		}
		else
		{
			_isInitialised = true;
		}

		handleSettingsUpdate(settings::CECEVENTS,_config);
	}
	return _isInitialised;
}

void CECHandler::stop()
{
	if (_cecAdapter != nullptr)
	{
		Info(_logger, "Stopping CEC handler");
		_cecAdapter->Close();
		UnloadLibCec(_cecAdapter);
	}
}

bool CECHandler::enable()
{
	if (_isInitialised)
	{
		if (!_isOpen)
		{
			const auto adapters = getAdapters();
			if (adapters.isEmpty())
			{
				Error(_logger, "Failed to find any CEC adapter. CEC event handling will be disabled.");
				_cecAdapter->Close();
				return false;
			}

			Info(_logger, "Auto detecting CEC adapter");
			bool opened {false};
			for (const auto & adapter : adapters)
			{
				printAdapter(adapter);
				if (!opened)
				{
					if (openAdapter(adapter))
					{

						Info(_logger, "CEC adapter '%s', type: %s initialized." , adapter.strComName, _cecAdapter->ToString(adapter.adapterType));
						opened = true;

						break;
					}
				}
			}

			if (!opened)
			{
				Error(_logger, "Could not initialize any CEC adapter.");
				_cecAdapter->Close();
			}
			else
			{
				_isOpen=true;
				QObject::connect(this, &CECHandler::signalEvent, EventHandler::getInstance().data(), &EventHandler::handleEvent);
#ifdef VERBOSE_CEC
				std::cout << "Found Devices: " << scan().toStdString() << std::endl;
#endif
			}
		}

		if (_isOpen && !_cecAdapter->SetConfiguration(&_cecConfig))
		{
			Error(_logger, "Failed setting remote button press timing parameters");
		}

		Info(_logger, "CEC handler enabled");

	}

	return _isOpen;
}

void CECHandler::disable()
{
	if (_isInitialised)
	{
		 QObject::disconnect(this, &CECHandler::signalEvent, EventHandler::getInstance().data(), &EventHandler::handleEvent);
		_cecAdapter->Close();
		_isOpen=false;
		Info(_logger, "CEC handler disabled");
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
	if (_cecAdapter == nullptr)
		return {};

	QVector<CECAdapterDescriptor> descriptors(16);
	int8_t size = _cecAdapter->DetectAdapters(descriptors.data(), static_cast<uint8_t>(descriptors.size()), nullptr, true /*quickscan*/);
	descriptors.resize(size);

	return descriptors;
}

bool CECHandler::openAdapter(const CECAdapterDescriptor & descriptor)
{
	if (_cecAdapter == nullptr)
	{
		return false;
	}

	if(!_cecAdapter->Open(descriptor.strComName))
	{
		Error(_logger, "CEC adapter '%s', type: %s failed to open.", descriptor.strComName, _cecAdapter->ToString(descriptor.adapterType));
		return false;
	}
	return true;
}

void CECHandler::printAdapter(const CECAdapterDescriptor & descriptor) const
{
	Debug(_logger, "CEC Adapter:");
	Debug(_logger, "\tName       : %s", descriptor.strComName);
	Debug(_logger, "\tPath       : %s", descriptor.strComPath);
	if (descriptor.iVendorId != 0)
	{
		Debug(_logger, "\tVendor   id: %04x", descriptor.iVendorId);
	}
	if (descriptor.iProductId != 0)
	{
		Debug(_logger, "\tProduct  id: %04x", descriptor.iProductId);
	}
	if (descriptor.iFirmwareVersion != 0)
	{
		Debug(_logger, "\tFirmware id: %d", descriptor.iFirmwareVersion);
	}
	if (descriptor.adapterType != CEC::ADAPTERTYPE_UNKNOWN)
	{
		Debug(_logger, "\tType   : %s", _cecAdapter->ToString(descriptor.adapterType));
	}
}

QString CECHandler::scan() const
{
	if (_cecAdapter == nullptr)
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
	return QJsonDocument(devices).toJson(QJsonDocument::Compact);
}

void CECHandler::triggerAction(const QString& cecEvent)
{
	Event action = _cecEventActionMap.value(cecEvent, Event::Unknown);
	if ( action != Event::Unknown )
	{
		Debug(_logger, "CEC-Event : \"%s\" triggers action \"%s\"", QSTRING_CSTR(cecEvent), eventToString(action) );
		emit signalEvent(action);
	}
}

void CECHandler::onCecLogMessage(void * context, const CECLogMessage * message)
{
#ifdef VERBOSE_CEC
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (handler == nullptr)
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
	if (handler == nullptr)
		return;

	CECAdapter * adapter = handler->_cecAdapter;

#ifdef VERBOSE_CEC
	Debug(handler->_logger, "CECHandler::onCecKeyPress: %s", adapter->ToString(key->keycode));
#endif
	switch (key->keycode) {
	case CEC::CEC_USER_CONTROL_CODE_F1_BLUE:
	case CEC::CEC_USER_CONTROL_CODE_F2_RED:
	case CEC::CEC_USER_CONTROL_CODE_F3_GREEN:
	case CEC::CEC_USER_CONTROL_CODE_F4_YELLOW:
		handler->triggerAction(adapter->ToString(key->keycode));
		break;
	default:
		break;
	}
}

void CECHandler::onCecAlert(void * context, const CECAlert alert, const CECParameter /* data */)
{
#ifdef VERBOSE_CEC
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (handler == nullptr)
		return;

	Error(handler->_logger, QSTRING_CSTR(QString("CECHandler::onCecAlert: %1")
										 .arg(alert)));
#endif
}

void CECHandler::onCecConfigurationChanged(void * context, const CECConfig * configuration)
{
#ifdef VERBOSE_CEC
	CECHandler * handler = qobject_cast<CECHandler*>(static_cast<QObject*>(context));
	if (handler == nullptr)
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
		case  CEC::CEC_OPCODE_SET_STREAM_PATH:
		{
			handler->triggerAction(adapter->ToString(command->opcode));
		}
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
	if (handler == nullptr)
		return;

	CECAdapter * adapter = handler->_cecAdapter;
	Debug(handler->_logger, QSTRING_CSTR(QString("CEC source %1 : %2")
										 .arg(activated ? "activated" : "deactivated",
											  adapter->ToString(address))
										 ));
#endif
}


