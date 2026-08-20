// STL includes
#include<algorithm>
#include<string>
#include<cmath>

// QT includes
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QThread>
#include <QVariantMap>
#include <QtMath>
#include <QDateTime>

#ifdef _WIN32
#include <windows.h>
#endif

// hyperion include
#include <hyperion/Hyperion.h>

#include <hyperion/ImageProcessor.h>
#include <hyperion/ColorAdjustment.h>

// utils
#include <utils/hyperion.h>
#include <utils/GlobalSignals.h>
#include <utils/Logger.h>
#include <utils/JsonUtils.h>
#include "utils/WaitTime.h"
#include "utils/MemoryTracker.h"

// LedDevice includes
#include <leddevice/LedDeviceWrapper.h>

#include <hyperion/MultiColorAdjustment.h>
#include <hyperion/LinearColorSmoothing.h>

// Priority used for startup source (below FG_PRIORITY=1, above BG_PRIORITY=254)
static const int STARTUPSOURCE_PRIORITY = 100;

#if defined(ENABLE_EFFECTENGINE)
// effect engine includes
#include <effectengine/EffectEngine.h>
#endif

// settingsManagaer
#include <hyperion/SettingsManager.h>

// BGEffectHandler
#include <hyperion/BGEffectHandler.h>

// CaptureControl (Daemon capture)
#include <hyperion/CaptureCont.h>

// Boblight
#if defined(ENABLE_BOBLIGHT_SERVER)
#include <boblightserver/BoblightServer.h>
#endif

Q_LOGGING_CATEGORY(instance_flow, "hyperion.instance.flow");
Q_LOGGING_CATEGORY(instance_update, "hyperion.instance.update");

// Constants
namespace {
	const double DEFAULT_SKIPPEDUPDATES_LOWERBOUND = {5}; // Report skipped updates only if greater 5%
	constexpr std::chrono::seconds DEFAULT_STATISTICS_INTERVAL{ 60 }; //Generate statistics every 60 seconds
} //End of constants

Hyperion::Hyperion(quint8 instance, QObject* parent)
	: QObject(parent)
	, _instIndex(instance)
	, _settingsManager(nullptr)
	, _componentRegister(nullptr)
	, _imageProcessor(nullptr)
	, _raw2ledAdjustment(nullptr)
	, _muxer(nullptr)
	, _ledDeviceWrapper(nullptr)
	, _deviceSmooth(nullptr)
	, _captureCont(nullptr)
	, _BGEffectHandler(nullptr)
#if defined(ENABLE_EFFECTENGINE)	
	, _effectEngine(nullptr)
#endif	
#if defined(ENABLE_BOBLIGHT_SERVER)
	, _boblightServer(nullptr)
#endif
	, _log(nullptr)
	, _hwLedCount(0)
	, _layoutLedCount(0)
	, _colorOrder("rgb")
	, _statisticsTimer(nullptr)
	, _suspendOnStart(false)
{
	qRegisterMetaType<ComponentList>("ComponentList");
	qRegisterMetaType<Image<ColorRgb>>("ColorRgbImage");

	QString const subComponent = "I" + QString::number(_instIndex);
	this->setProperty("instance", QVariant::fromValue(subComponent));

	_log = Logger::getInstance("HYPERION", subComponent);
	TRACK_SCOPE_SUBCOMPONENT();
}

Hyperion::~Hyperion()
{
	Debug(_log, "Hyperion instance [%u] is stopping...", _instIndex);
	TRACK_SCOPE_SUBCOMPONENT();
}

#ifdef _WIN32
static void enumerateCloudStoreKeys()
{
	QSharedPointer<Logger> log = Logger::getInstance("CLOUD");
	const wchar_t* basePaths[] = {
		L"Software\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\DefaultAccount\\Current",
		L"Software\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\DefaultAccount\\Cloud"
	};
	for (int bi = 0; bi < 2; ++bi)
	{
		HKEY hBase = nullptr;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, basePaths[bi], 0, KEY_ENUMERATE_SUB_KEYS, &hBase) != ERROR_SUCCESS)
			continue;
		wchar_t subKeyName[256];
		DWORD subKeySize = 256;
		for (DWORD ki = 0; RegEnumKeyExW(hBase, ki, subKeyName, &subKeySize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS; ++ki)
		{
			std::wstring subPath = std::wstring(basePaths[bi]) + L"\\" + subKeyName;
			HKEY hSub = nullptr;
			if (RegOpenKeyExW(HKEY_CURRENT_USER, subPath.c_str(), 0, KEY_ENUMERATE_SUB_KEYS | KEY_READ, &hSub) != ERROR_SUCCESS)
			{
				subKeySize = 256;
				continue;
			}
			// Check for Data value at this level
			{
				BYTE buf[8192];
				DWORD sz = sizeof(buf);
				if (RegQueryValueExW(hSub, L"Data", nullptr, nullptr, buf, &sz) == ERROR_SUCCESS)
				{
					QString qPath = QString::fromWCharArray(subPath.c_str());
					QString nlHex;
					for (DWORD nlDi = 0; nlDi < sz; ++nlDi)
						nlHex += QString::asprintf("%02X ", buf[nlDi]);
					Debug(log, "CloudStore key [%s] blob(%lu): %s", QSTRING_CSTR(qPath), sz, QSTRING_CSTR(nlHex.trimmed()));
				}
			}
			// Enumerate sub-sub keys
			{
				wchar_t subSubName[256];
				DWORD subSubSize = 256;
				for (DWORD sj = 0; RegEnumKeyExW(hSub, sj, subSubName, &subSubSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS; ++sj)
				{
					std::wstring subSubPath = subPath + L"\\" + subSubName;
					HKEY hSubSub = nullptr;
					if (RegOpenKeyExW(HKEY_CURRENT_USER, subSubPath.c_str(), 0, KEY_READ, &hSubSub) == ERROR_SUCCESS)
					{
						BYTE buf[8192];
						DWORD sz = sizeof(buf);
						if (RegQueryValueExW(hSubSub, L"Data", nullptr, nullptr, buf, &sz) == ERROR_SUCCESS)
						{
							QString qPath = QString::fromWCharArray(subSubPath.c_str());
							QString nlHex;
							for (DWORD nlDi = 0; nlDi < sz; ++nlDi)
								nlHex += QString::asprintf("%02X ", buf[nlDi]);
							Debug(log, "CloudStore key [%s] blob(%lu): %s", QSTRING_CSTR(qPath), sz, QSTRING_CSTR(nlHex.trimmed()));
						}
						RegCloseKey(hSubSub);
					}
					subSubSize = 256;
				}
			}
			RegCloseKey(hSub);
			subKeySize = 256;
		}
		RegCloseKey(hBase);
	}
}
#endif

