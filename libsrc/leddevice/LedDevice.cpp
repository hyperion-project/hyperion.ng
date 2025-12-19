#include <leddevice/LedDevice.h>

#include <sstream>
#include <iomanip>
#include <chrono>

#include <QResource>
#include <QStringList>
#include <QDir>
#include <QDateTime>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <QThread>

#include "hyperion/Hyperion.h"
#include <utils/JsonUtils.h>
#include <utils/WaitTime.h>

Q_LOGGING_CATEGORY(leddevice_config, "hyperion.leddevice.config");
Q_LOGGING_CATEGORY(leddevice_control, "hyperion.leddevice.control");
Q_LOGGING_CATEGORY(leddevice_flow, "hyperion.leddevice.flow");
Q_LOGGING_CATEGORY(leddevice_properties, "hyperion.leddevice.properties");
Q_LOGGING_CATEGORY(leddevice_write, "hyperion.leddevice.write");

#define TRACK_DEVICE(category,action, ...) qCDebug(category).noquote() << QString("|%1| %2 device '%3'").arg(_log->getSubName(), action, _activeDeviceType) << ##__VA_ARGS__;

// Constants
namespace {

	// Configuration settings
	const char CONFIG_HARDWARE_LED_COUNT[] = "hardwareLedCount";
	const char CONFIG_COLOR_ORDER[] = "colorOrder";
	const char CONFIG_AUTOSTART[] = "autoStart";
	const char CONFIG_LATCH_TIME[] = "latchTime";
	const char CONFIG_REWRITE_TIME[] = "rewriteTime";

	const int DEFAULT_LED_COUNT{ 1 };
	const char DEFAULT_COLOR_ORDER[]{ "RGB" };
	const bool DEFAULT_IS_AUTOSTART{ true };

	const char CONFIG_ENABLE_ATTEMPTS[] = "enableAttempts";
	const char CONFIG_ENABLE_ATTEMPTS_INTERVALL[] = "enableAttemptsInterval";

	const int DEFAULT_MAX_ENABLE_ATTEMPTS{ 5 };
	constexpr std::chrono::seconds DEFAULT_ENABLE_ATTEMPTS_INTERVAL{ 5 };
	constexpr std::chrono::milliseconds SWITCH_OFF_WAIT_TIMEOUT{ 5000 };

} //End of constants

LedDevice::LedDevice(const QJsonObject& deviceConfig, QObject* parent)
	: QObject(parent)
	, _devConfig(deviceConfig)
	, _log(Logger::getInstance("LEDDEVICE"))
	, _ledBuffer(0)
	, _refreshTimer(nullptr)
	, _refreshTimerInterval_ms(0)
	, _latchTime_ms(0)
	, _ledCount(0)
	, _isRestoreOrigState(false)
	, _isStayOnAfterStreaming(false)
	, _isEnabled(false)
	, _isDeviceInitialised(false)
	, _isDeviceReady(false)
	, _isOn(false)
	, _isDeviceInError(false)
	, _isDeviceRecoverable(false)
	, _lastWriteTime(QDateTime::currentDateTime())
	, _enableAttemptsTimer(nullptr)
	, _enableAttemptTimerInterval(DEFAULT_ENABLE_ATTEMPTS_INTERVAL)
	, _enableAttempts(0)
	, _maxEnableAttempts(DEFAULT_MAX_ENABLE_ATTEMPTS)
	, _isRefreshEnabled(false)
	, _isAutoStart(true)
{
	_activeDeviceType = deviceConfig["type"].toString("UNSPECIFIED").toLower();
	TRACK_SCOPE_SUBCOMPONENT() << _activeDeviceType;	
}

LedDevice::~LedDevice()
{
	TRACK_SCOPE_SUBCOMPONENT();
}

void LedDevice::start()
{
	Info(_log, "Start LedDevice '%s'.", QSTRING_CSTR(_activeDeviceType));

	close();
	_isDeviceInitialised = false;

	if (init(_devConfig))
	{
		// Everything is OK -> enable device
		_isDeviceInitialised = true;

		if (_isAutoStart && !_isDeviceInError)
		{
				Debug(_log, "Not enabled -> enable device");
				enable();
		}
	}
}