static QPair<QDateTime, QDateTime> calculateSunriseSunset(double latitude, double longitude, const QDate& date)
{
	// Returns (sunrise, sunset) in local time. Invalid QDateTime for polar day/night.
	// Uses the NOAA sunrise/sunset algorithm.
	double latRad = qDegreesToRadians(latitude);
	int dayOfYear = date.dayOfYear();
	double N = dayOfYear;
	double lngHour = longitude / 15.0;

	auto calcTime = [&](double hourAngleFactor) -> double {
		double t = N + (hourAngleFactor - lngHour) / 24.0;
		double M = 0.9856 * t - 3.289;
		double L = M + 1.916 * qSin(qDegreesToRadians(M)) + 0.020 * qSin(qDegreesToRadians(2.0 * M)) + 282.634;
		L = fmod(L, 360.0);
		if (L < 0) L += 360;
		double RA = qRadiansToDegrees(qAtan(0.91764 * qTan(qDegreesToRadians(L))));
		RA += 90.0 * (std::floor(L / 90.0) - std::floor(RA / 90.0));
		RA /= 15.0;
		double sinDec = 0.39782 * qSin(qDegreesToRadians(L));
		double cosDec = qCos(qAsin(sinDec));
		double cosH = (qCos(qDegreesToRadians(90.833)) - sinDec * qSin(latRad)) / (cosDec * qCos(latRad));
		if (cosH < -1.0 || cosH > 1.0)
			return -999.0; // polar
		double H = qRadiansToDegrees(qAcos(cosH)) / 15.0;
		if (hourAngleFactor < 12.0) H = -H; // sunrise: negative hour angle
		double T = H + RA - 0.06571 * t - 6.622;
		double UT = T - lngHour;
		UT = fmod(UT, 24.0);
		if (UT < 0) UT += 24.0;
		return UT;
	};

	double utSunrise = calcTime(6.0);
	double utSunset = calcTime(18.0);

	QPair<QDateTime, QDateTime> result;
	if (utSunrise < -100 || utSunset < -100)
	{
		// Polar day or night: check which one
		double t = N + (12.0 - lngHour) / 24.0;
		double M = 0.9856 * t - 3.289;
		double L = M + 1.916 * qSin(qDegreesToRadians(M)) + 0.020 * qSin(qDegreesToRadians(2.0 * M)) + 282.634;
		L = fmod(L, 360.0);
		double sinDec = 0.39782 * qSin(qDegreesToRadians(L));
		double cosDec = qCos(qAsin(sinDec));
		double cosH = (qCos(qDegreesToRadians(90.833)) - sinDec * qSin(latRad)) / (cosDec * qCos(latRad));
		if (cosH > 1.0)
		{
			// Sun never rises (polar night)
			result.first = QDateTime(date, QTime(12, 0), Qt::LocalTime);
			result.second = QDateTime(date, QTime(12, 0), Qt::LocalTime);
		}
		else
		{
			// Sun never sets (midnight sun)
			result.first = QDateTime();
			result.second = QDateTime();
		}
		return result;
	}
	int srH = static_cast<int>(std::floor(utSunrise));
	int srM = static_cast<int>(std::floor((utSunrise - srH) * 60.0));
	int ssH = static_cast<int>(std::floor(utSunset));
	int ssM = static_cast<int>(std::floor((utSunset - ssH) * 60.0));
	result.first = QDateTime(date, QTime(srH, srM, 0), Qt::UTC).toLocalTime();
	result.second = QDateTime(date, QTime(ssH, ssM, 0), Qt::UTC).toLocalTime();
	return result;
}

void Hyperion::start()
{
	Debug(_log, "Hyperion instance starting...");

#ifdef _WIN32
	enumerateCloudStoreKeys();
	{
		const wchar_t* nlPaths[] = {
			L"Software\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\DefaultAccount\\Current\\"
			L"default$windows.data.bluelightreduction.bluelightreductionstate\\"
			L"windows.data.bluelightreduction.bluelightreductionstate",
			L"Software\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\DefaultAccount\\Cloud\\"
			L"default$windows.data.bluelightreduction.bluelightreductionstate\\"
			L"windows.data.bluelightreduction.bluelightreductionstate"
		};
		const char* nlLabels[] = { "Cur", "Cld" };
		for (int nlPi = 0; nlPi < 2; ++nlPi)
		{
			HKEY hKey = nullptr;
			LSTATUS regStatus = RegOpenKeyExW(HKEY_CURRENT_USER, nlPaths[nlPi], 0, KEY_READ, &hKey);
			if (regStatus != ERROR_SUCCESS)
				continue;
			BYTE nlBuffer[8192];
			DWORD nlSize = sizeof(nlBuffer);
			regStatus = RegQueryValueExW(hKey, L"Data", nullptr, nullptr, nlBuffer, &nlSize);
			RegCloseKey(hKey);
			if (regStatus != ERROR_SUCCESS || nlSize < 5)
				continue;
			QString nlHex;
			for (DWORD nlDi = 0; nlDi < nlSize; ++nlDi)
				nlHex += QString::asprintf("%02X ", nlBuffer[nlDi]);
			Debug(_log, "Night Light blob [%s] hex: %s", nlLabels[nlPi], QSTRING_CSTR(nlHex.trimmed()));
			break;
		}
	}
#endif

	_statisticsTimer.reset(new QTimer());
	_statisticsTimer->setTimerType(Qt::PreciseTimer);
	_statisticsTimer->setInterval(DEFAULT_STATISTICS_INTERVAL.count() * 1000);
	connect(_statisticsTimer.get(), &QTimer::timeout, this, &Hyperion::reportImagesProcessedStatistics);

	_settingsManager.reset(new SettingsManager(_instIndex));

	// link settings changed with the current Hyperion instance
	connect(_settingsManager.get(), &SettingsManager::settingsChanged, this, &Hyperion::settingsChanged);
	// listen for settings updates of this instance (LEDS & COLOR)
	connect(_settingsManager.get(), &SettingsManager::settingsChanged, this, &Hyperion::handleSettingsUpdate);

	_componentRegister = MAKE_TRACKED_SHARED(ComponentRegister, sharedFromThis());
	connect(this, &Hyperion::isSetNewComponentState, _componentRegister.get(), &ComponentRegister::setNewComponentState);

	// get newVideoMode from HyperionIManager
	connect(this, &Hyperion::newVideoMode, this, &Hyperion::handleNewVideoMode);

	// handle hwLedCount
	_hwLedCount = getSetting(settings::DEVICE).object()["hardwareLedCount"].toInt(1);
	_colorOrder = getSetting(settings::DEVICE).object()["colorOrder"].toString("rgb");

	_muxer = MAKE_TRACKED_SHARED(PriorityMuxer, _hwLedCount, this);

	// connect Hyperion::update with Muxer visible priority changes as muxer updates independent
	connect(_muxer.get(), &PriorityMuxer::visiblePriorityChanged, this, &Hyperion::update);
	connect(_muxer.get(), &PriorityMuxer::visiblePriorityChanged, this, &Hyperion::handleSourceAvailability);
	connect(_muxer.get(), &PriorityMuxer::visibleComponentChanged, this, &Hyperion::handleVisibleComponentChanged);

	connect(_muxer.get(), &PriorityMuxer::visiblePriorityChanged, this, &Hyperion::resetImagesProcessedStatistics);
	connect(_muxer.get(), &PriorityMuxer::visibleComponentChanged, this, &Hyperion::resetImagesProcessedStatistics);

	QJsonArray const ledLayout = getSetting(settings::LEDS).array();
	updateLedLayout(ledLayout);
	_ledBuffer = QVector<ColorRgb>(static_cast<size_t>(_hwLedCount), ColorRgb::BLACK);

	// smoothing
	_deviceSmooth = MAKE_TRACKED_SHARED(LinearColorSmoothing, getSetting(settings::SMOOTHING).object(), sharedFromThis());

	connect(this, &Hyperion::settingsChanged, _deviceSmooth.get(), &LinearColorSmoothing::handleSettingsUpdate);
	_deviceSmooth->start();

	// initialize LED-devices
	QJsonObject const ledDeviceSettings = getSetting(settings::DEVICE).object();

	_ledDeviceWrapper = MAKE_TRACKED_SHARED(LedDeviceWrapper, sharedFromThis());
	connect(this, &Hyperion::compStateChangeRequest, _ledDeviceWrapper.get(), &LedDeviceWrapper::handleComponentState);
	connect(this, &Hyperion::ledDeviceData, _ledDeviceWrapper.get(), &LedDeviceWrapper::updateLeds);

	// initialize twilight auto-suspend BEFORE device creation to prevent auto-start during daytime
	{
		_isTwilightNight = true; // assume night until proven otherwise
		_twilightTimer.reset(new QTimer(this));
		_twilightTimer->setInterval(600000);
		connect(_twilightTimer.get(), &QTimer::timeout, this, &Hyperion::checkTwilightState);
		QJsonDocument twDoc = getSetting(settings::TWILIGHT);
		if (!twDoc.isNull() && twDoc.isObject())
		{
			QJsonObject twObj = twDoc.object();
			_twilightEnabled = twObj["enabled"].toBool(false);
			_twilightLatitude = twObj["latitude"].toDouble(55.7558);
			_twilightLongitude = twObj["longitude"].toDouble(37.6173);
		}

		if (_twilightEnabled)
		{
			QDate today = QDate::currentDate();
			QDateTime sunrise, sunset;
			auto pair = calculateSunriseSunset(_twilightLatitude, _twilightLongitude, today);
			sunrise = pair.first;
			sunset = pair.second;
			QDateTime now = QDateTime::currentDateTime();
			bool isNight = false;
			if (!sunrise.isValid()) { isNight = false; }
			else if (sunrise == sunset) { isNight = true; }
			else if (sunset > sunrise) { isNight = (now >= sunset || now < sunrise); }
			else { isNight = (now >= sunset && now < sunrise); }
			_isTwilightNight = isNight;

			if (!isNight)
			{
				_ledDeviceWrapper->setStartEnabled(false);
				Info(_log, "Twilight: daytime detected, LED device will be created but not auto-started");
			}
		}
	}

	_ledDeviceWrapper->createLedDevice(ledDeviceSettings);

	// listen for suspend/resume, idle requests to perform core activation/deactivation actions
	connect(this, &Hyperion::suspendRequest, this, &Hyperion::setSuspend);
	connect(this, &Hyperion::idleRequest, this, &Hyperion::setIdle);

	_muxer->start();

#if defined(ENABLE_EFFECTENGINE)
	// create the effect engine; needs to be initialized after smoothing!
	_effectEngine = MAKE_TRACKED_SHARED(EffectEngine, sharedFromThis());
	connect(_effectEngine.get(), &EffectEngine::effectListUpdated, this, &Hyperion::effectListUpdated);
	connect(this, &Hyperion::stopEffects, _effectEngine.get(), &EffectEngine::stopAllEffects);
#endif
	// initial startup effect
	hyperion::handleInitialEffect(this, getSetting(settings::FGEFFECT).object());

	// apply saved startup source (overrides foreground effect if set)
	QJsonDocument startupSrcDoc = getSetting(settings::STARTUPSOURCE);
	Info(_log, "Startup source read: %s", QSTRING_CSTR(QString(startupSrcDoc.toJson(QJsonDocument::Compact))));
	if (!startupSrcDoc.isNull() && startupSrcDoc.isObject())
	{
		QJsonObject startupSrc = startupSrcDoc.object();
		if (!startupSrc.isEmpty())
		{
			QString componentId = startupSrc["componentId"].toString();
			int duration_ms = startupSrc["duration_ms"].toInt(0);
			if (duration_ms <= 0)
				duration_ms = PriorityMuxer::ENDLESS;

			if (componentId == "COLOR")
			{
				auto color = startupSrc["color"].toArray();
				if (color.size() >= 3)
				{
					QVector<ColorRgb> fg_color = {
						ColorRgb {
							static_cast<uint8_t>(color[0].toInt(0)),
							static_cast<uint8_t>(color[1].toInt(0)),
							static_cast<uint8_t>(color[2].toInt(0))
						}
					};
					Info(_log, "Applying startup source COLOR (%d %d %d)", fg_color[0].red, fg_color[0].green, fg_color[0].blue);
					setColor(STARTUPSOURCE_PRIORITY, fg_color, duration_ms, "startupSource");
					Info(_log, "Startup source color applied");
				}
			}
#if defined(ENABLE_EFFECTENGINE)
			else if (componentId == "EFFECT")
			{
				QString effectName = startupSrc["effectName"].toString();
				if (!effectName.isEmpty())
				{
					Info(_log, "Applying startup source EFFECT '%s'", QSTRING_CSTR(effectName));
					int res = setEffect(effectName, STARTUPSOURCE_PRIORITY, duration_ms, "startupSource");
					Info(_log, "Startup source effect '%s' %s", QSTRING_CSTR(effectName), (res == 0) ? "started" : "failed");
				}
			}
#endif
			else
			{
				Info(_log, "Unknown startup source component: %s", QSTRING_CSTR(componentId));
			}
		}
		else
		{
			Info(_log, "Startup source data is empty, skipping");
		}
	}
	else
	{
		Info(_log, "Startup source document is null or not an object, skipping");
	}

	// Apply initial twilight state
	if (_twilightEnabled)
	{
		if (_isTwilightNight)
		{
			if (_ledDeviceWrapper)
				_ledDeviceWrapper->handleComponentState(hyperion::COMP_LEDDEVICE, true);
			refreshUpdate();
			Info(_log, "Twilight: nighttime detected, LED device enabled");
		}
		else
		{
			if (_ledDeviceWrapper)
				_ledDeviceWrapper->handleComponentState(hyperion::COMP_LEDDEVICE, false);
			Info(_log, "Twilight: daytime detected, LED device suspended (manual resume allowed)");
		}
		_twilightTimer->start();
	}

	// handle background effect
	_BGEffectHandler = MAKE_TRACKED_SHARED(BGEffectHandler, sharedFromThis());

	// create the Daemon capture interface
	_captureCont = MAKE_TRACKED_SHARED(CaptureCont, sharedFromThis());
	_captureCont->start();

	// link global signals with the corresponding slots
	connect(GlobalSignals::getInstance(), &GlobalSignals::registerGlobalInput, this, &Hyperion::registerInput);
	connect(GlobalSignals::getInstance(), &GlobalSignals::clearGlobalInput, this, &Hyperion::clear);
	connect(GlobalSignals::getInstance(), &GlobalSignals::setGlobalColor, this, &Hyperion::setColor);
	connect(GlobalSignals::getInstance(), &GlobalSignals::setGlobalImage, this, &Hyperion::setInputImage);

	// if there is no startup / background effect and no sending capture interface we probably want to push once BLACK (as PrioMuxer won't emit a priority change)
	refreshUpdate();


	_statisticsTimer->start();

#if defined(ENABLE_BOBLIGHT_SERVER)
	// boblight, can't live in global scope as it depends on layout
	_boblightServer = MAKE_TRACKED_SHARED(BoblightServer, sharedFromThis(), getSetting(settings::BOBLSERVER));

	connect(this, &Hyperion::settingsChanged, _boblightServer.get(), &BoblightServer::handleSettingsUpdate);
#endif

	// instance initiated, enter thread event loop
	emit started();

	// Boot sequence complete (delayed so any pending refreshUpdate/singleShot(0) runs first)
	QTimer::singleShot(50, this, [this]() {
		_twilightBootComplete = true;
	});
}