void LedDevice::stop()
{
	trackDevice(leddevice_flow, "Stop");
	if (_isSwitchOffInProgress.load(std::memory_order_acquire))
	{
		if (QThread::currentThread() == thread())
		{
			trackDevice(leddevice_flow, "Stop deferred until current switchOff completes");
			scheduleStopAfterSwitchOff();
			return;
		}

		waitForPendingSwitchOff();
	}

	resetStopDeferral();
	this->stopEnableAttemptsTimer();
	this->disable();
	waitForPendingSwitchOff();
	this->stopRefreshTimer();
	Info(_log, "Stopped LedDevice '%s'", QSTRING_CSTR(_activeDeviceType));
	emit isStopped();
}

int LedDevice::open()
{
	_isDeviceReady = true;
	int retval = 0;

	return retval;
}

int LedDevice::close()
{
	_isDeviceReady = false;
	int retval = 0;

	return retval;
}

void LedDevice::setInError(const QString& errorMsg, bool isRecoverable)
{
	_isOn = false;
	_isDeviceInError = true;
	_isDeviceReady = false;
	_isEnabled = false;
	this->stopRefreshTimer();

	if (isRecoverable)
	{
		_isDeviceRecoverable = isRecoverable;
	}
	Error(_log, "Device disabled, device '%s' signals error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(errorMsg));
	trackDevice(leddevice_flow, "Disabled") << "due to error:" << errorMsg;
	emit isEnabledChanged(_isEnabled);
}

void LedDevice::enable()
{
	trackDevice(leddevice_flow, "Enable") << ", current state is :" << (_isEnabled ? "enabled" : "disabled");
	if (_isEnabled)
	{
		return;
	}

	if (_enableAttemptsTimer != nullptr && _enableAttemptsTimer->isActive())
	{
		_enableAttemptsTimer->stop();
	}

	_isDeviceInError = false;

	if (!_isDeviceInitialised)
	{
		_isDeviceInitialised = init(_devConfig);
	}

	if (!_isDeviceReady && open() < 0)
	{
			setInError("Failed to open device", _isDeviceRecoverable);
			QMetaObject::invokeMethod(this, "retryEnable", Qt::QueuedConnection);				
			return;
	}

	if (_isDeviceReady && switchOn())	
	{
		trackDevice(leddevice_flow, "Enabled") << "successfully";
		stopEnableAttemptsTimer();
		_isEnabled = true;
		emit isEnabledChanged(_isEnabled);
		Info(_log, "LedDevice '%s' enabled", QSTRING_CSTR(_activeDeviceType));
		return;
	}

	trackDevice(leddevice_flow, "Enabling") << "failed - trigger retry";
	QMetaObject::invokeMethod(this, "retryEnable", Qt::QueuedConnection);
}

void LedDevice::retryEnable()
{
	trackDevice(leddevice_flow, "Retrying to enable");
	if (_maxEnableAttempts > 0 && _isDeviceRecoverable)
	{
		Debug(_log, "Device's enablement failed - Start retry timer. Retried already done [%d], isEnabled: [%d]", _enableAttempts, _isEnabled);
		startEnableAttemptsTimer();
	}
	else
	{
		Debug(_log, "Device's enablement failed");
	}
}

void LedDevice::disable()
{
	trackDevice(leddevice_flow, "Disable") << ", current state is :" << (_isEnabled ? "enabled" : "disabled");
	if (!_isEnabled)
	{
		return;
	}

	_isEnabled = false;
	this->stopEnableAttemptsTimer();
	this->stopRefreshTimer();

	switchOff();
	close();

	trackDevice(leddevice_flow, "Disabled");
	emit isEnabledChanged(_isEnabled);
}

void LedDevice::setActiveDeviceType(const QString& deviceType)
{
	_activeDeviceType = deviceType;
}

bool LedDevice::init(const QJsonObject& deviceConfig)
{
	trackDevice(leddevice_config, "Init") << ", deviceConfig:" << JsonUtils::toCompact(_devConfig);

	setLedCount(deviceConfig[CONFIG_HARDWARE_LED_COUNT].toInt(DEFAULT_LED_COUNT)); // property injected to reflect real led count
	setColorOrder(deviceConfig[CONFIG_COLOR_ORDER].toString(DEFAULT_COLOR_ORDER));
	setLatchTime(deviceConfig[CONFIG_LATCH_TIME].toInt(_latchTime_ms));
	setRewriteTime(deviceConfig[CONFIG_REWRITE_TIME].toInt(_refreshTimerInterval_ms));
	setAutoStart(deviceConfig[CONFIG_AUTOSTART].toBool(DEFAULT_IS_AUTOSTART));
	setEnableAttempts(deviceConfig[CONFIG_ENABLE_ATTEMPTS].toInt(DEFAULT_MAX_ENABLE_ATTEMPTS),
	std::chrono::seconds(deviceConfig[CONFIG_ENABLE_ATTEMPTS_INTERVALL].toInt(DEFAULT_ENABLE_ATTEMPTS_INTERVAL.count()))
	);

	return true;
}

void LedDevice::startRefreshTimer()
{
	if (_refreshTimerInterval_ms > 0)
	{
		trackDevice(leddevice_flow, "Starting refresh timer") << ", interval:" << _refreshTimerInterval_ms << "ms";
		if (_isDeviceReady && _isOn)
		{
			// setup refreshTimer
			if (_refreshTimer == nullptr)
			{
				_refreshTimer.reset(new QTimer());
				_refreshTimer->setTimerType(Qt::PreciseTimer);
				connect(_refreshTimer.get(), &QTimer::timeout, this, &LedDevice::rewriteLEDs);
			}
			_refreshTimer->setInterval(_refreshTimerInterval_ms);
			_refreshTimer->start();
		}
		else
		{
			Debug(_log, "Device is not ready to start a refresh timer");
		}
	}
}

void LedDevice::stopRefreshTimer()
{
	if (_refreshTimer != nullptr)
	{
		trackDevice(leddevice_flow, "Stopping refresh timer");
		_refreshTimer->stop();
	}
}

void LedDevice::startEnableAttemptsTimer()
{
	if (!_isDeviceRecoverable)
	{
		return;
	}

	++_enableAttempts;
	trackDevice(leddevice_flow, "Starting enable retry timer") << ", current attempt:" << (_enableAttempts) << "of" << _maxEnableAttempts;

	if (_enableAttempts <= _maxEnableAttempts && _enableAttemptTimerInterval.count() > 0)
	{
		// setup enable retry timer
		if (_enableAttemptsTimer.isNull())
		{
			_enableAttemptsTimer.reset(new QTimer());
			_enableAttemptsTimer->setTimerType(Qt::PreciseTimer);
			connect(_enableAttemptsTimer.get(), &QTimer::timeout, this, &LedDevice::enable);
		}
		_enableAttemptsTimer->setInterval(static_cast<int>(_enableAttemptTimerInterval.count() * 1000)); //NOLINT

		Info(_log, "Start %d. attempt of %d to enable the device in %d seconds", _enableAttempts, _maxEnableAttempts, _enableAttemptTimerInterval.count());
		_enableAttemptsTimer->start();
	}
	else
	{
		Error(_log, "Device disabled. Maximum number of %d attempts enabling the device reached. Tried for %d seconds.", _maxEnableAttempts, _enableAttempts * _enableAttemptTimerInterval.count());
		_enableAttempts = 0;
	}
}

void LedDevice::stopEnableAttemptsTimer()
{
	if (_enableAttemptsTimer != nullptr)
	{
		trackDevice(leddevice_flow, "Stopping enable retry timer");
		_enableAttemptsTimer->stop();
		_enableAttempts = 0;
	}
}

int LedDevice::updateLeds(const QVector<ColorRgb>& ledValues)
{
	trackDevice(leddevice_write, "Update LED values on") << (_isLedUpdatePending.load() ? ", but skipping update as an LED update is pending." : "will be executed.");
	// Take the LED update into a shared buffer and return quickly
	{
		QMutexLocker locker(&_ledBufferMutex);
		_ledUpdateBuffer = ledValues;
	}

	// If a frame processing is NOT already scheduled, schedule one.
	if (!_isLedUpdatePending.exchange(true))
	{
		QTimer::singleShot(0, this, &LedDevice::processLedUpdate);
	}

	return 0; // Return immediately
}

void LedDevice::processLedUpdate()
{
	QVector<ColorRgb> valuesToProcess;
	{
		QMutexLocker locker(&_ledBufferMutex);
		valuesToProcess = _ledUpdateBuffer;
	}

	writeLedUpdate(valuesToProcess);

	_isLedUpdatePending.store(false);
}

int LedDevice::writeLedUpdate(const QVector<ColorRgb>& ledValues)
{
	if (!_isEnabled || !_isOn || !_isDeviceReady || _isDeviceInError)
	{
		// LedDevice NOT ready!
		trackDevice(leddevice_flow, "Not ready writing LED values on") << QString("Status: isEnabled=%1 isOn=%2 isDeviceReady=%3 isDeviceInError=%4")
									.arg(_isEnabled)
									.arg(_isOn)
									.arg(_isDeviceReady)
									.arg(_isDeviceInError);
		return -1;
	}

	qint64 const elapsedTimeMs = _lastWriteTime.msecsTo(QDateTime::currentDateTime());
	trackDevice(leddevice_write, "Writing LED values to") << QString("Time since last write: %1 ms, Latch time: %2 ms, number of LEDs: %3")
									.arg(elapsedTimeMs)
									.arg(_latchTime_ms)
									.arg(ledValues.size());

	if (_latchTime_ms > 0 && elapsedTimeMs < _latchTime_ms)
	{
		// Skip write as elapsedTime < latchTime
		if (_isRefreshEnabled)
		{
			//Stop timer to allow for next non-refresh update
			this->stopRefreshTimer();
		}
		return 0;
	}

	int const result = write(ledValues);
	_lastWriteTime = QDateTime::currentDateTime();

	// if device requires refreshing, save Led-Values and restart the timer
	if (_isRefreshEnabled && _isEnabled)
	{
		_lastLedValues = ledValues;
		this->startRefreshTimer();
	}

	return result;
}

int LedDevice::rewriteLEDs()
{
	trackDevice(leddevice_write, "Rewriting LED values") << (_isLedUpdatePending.load() ? ", but skipping update as an LED update is pending." : "will be executed.");
	bool expected = false;
	if (!_isLedUpdatePending.compare_exchange_strong(expected, true))
	{
		// The flag was already true, meaning a write from updateLeds/processFrame is currently in progress or scheduled.
		// We can safely skip this refresh, as the other write will restart the timer.
		return 0;
	}

	if (!(_isEnabled && _isOn && _isDeviceReady && !_isDeviceInError))
	{
		trackDevice(leddevice_write, "Not ready rewriting LED values on") << QString("Status: isEnabled=%1 isOn=%2 isDeviceReady=%3 isDeviceInError=%4")
									.arg(_isEnabled)
									.arg(_isOn)
									.arg(_isDeviceReady)
									.arg(_isDeviceInError);
		// If Device is not ready stop timer
		this->stopRefreshTimer();
		_isLedUpdatePending.store(false);
		return -1;
	}

	bool success {true};
	if (!_lastLedValues.empty())
	{
		trackDevice(leddevice_write, "Rewriting LED values");
		success = write(_lastLedValues);
		_isLedUpdatePending.store(false);		
	}

	return success;
}

int LedDevice::writeBlack(int numberOfWrites)
{
	trackDevice(leddevice_flow, "Set LED strip to black for") << "to switch LEDs off";
	return writeColor(ColorRgb::BLACK, numberOfWrites);
}

int LedDevice::writeColor(const ColorRgb& color, int numberOfWrites)
{
	trackDevice(leddevice_flow, "Set LED strip's color for") << ", color:" << color.toQString() << ",  number of writes:" << numberOfWrites;
	int rc = -1;

	for (int i = 0; i < numberOfWrites; i++)
	{
		if (_latchTime_ms > 0)
		{
			wait(_latchTime_ms);
		}
		_lastLedValues = QVector<ColorRgb>(_ledCount, color);
		rc = write(_lastLedValues);
	}
	return rc;
}

void LedDevice::waitForPendingSwitchOff() const
{
	trackDevice(leddevice_flow, "Waiting for pending Switch OFF") << (_isSwitchOffInProgress.load() ? "is in progress" : "is not in required");
	if (!_isSwitchOffInProgress.load(std::memory_order_acquire))
	{
		return;
	}

	if (QThread::currentThread() == thread())
	{
		trackDevice(leddevice_flow, "Skip waiting for pending switchOff on device thread");
		return;
	}

	trackDevice(leddevice_flow, "Waiting for pending Switch OFF - start loop") << "Switch Off is" << (_isSwitchOffInProgress.load() ? "in progress" : "not in progress");
	QEventLoop loop;
	QMetaObject::Connection const connection = connect(this, &LedDevice::switchOffCompleted, &loop, &QEventLoop::quit, Qt::QueuedConnection);
	QTimer timeoutTimer;
	timeoutTimer.setSingleShot(true);
	QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
	timeoutTimer.start(static_cast<int>(SWITCH_OFF_WAIT_TIMEOUT.count()));
	if (_isSwitchOffInProgress.load(std::memory_order_acquire))
	{
		loop.exec();
	}
	disconnect(connection);
	if (_isSwitchOffInProgress.load(std::memory_order_acquire))
	{
		Warning(_log, "Timeout waiting for switch-off completion on device '%s'", QSTRING_CSTR(_activeDeviceType));
	}
}

bool LedDevice::beginSwitchOff()
{
	bool expected = false;
	trackDevice(leddevice_flow, "Begin Switch OFF") << "Switch Off is" << (_isSwitchOffInProgress.load() ? "in progress" : "will be started");
	return _isSwitchOffInProgress.compare_exchange_strong(expected, true, std::memory_order_acq_rel);
}

void LedDevice::endSwitchOff()
{
	_isSwitchOffInProgress.store(false, std::memory_order_release);
	trackDevice(leddevice_flow, "End Switch OFF") << "Switch Off" << (_isSwitchOffInProgress.load() ? "is in progress" : "completed");
	emit switchOffCompleted();
}

void LedDevice::scheduleStopAfterSwitchOff()
{
	if (_isStopDeferred.exchange(true))
	{
		return;
	}

	_stopDeferredConnection = connect(this, &LedDevice::switchOffCompleted, this, [this]()
	{
		const QMetaObject::Connection connection = _stopDeferredConnection;
		if (connection)
		{
			disconnect(connection);
		}
		_stopDeferredConnection = QMetaObject::Connection{};
		_isStopDeferred.store(false, std::memory_order_release);
		QMetaObject::invokeMethod(this, "stop", Qt::QueuedConnection);
	}, Qt::QueuedConnection);
}

void LedDevice::resetStopDeferral()
{
	if (_isStopDeferred.exchange(false))
	{
		const QMetaObject::Connection connection = _stopDeferredConnection;
		if (connection)
		{
			disconnect(connection);
		}
		_stopDeferredConnection = QMetaObject::Connection{};
	}
}

bool LedDevice::switchOn()
{
	trackDevice(leddevice_flow, "Switch ON") << ", current state is :" << (_isOn ? "ON" : "OFF");
	if (_isOn)
	{
		Debug(_log, "Device %s is already on. Skipping.", QSTRING_CSTR(_activeDeviceType));
		return true;
	}

	if (_isDeviceReady)
	{
		Info(_log, "Switching device %s ON", QSTRING_CSTR(_activeDeviceType));
		if (storeState())
		{
			if (powerOn())
			{
				Info(_log, "Device %s is ON", QSTRING_CSTR(_activeDeviceType));
				_isOn = true;
			}
			else
			{
				Warning(_log, "Failed switching device %s ON", QSTRING_CSTR(_activeDeviceType));
			}
		}
	}
	emit isOnChanged(_isOn);

	return _isOn;
}

bool LedDevice::switchOff()
{
	trackDevice(leddevice_flow, "Switch OFF") << ", current state is :" << (_isOn ? "ON" : "OFF");
	if (!_isOn)
	{
		waitForPendingSwitchOff();
		return true;
	}

	if (!beginSwitchOff())
	{
		waitForPendingSwitchOff();
		return true;
	}

	SwitchOffCompletionGuard completion(this);

	if (!_isDeviceInitialised)
	{
		_isOn = false;
		emit isOnChanged(_isOn);
		return false;
	}

	Info(_log, "Switching device %s OFF", QSTRING_CSTR(_activeDeviceType));

	// Disable device to ensure no standard LED updates are written/processed
	_isOn = false;

	if (_isDeviceReady)
	{
		if (_isRestoreOrigState)
		{
			//Restore devices state
			restoreState();
		}
		else
		{
			if (powerOff())
			{
				Info(_log, "Device %s is OFF", QSTRING_CSTR(_activeDeviceType));
			}
			else
			{
				Warning(_log, "Failed switching device %s OFF", QSTRING_CSTR(_activeDeviceType));
			}
		}
	}

	emit isOnChanged(_isOn);

	return true;
}

bool LedDevice::powerOff()
{
	bool rc{ true };

	trackDevice(leddevice_flow, "Power OFF") << ", current state is :" << (_isOn ? "ON" : "OFF");

	if (!_isStayOnAfterStreaming)
	{
		Debug(_log, "Power Off: %s", QSTRING_CSTR(_activeDeviceType));
		// Simulate power-off by writing a final "Black" to have a defined outcome
		if (writeBlack() < 0)
		{
			rc = false;
		}
	}
	return rc;
}

bool LedDevice::powerOn()
{
	bool rc{ true };

	trackDevice(leddevice_flow, "Power ON") << ", current state is :" << (_isOn ? "ON" : "OFF");

	Debug(_log, "Power On: %s", QSTRING_CSTR(_activeDeviceType));

	return rc;
}

bool LedDevice::storeState()
{
	bool rc{ true };

	//to be implemented by the subclass if needed

#if 0
	if (_isRestoreOrigState)
	{
		// Save device's original state
		// _originalStateValues = get device's state;
		// store original power on/off state, if available
	}
#endif

	return rc;
}

bool LedDevice::restoreState()
{
	bool rc{ true };

	//to be implemented by the subclass if needed

#if 0
	if (_isRestoreOrigState)
	{
		// Restore device's original state
		// update device using _originalStateValues
		// update original power on/off state, if supported
	}
#endif
	return rc;
}

QJsonObject LedDevice::discover(const QJsonObject& /*params*/)
{
	QJsonObject devicesDiscovered;

	devicesDiscovered.insert("ledDeviceType", _activeDeviceType);

	QJsonArray const deviceList;

	//to be implemented by the subclass if needed

	devicesDiscovered.insert("devices", deviceList);
	return devicesDiscovered;
}

QString LedDevice::discoverFirst()
{
	QString deviceDiscovered;

	//to be implemented by the subclass if needed
	return deviceDiscovered;
}

QJsonObject LedDevice::getProperties(const QJsonObject& params)
{
	QJsonObject properties;

	QJsonObject const deviceProperties;

	//to be implemented by the subclass if needed

	properties.insert("properties", deviceProperties);

	return properties;
}

void LedDevice::identify(const QJsonObject& /*params*/)
{
	//to be implemented by the subclass if needed
}

QJsonObject LedDevice::addAuthorization(const QJsonObject& /*params*/) 
{ 
	//to be implemented by the subclass if needed
	return QJsonObject(); 
}

void LedDevice::setLogger(QSharedPointer<Logger> log)
{
	_log = log;
}

void LedDevice::setLedCount(int ledCount)
{
	assert(ledCount >= 0);
	_ledCount = static_cast<uint>(ledCount);
	_ledRGBCount = _ledCount * sizeof(ColorRgb);
	_ledRGBWCount = _ledCount * sizeof(ColorRgbw);
	Debug(_log, "LedCount set to %d", _ledCount);
}

void LedDevice::setColorOrder(const QString& colorOrder)
{
	_colorOrder = colorOrder;
	Debug(_log, "ColorOrder set to %s", QSTRING_CSTR(_colorOrder.toUpper()));
}

void LedDevice::setLatchTime(int latchTime_ms)
{
	assert(latchTime_ms >= 0);
	_latchTime_ms = latchTime_ms;
	Debug(_log, "LatchTime set to %dms", _latchTime_ms);
}

void LedDevice::setAutoStart(bool isAutoStart)
{
	_isAutoStart = isAutoStart;
	Debug(_log, "AutoStart %s", (_isAutoStart ? "enabled" : "disabled"));
}

void LedDevice::setRewriteTime(int rewriteTime_ms)
{
	_refreshTimerInterval_ms = qMax(rewriteTime_ms, 0);

	if (_refreshTimerInterval_ms > 0)
	{
		_isRefreshEnabled = true;

		if (_refreshTimerInterval_ms <= _latchTime_ms)
		{
			int new_refresh_timer_interval = _latchTime_ms + 10; //NOLINT
			Warning(_log, "latchTime(%d) is bigger/equal rewriteTime(%d), set rewriteTime to %dms", _latchTime_ms, _refreshTimerInterval_ms, new_refresh_timer_interval);
			_refreshTimerInterval_ms = new_refresh_timer_interval;
		}

		Debug(_log, "Refresh interval = %dms", _refreshTimerInterval_ms);
		startRefreshTimer();
	}
	else
	{
		_isRefreshEnabled = false;
		stopRefreshTimer();
	}
}

void LedDevice::setEnableAttempts(int maxEnableRetries, std::chrono::seconds enableRetryTimerInterval)
{
	stopEnableAttemptsTimer();
	maxEnableRetries = qMax(maxEnableRetries, 0);

	_enableAttempts = 0;
	_maxEnableAttempts = maxEnableRetries;
	_enableAttemptTimerInterval = enableRetryTimerInterval;

	Debug(_log, "Max enable retries: %d, enable retry interval = %llds", _maxEnableAttempts, _enableAttemptTimerInterval.count());
}

void LedDevice::printLedValues(const QVector<ColorRgb>& ledValues)
{
	std::cout << "LedValues [" << ledValues.size() << "] [";
	for (const ColorRgb& color : ledValues)
	{
		std::cout << color;
	}
	std::cout << "]\n";
}

QString LedDevice::uint8_t_to_hex_string(const uint8_t* data, const int size, int number)
{
	if (number <= 0 || number > size)
	{
		number = size;
	}

	QByteArray const bytes(reinterpret_cast<const char*>(data), number);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
	return bytes.toHex(':');
#else
	return bytes.toHex();
#endif
}

QString LedDevice::toHex(const QByteArray& data, qsizetype number)
{
	if (number <= 0 || number > data.size())
	{
		number = data.size();
	}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
	return data.left(number).toHex(':');
#else
	return data.left(number).toHex();
#endif
}
bool LedDevice::isInitialised() const
{
	return _isDeviceInitialised;
}

bool LedDevice::isReady() const
{
	return _isDeviceReady;
}

bool LedDevice::isInError() const
{
	return _isDeviceInError;
}

int LedDevice::getLatchTime() const
{
	return _latchTime_ms;
}

int LedDevice::getRewriteTime() const
{
	return _refreshTimerInterval_ms;
}

int LedDevice::getLedCount() const
{
	return static_cast<int>(_ledCount);
}

QString LedDevice::getActiveDeviceType() const
{
	return _activeDeviceType;
}

QString LedDevice::getColorOrder() const
{
	return _colorOrder;
}

bool LedDevice::componentState() const {
	return _isEnabled;
}