void Hyperion::stop(const QString name)
{
	Debug(_log, "Hyperion instance [%u] - %s is stopping.", _instIndex, QSTRING_CSTR(name));

	//Stop Background effect first that it does not kick in when other priorities are stopped
	_BGEffectHandler->stop();

	_captureCont->stop();

#if defined(ENABLE_BOBLIGHT_SERVER)
	_boblightServer->stop();
	_boblightServer.clear();
#endif

	//Remove all priorities
	_muxer->clearAll(true);

#if defined(ENABLE_EFFECTENGINE)
	_effectEngine->stopAllEffects();
	_effectEngine.clear();
#endif

	_muxer->stop();
	_deviceSmooth->stop();

	// Trigger instance stopped when the LedDevice signals it has stopped
	connect(_ledDeviceWrapper.get(), &LedDeviceWrapper::isStopped, [this, name]()
	{
		TRACK_SCOPE_SUBCOMPONENT_CATEGORY(instance_flow) << "LedDeviceWrapper signaled it has stopped for Hyperion instance:" << QSTRING_CSTR(name);
		emit finished(name);
	});
	_ledDeviceWrapper->stopDevice();
}

void Hyperion::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if (type == settings::COLOR)
	{

		updateLedColorAdjustment(_layoutLedCount, config.object());
		refreshUpdate();
	}
	else if (type == settings::LEDS)
	{
#if defined(ENABLE_EFFECTENGINE)
		// stop and cache all running effects, as effects depend heavily on LED-layout
		_effectEngine->cacheRunningEffects();
#endif

		updateLedLayout(config.array());

#if defined(ENABLE_EFFECTENGINE)
		// start cached effects
		_effectEngine->startCachedEffects();
#endif

		refreshUpdate();
	}
	else if (type == settings::DEVICE)
	{
		QJsonObject const deviceConfig = config.object();

		// Recreate LED-Device with new configuration
		_ledDeviceWrapper->createLedDevice(deviceConfig);
		_hwLedCount = _ledDeviceWrapper->getLedCount();
		_colorOrder = _ledDeviceWrapper->getColorOrder();

		updateLedLayout(getSetting(settings::LEDS).array());
		_ledBuffer.fill(ColorRgb::BLACK, _hwLedCount);
	}
	else if (type == settings::TWILIGHT)
	{
		QJsonObject const twConfig = config.object();
		_twilightEnabled = twConfig["enabled"].toBool(false);
		_twilightLatitude = twConfig["latitude"].toDouble(55.7558);
		_twilightLongitude = twConfig["longitude"].toDouble(37.6173);
		Info(_log, "Twilight auto-suspend %s (lat=%.4f lon=%.4f)",
			_twilightEnabled ? "enabled" : "disabled",
			_twilightLatitude, _twilightLongitude);
		checkTwilightState();
	}
}

void Hyperion::updateLedColorAdjustment(int ledCount, const QJsonObject& colors)
{
	// change in LEDs are also reflected in adjustment
	_raw2ledAdjustment.reset(hyperion::createLedColorsAdjustment(ledCount, colors));
	if (!_raw2ledAdjustment->verifyAdjustments())
	{
		Warning(_log, "At least one LED has no color calibration, please add all LEDs from your LED layout to an 'LED index' field!");
	}
}

void Hyperion::updateLedLayout(const QJsonArray& ledLayout)
{
	_ledString = LedString::createLedString(ledLayout, hyperion::createColorOrder(_colorOrder), _hwLedCount);
	_layoutLedCount = static_cast<int>(_ledString.leds().size());
	_layoutGridSize = hyperion::getLedLayoutGridSize(ledLayout);

	_ledStringColorOrder.clear();
	for (const Led& led : _ledString.leds())
	{
		_ledStringColorOrder.push_back(led.colorOrder);
	}

	updateLedColorAdjustment(_layoutLedCount, getSetting(settings::COLOR).object());

	if (_imageProcessor.isNull())
	{
		_imageProcessor = MAKE_TRACKED_SHARED(ImageProcessor, _ledString, sharedFromThis());
	}
	else
	{
		_imageProcessor->setLedString(_ledString);
	}

	_muxer->updateLedColorsLength(_layoutLedCount);

	if (_layoutLedCount < static_cast<int>(_ledBuffer.size()))
	{
		std::fill(_ledBuffer.begin() + _layoutLedCount, _ledBuffer.end(), ColorRgb{ 0, 0, 0 });
	}
}

QJsonDocument Hyperion::getSetting(settings::type type) const
{
	return _settingsManager->getSetting(type);
}

QString Hyperion::getSettingString(settings::type type) const
{
	return _settingsManager->getSettingString(type);
}

QPair<bool, QStringList> Hyperion::saveSettings(const QJsonObject& config)
{
	return _settingsManager->saveSettings(config);
}

int Hyperion::getLatchTime() const
{
	return _ledDeviceWrapper->getLatchTime();
}

unsigned Hyperion::addSmoothingConfig(int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	return _deviceSmooth->addConfig(settlingTime_ms, ledUpdateFrequency_hz, updateDelay);
}

unsigned Hyperion::updateSmoothingConfig(unsigned id, int settlingTime_ms, double ledUpdateFrequency_hz, unsigned updateDelay)
{
	return _deviceSmooth->updateConfig(id, settlingTime_ms, ledUpdateFrequency_hz, updateDelay);
}

int Hyperion::getLedCount() const
{
	return _layoutLedCount;
}

void Hyperion::setSourceAutoSelect(bool state)
{
	if (!_muxer.isNull())
	{
		_muxer->setSourceAutoSelectEnabled(state);
	}	
}

bool Hyperion::setVisiblePriority(int priority)
{
	if (!_muxer.isNull())
	{
		return _muxer->setPriority(priority);
	}
	
	return false;
}

bool Hyperion::sourceAutoSelectEnabled() const
{
	if (!_muxer.isNull())
	{
		return _muxer->isSourceAutoSelectEnabled();
	}

	return false;
}

void Hyperion::setNewComponentState(hyperion::Components component, bool state)
{
	TRACK_SCOPE_SUBCOMPONENT_CATEGORY(instance_flow) << "component" << componentToString(component) << "will be set to" << (state ? "ENABLED" : "DISABLED");
	if (_componentRegister.isNull())
	{
		Debug(_log, "ComponentRegister is not initialized, cannot set state for component '%s'", componentToString(component));
	}

	emit isSetNewComponentState(component, state);
}

std::map<hyperion::Components, bool> Hyperion::getAllComponents() const
{
	return _componentRegister->getRegister();
}

int Hyperion::isComponentEnabled(hyperion::Components comp) const
{
	return _componentRegister->isComponentEnabled(comp);
}

void Hyperion::checkTwilightState()
{
	if (!_twilightEnabled)
	{
		if (_twilightTimer && _twilightTimer->isActive())
			_twilightTimer->stop();
		return;
	}
	if (!_twilightTimer->isActive())
		_twilightTimer->start();

	QDate today = QDate::currentDate();
	QDateTime sunrise, sunset;
	{
		auto pair = calculateSunriseSunset(_twilightLatitude, _twilightLongitude, today);
		sunrise = pair.first;
		sunset = pair.second;
	}
	QDateTime now = QDateTime::currentDateTime();
	bool wasNight = _isTwilightNight;

	if (!sunrise.isValid())
	{
		// Midnight sun - always day
		_isTwilightNight = false;
	}
	else if (sunrise == sunset)
	{
		// Polar night - always night
		_isTwilightNight = true;
	}
	else if (sunset > sunrise)
	{
		// Normal: sunset after sunrise (northern hemisphere)
		_isTwilightNight = (now >= sunset || now < sunrise);
	}
	else
	{
		// Southern hemisphere: sunset before sunrise (crosses midnight)
		_isTwilightNight = (now >= sunset && now < sunrise);
	}

	Debug(_log, "Twilight check: now=%s sunrise=%s sunset=%s night=%s",
		QSTRING_CSTR(now.toString("HH:mm")),
		QSTRING_CSTR(sunrise.toString("HH:mm")),
		QSTRING_CSTR(sunset.toString("HH:mm")),
		_isTwilightNight ? "YES (suspend)" : "NO (normal)");

	if (_isTwilightNight != wasNight)
	{
		if (_isTwilightNight)
		{
			Info(_log, "Twilight: night started, resuming LEDs");
			if (_ledDeviceWrapper)
			{
				_ledDeviceWrapper->handleComponentState(hyperion::COMP_LEDDEVICE, true);
			}
			refreshUpdate();
		}
		else
		{
			Info(_log, "Twilight: day started, suspending LEDs (manual resume allowed)");
			if (_ledDeviceWrapper)
			{
				_ledDeviceWrapper->handleComponentState(hyperion::COMP_LEDDEVICE, false);
				_ledDeviceWrapper->disableDevice();
			}
		}
	}
}

void Hyperion::setSuspend(bool isSuspend)
{
	bool const enable = !isSuspend;
	emit compStateChangeRequest(hyperion::COMP_ALL, enable);
}

void Hyperion::setIdle(bool isIdle)
{
	clear(-1);

	bool const enable = !isIdle;
	emit compStateChangeRequestAll(enable, { hyperion::COMP_LEDDEVICE, hyperion::COMP_SMOOTHING });
}

void Hyperion::registerInput(int priority, hyperion::Components component, const QString& origin, const QString& owner, unsigned smooth_cfg)
{
	if (!_muxer.isNull())
	{
		_muxer->registerInput(priority, component, origin, owner, smooth_cfg);
	}
}

bool Hyperion::setInput(int priority, const QVector<ColorRgb>& ledColors, int timeout_ms, bool clearEffect)
{
	if (_muxer.isNull())
	{
		return false;
	}

	if (_muxer->setInput(priority, ledColors, timeout_ms))
	{
#if defined(ENABLE_EFFECTENGINE)
		// clear effect if this call does not come from an effect
		if (clearEffect)
		{
			_effectEngine->channelCleared(priority);
		}
#endif

		// if this priority is visible, update immediately
		if (priority == _muxer->getCurrentPriority())
		{
			update();
		}

		return true;
	}
	return false;
}

bool Hyperion::setInputImage(int priority, const Image<ColorRgb>& image, int64_t timeout_ms, bool clearEffect)
{
	if (_muxer.isNull())
	{
		TRACK_SCOPE_SUBCOMPONENT_CATEGORY(image_track) << "Image [" << image.id() << "] not set, as muxer is null.";
		return false;
	}

	TRACK_SCOPE_SUBCOMPONENT_CATEGORY(image_track) << "Image [" << image.id() << "], priority" << priority << "image id" << image.id() << "timeout" << timeout_ms << "ms";

	if (!_muxer->hasPriority(priority))
	{
		emit GlobalSignals::getInstance()->globalRegRequired(priority);
		return false;
	}

	if (_muxer->setInputImage(priority, image, timeout_ms))
	{
#if defined(ENABLE_EFFECTENGINE)
		// clear effect if this call does not come from an effect
		if (clearEffect)
		{
			_effectEngine->channelCleared(priority);
		}
#endif

		// if this priority is visible, update immediately
		if (priority == _muxer->getCurrentPriority())
		{
			update();
		}

		return true;
	}
	return false;
}

bool Hyperion::setInputInactive(int priority)
{
	if (!_muxer.isNull())
	{
		return _muxer->setInputInactive(priority);
	}
	return false;
}

void Hyperion::setColor(int priority, const QVector<ColorRgb>& ledColors, int timeout_ms, const QString& origin, bool clearEffects)
{
#if defined(ENABLE_EFFECTENGINE)
	// clear effect if this call does not come from an effect
	if (clearEffects)
	{
		_effectEngine->channelCleared(priority);
	}
#endif

	// create full led vector from single/multiple colors

	QVector<ColorRgb> newLedColors;
	newLedColors.resize(_layoutLedCount);

	if (!ledColors.isEmpty())
	{
		const int ledCount   = _layoutLedCount;
		const auto colorCount = ledColors.size();

		if (colorCount == 1)
		{
			// Special case: single color, fill the entire vector
			newLedColors.fill(ledColors[0]);
		}
		else
		{
			// General case: multiple colors, repeat colors if necessary to fill the entire vector
			for (int i = 0; i < ledCount; ++i)
			{
				newLedColors[i] = ledColors[i % colorCount];		
			}
		}
	}
	else
	{
		// no color provided, set all to black
		newLedColors.fill(ColorRgb::BLACK);
	}

	// register color
	registerInput(priority, hyperion::COMP_COLOR, origin);

	// write color to muxer
	setInput(priority, newLedColors, timeout_ms);
}

QStringList Hyperion::getAdjustmentIds() const
{
	return _raw2ledAdjustment->getAdjustmentIds();
}

ColorAdjustment* Hyperion::getAdjustment(const QString& identifier) const
{
	return _raw2ledAdjustment->getAdjustment(identifier);
}

void Hyperion::adjustmentsUpdated()
{
	emit adjustmentChanged();
	refreshUpdate();
}

bool Hyperion::clear(int priority, bool forceClearAll)
{
	if (_muxer.isNull())
	{
		return false;
	}

	bool isCleared = false;
	if (priority < 0)
	{
		_muxer->clearAll(forceClearAll);

#if defined(ENABLE_EFFECTENGINE)
		// send clearall signal to the effect engine
		_effectEngine->allChannelsCleared();
#endif

		isCleared = true;
	}
	else
	{
#if defined(ENABLE_EFFECTENGINE)
		// send clear signal to the effect engine
		// (outside the check so the effect gets cleared even when the effect is not sending colors)
		_effectEngine->channelCleared(priority);
#endif

		if (_muxer->clearInput(priority))
		{
			isCleared = true;
		}
	}
	return isCleared;
}

int Hyperion::getCurrentPriority() const
{
	return  _muxer.isNull() ? PriorityMuxer::LOWEST_PRIORITY : _muxer->getCurrentPriority();
}

bool Hyperion::isCurrentPriority(int priority) const
{
	return getCurrentPriority() == priority;
}

QList<int> Hyperion::getActivePriorities() const
{
	return !_muxer.isNull() ? _muxer->getPriorities() : QList<int>();
}

Hyperion::InputsMap Hyperion::getPriorityInfo() const
{
	return !_muxer.isNull() ? _muxer->getInputInfo() : InputsMap();
}

Hyperion::InputInfo Hyperion::getPriorityInfo(int priority) const
{
	return !_muxer.isNull() ? _muxer->getInputInfo(priority) : InputInfo();
}

#if defined(ENABLE_EFFECTENGINE)
QList<ActiveEffectDefinition> Hyperion::getActiveEffects() const
{
	return !_effectEngine.isNull() ? _effectEngine->getActiveEffects() : QList<ActiveEffectDefinition>();
}

int Hyperion::setEffect(const QString& effectName, int priority, int timeout, const QString& origin)
{
	return !_effectEngine.isNull() ? _effectEngine->runEffect(effectName, priority, timeout, origin) : -1;
}

int Hyperion::setEffect(const QString& effectName, const QJsonObject& args, int priority, int timeout, const QString& pythonScript, const QString& origin, const QString& imageData)
{
	return !_effectEngine.isNull() ? _effectEngine->runEffect(effectName, args, priority, timeout, pythonScript, origin, 0, imageData) : -1;
}
#endif

void Hyperion::setLedMappingType(int mappingType)
{
	if (mappingType != _imageProcessor->getUserLedMappingType())
	{
		_imageProcessor->setLedMappingType(mappingType);
		emit imageToLedsMappingChanged(mappingType);
	}
}

int Hyperion::getLedMappingType() const
{
	return _imageProcessor.isNull() ? -1 : _imageProcessor->getUserLedMappingType();
}

void Hyperion::setVideoMode(VideoMode mode)
{
	emit videoMode(mode);
}

VideoMode Hyperion::getCurrentVideoMode() const
{
	return _currVideoMode;
}

QString Hyperion::getActiveDeviceType() const
{
	return _ledDeviceWrapper.isNull() ? QString() : _ledDeviceWrapper->getActiveDeviceType();
}

void Hyperion::handleVisibleComponentChanged(hyperion::Components comp)
{
	if (!_imageProcessor.isNull())
	{
		_imageProcessor->setBlackbarDetectDisable(comp == hyperion::COMP_EFFECT);
		_imageProcessor->setHardLedMappingType((comp == hyperion::COMP_EFFECT) ? 0 : -1);
	}
	if (!_raw2ledAdjustment.isNull())
	{
		_raw2ledAdjustment->setBacklightEnabled(comp != hyperion::COMP_COLOR && comp != hyperion::COMP_EFFECT);
	}
}
void Hyperion::handleSourceAvailability(int priority)
{
	if (_muxer.isNull())
	{
		return;
	}

	int const previousPriority = _muxer->getPreviousPriority();

	if (priority == PriorityMuxer::LOWEST_PRIORITY)
	{
		// Keep LED-device on, as background effect will kick-in shortly
		if (!_BGEffectHandler->_isEnabled())
		{
			Debug(_log, "No source left -> Pause output processing and switch LED-Device off");
			emit _ledDeviceWrapper->switchOff();
			_deviceSmooth->setPause(true);
		}
	}
	else
	{
		if (previousPriority == PriorityMuxer::LOWEST_PRIORITY)
		{
			if (_ledDeviceWrapper->isEnabled())
			{
				Debug(_log, "new source available -> LED-Device is enabled, switch LED-device on and resume output processing");
				emit _ledDeviceWrapper->switchOn();
			}
			else
			{
				Debug(_log, "new source available -> LED-Device not enabled, cannot switch on LED-device");
			}
			_deviceSmooth->setPause(false);
		}
	}
}

void Hyperion::applyBlacklist(QVector<ColorRgb>& ledColors)
{
	if (_ledString.hasBlackListedLeds())
	{
		for (const auto& id : _ledString.blacklistedLedIds())
		{
			if (id < ledColors.size())
			{
				ledColors[id] = ColorRgb::BLACK;
			}
		}
	}
}

void Hyperion::applyColorOrder(QVector<ColorRgb>& ledColors) const
{
	assert(ledColors.size() >= _ledStringColorOrder.size());

	// Only apply color order for LEDs defined by layout
	for (auto i = 0; i < _ledStringColorOrder.size(); ++i)
	{
		auto& color = ledColors[i];
		// correct the color byte order
		switch (_ledStringColorOrder.at(i))
		{
		case ColorOrder::ORDER_RGB:
			// leave as it is
			break;
		case ColorOrder::ORDER_BGR:
			std::swap(color.red, color.blue);
			break;
		case ColorOrder::ORDER_RBG:
			std::swap(color.green, color.blue);
			break;
		case ColorOrder::ORDER_GRB:
			std::swap(color.red, color.green);
			break;
		case ColorOrder::ORDER_GBR:
			std::swap(color.red, color.green);
			std::swap(color.green, color.blue);
			break;
		case ColorOrder::ORDER_BRG:
			std::swap(color.red, color.blue);
			std::swap(color.green, color.blue);
			break;
		}
	}
}

void Hyperion::writeToLeds(hyperion::Components sourceComponent)
{
	if (_ledDeviceWrapper->isOn())
	{
		// Smoothing is disabled
		if (!_deviceSmooth->enabled())
		{
				emit ledDeviceData(_ledBuffer);
		}
		else
		{
			// device is enabled, feed smoothing in pause mode to maintain a smooth transition back to smooth mode
			if (!_deviceSmooth->pause())
			{
				_deviceSmooth->updateLedValues(_ledBuffer);
			}
		}
	}
	else if (_twilightBootComplete && _twilightEnabled && !_isTwilightNight
		&& (sourceComponent == hyperion::COMP_COLOR || sourceComponent == hyperion::COMP_EFFECT))
	{
		// Daytime with twilight: auto-enable device on user interaction (color/effect change)
		_ledDeviceWrapper->handleComponentState(hyperion::COMP_LEDDEVICE, true);
	}
}

void Hyperion::refreshUpdate() const
{
	qCDebug(instance_flow) << "A forced refresh update is requested, waiting for latch time of" << _ledDeviceWrapper->getLatchTime() << "ms before processing.";
	wait(_ledDeviceWrapper->getLatchTime());
	QTimer::singleShot(0, this, &Hyperion::handleForceUpdate);
}

void Hyperion::update()
{
	_totalImagesProcessed++;
	TRACK_SCOPE_SUBCOMPONENT_CATEGORY(instance_update) << "Update output" << (_isUpdatePending.load() ? "will be skipped as another update is pending." : "will be executed.");

	// If an update processing is NOT already scheduled, schedule one.
	if (!_isUpdatePending.exchange(true))
	{
		QTimer::singleShot(0, this, &Hyperion::handleUpdate);
	}
	else
	{
		_imagesSkipped++;
	}

	return; // Return immediately
}

void Hyperion::handleForceUpdate()
{
	_isUpdateQueued.store(true);
	update();
}

void Hyperion::handleUpdate()
{
	do
	{
		_isUpdateQueued.store(false);
		processUpdate();
	} while (_isUpdateQueued.load());

	_isUpdatePending.store(false);
}

void Hyperion::processUpdate()
{
	// Obtain the current priority channel
	const PriorityMuxer::InputInfo& priorityInfo = _muxer->getInputInfo(_muxer->getCurrentPriority());

	// copy image & process OR copy ledColors from muxer
	const Image<ColorRgb>& image = priorityInfo.image;

	QVector<ColorRgb> ledColors;

	if (!image.isNull())
	{
		TRACK_SCOPE_SUBCOMPONENT_CATEGORY(instance_update) << "Process update using image with id" << image.id() << "and resolution" << image.width() << "x" << image.height();
		emit currentImage(image);  // Emit the image signal at the controlled rate
		ledColors = _imageProcessor->process(image);
	}
	else
	{
		ledColors = priorityInfo.ledColors;
		if (ledColors.empty())
		{		
			TRACK_SCOPE_SUBCOMPONENT_CATEGORY(instance_update) << "Empty image and no LED colors provided - skip update";
			return;
		}
	}

	emit rawLedColors(ledColors);
	applyBlacklist(ledColors);

	// Start transformations
	_raw2ledAdjustment->applyAdjustment(ledColors);

	applyColorOrder(ledColors);

	// Copy elements from ledColors to _ledBuffer up to the size of _ledBuffer
	std::copy_n(ledColors.begin(), std::min<qsizetype>(_ledBuffer.size(), ledColors.size()), _ledBuffer.begin());

	writeToLeds(priorityInfo.componentId);
}

void Hyperion::resetImagesProcessedStatistics()
{
	_totalImagesProcessed.store(0);
	_imagesSkipped.store(0);
	if (_statisticsTimer)
	{
		_statisticsTimer->start();
	}
}

void Hyperion::reportImagesProcessedStatistics()
{
	int total = _totalImagesProcessed.exchange(0);
	int skipped = _imagesSkipped.exchange(0);

	if (total > 0)
	{
		double interval_s = _statisticsTimer->interval() / 1000.0;
		if (interval_s > 0)
		{
			Debug(_log, "Processed %d images in the last %d seconds. Images processed per second: %.2f", total, static_cast<int>(interval_s), total / interval_s);
		}

		double percentage = (double)skipped / total * 100.0;
		if (percentage > DEFAULT_SKIPPEDUPDATES_LOWERBOUND)
		{
			double actual_updates_ps = (total - skipped) / interval_s;
			Warning(_log, "Skipped %d of %d images (%.2f %%) in the last %d seconds. Actual images processed per second: %.2f", skipped, total, percentage, static_cast<int>(interval_s), actual_updates_ps);
		}
	}
}
