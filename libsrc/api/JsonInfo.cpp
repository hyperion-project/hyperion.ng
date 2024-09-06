#include <db/DBConfigManager.h>
#include <api/JsonInfo.h>
#include <api/API.h>

#include <utils/ColorSys.h>
#include <hyperion/GrabberWrapper.h>
#include <leddevice/LedDeviceWrapper.h>
#include <utils/SysInfo.h>
#include <hyperion/AuthManager.h>
#include <QCoreApplication>
#include <QApplication>

#include <HyperionConfig.h> // Required to determine the cmake options

#include <hyperion/GrabberWrapper.h>
#include <grabber/GrabberConfig.h>


QJsonArray JsonInfo::getAdjustmentInfo(const Hyperion* hyperion, Logger* log)
{
	QJsonArray adjustmentArray;
	for (const QString &adjustmentId : hyperion->getAdjustmentIds())
	{
		const ColorAdjustment *colorAdjustment = hyperion->getAdjustment(adjustmentId);
		if (colorAdjustment == nullptr)
		{
			Error(log, "Incorrect color adjustment id: %s", QSTRING_CSTR(adjustmentId));
			continue;
		}

		QJsonObject adjustment;
		adjustment["id"] = adjustmentId;

		QJsonArray whiteAdjust;
		whiteAdjust.append(colorAdjustment->_rgbWhiteAdjustment.getAdjustmentR());
		whiteAdjust.append(colorAdjustment->_rgbWhiteAdjustment.getAdjustmentG());
		whiteAdjust.append(colorAdjustment->_rgbWhiteAdjustment.getAdjustmentB());
		adjustment.insert("white", whiteAdjust);

		QJsonArray redAdjust;
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentR());
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentG());
		redAdjust.append(colorAdjustment->_rgbRedAdjustment.getAdjustmentB());
		adjustment.insert("red", redAdjust);

		QJsonArray greenAdjust;
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentR());
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentG());
		greenAdjust.append(colorAdjustment->_rgbGreenAdjustment.getAdjustmentB());
		adjustment.insert("green", greenAdjust);

		QJsonArray blueAdjust;
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentR());
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentG());
		blueAdjust.append(colorAdjustment->_rgbBlueAdjustment.getAdjustmentB());
		adjustment.insert("blue", blueAdjust);

		QJsonArray cyanAdjust;
		cyanAdjust.append(colorAdjustment->_rgbCyanAdjustment.getAdjustmentR());
		cyanAdjust.append(colorAdjustment->_rgbCyanAdjustment.getAdjustmentG());
		cyanAdjust.append(colorAdjustment->_rgbCyanAdjustment.getAdjustmentB());
		adjustment.insert("cyan", cyanAdjust);

		QJsonArray magentaAdjust;
		magentaAdjust.append(colorAdjustment->_rgbMagentaAdjustment.getAdjustmentR());
		magentaAdjust.append(colorAdjustment->_rgbMagentaAdjustment.getAdjustmentG());
		magentaAdjust.append(colorAdjustment->_rgbMagentaAdjustment.getAdjustmentB());
		adjustment.insert("magenta", magentaAdjust);

		QJsonArray yellowAdjust;
		yellowAdjust.append(colorAdjustment->_rgbYellowAdjustment.getAdjustmentR());
		yellowAdjust.append(colorAdjustment->_rgbYellowAdjustment.getAdjustmentG());
		yellowAdjust.append(colorAdjustment->_rgbYellowAdjustment.getAdjustmentB());
		adjustment.insert("yellow", yellowAdjust);

		adjustment["backlightThreshold"] = colorAdjustment->_rgbTransform.getBacklightThreshold();
		adjustment["backlightColored"] = colorAdjustment->_rgbTransform.getBacklightColored();
		adjustment["brightness"] = colorAdjustment->_rgbTransform.getBrightness();
		adjustment["brightnessCompensation"] = colorAdjustment->_rgbTransform.getBrightnessCompensation();
		adjustment["gammaRed"] = colorAdjustment->_rgbTransform.getGammaR();
		adjustment["gammaGreen"] = colorAdjustment->_rgbTransform.getGammaG();
		adjustment["gammaBlue"] = colorAdjustment->_rgbTransform.getGammaB();

		adjustment["saturationGain"] = colorAdjustment->_okhsvTransform.getSaturationGain();
		adjustment["brightnessGain"] = colorAdjustment->_okhsvTransform.getBrightnessGain();

		adjustmentArray.append(adjustment);
	}
	return adjustmentArray;
}

QJsonArray JsonInfo::getPrioritiestInfo(const Hyperion* hyperion)
{
	return getPrioritiestInfo(hyperion->getCurrentPriority(), hyperion->getPriorityInfo());
}

QJsonArray JsonInfo::getPrioritiestInfo(int currentPriority, const PriorityMuxer::InputsMap& activeInputs)
{
	QJsonArray priorities;
	int64_t now = QDateTime::currentMSecsSinceEpoch();

	QList<int> activePriorities = activeInputs.keys();
	activePriorities.removeAll(PriorityMuxer::LOWEST_PRIORITY);

	for(int priority : std::as_const(activePriorities))
	{
		const PriorityMuxer::InputInfo priorityInfo = activeInputs.value(priority);

		QJsonObject item;
		item["priority"] = priority;

		if (priorityInfo.timeoutTime_ms > 0 )
		{
			item["duration_ms"] = int(priorityInfo.timeoutTime_ms - now);
		}

		// owner has optional informations to the component
		if (!priorityInfo.owner.isEmpty())
		{
			item["owner"] = priorityInfo.owner;
		}

		item["componentId"] = QString(hyperion::componentToIdString(priorityInfo.componentId));
		item["origin"] = priorityInfo.origin;
		item["active"] = (priorityInfo.timeoutTime_ms >= -1);
		item["visible"] = (priority == currentPriority);

		if (priorityInfo.componentId == hyperion::COMP_COLOR && !priorityInfo.ledColors.empty())
		{
			QJsonObject LEDcolor;

			// add RGB Value to Array
			QJsonArray RGBValue;
			RGBValue.append(priorityInfo.ledColors.begin()->red);
			RGBValue.append(priorityInfo.ledColors.begin()->green);
			RGBValue.append(priorityInfo.ledColors.begin()->blue);
			LEDcolor.insert("RGB", RGBValue);

			uint16_t Hue;
			float Saturation;
			float Luminace;

			// add HSL Value to Array
			QJsonArray HSLValue;
			ColorSys::rgb2hsl(priorityInfo.ledColors.begin()->red,
							  priorityInfo.ledColors.begin()->green,
							  priorityInfo.ledColors.begin()->blue,
							  Hue, Saturation, Luminace);

			HSLValue.append(static_cast<double>(Hue));
			HSLValue.append(static_cast<double>(Saturation));
			HSLValue.append(static_cast<double>(Luminace));
			LEDcolor.insert("HSL", HSLValue);

			item["value"] = LEDcolor;
		}

		(priority == currentPriority)
				? priorities.prepend(item)
				: priorities.append(item);
	}
	return priorities;
}

QJsonArray JsonInfo::getEffects(const Hyperion* hyperion)
{
	QJsonArray effects;
#if defined(ENABLE_EFFECTENGINE)
	// collect effect info

	const std::list<EffectDefinition> &effectsDefinitions = hyperion->getEffects();
	for (const EffectDefinition &effectDefinition : effectsDefinitions)
	{
		QJsonObject effect;
		effect["name"] = effectDefinition.name;
		effect["file"] = effectDefinition.file;
		effect["script"] = effectDefinition.script;
		effect["args"] = effectDefinition.args;
		effects.append(effect);
	}
#endif
	return effects;
}

QJsonObject JsonInfo::getAvailableLedDevices()
{
	// get available led devices
	QJsonObject ledDevices;
	QJsonArray availableLedDevices;
	for (const auto& dev : LedDeviceWrapper::getDeviceMap())
	{
		availableLedDevices.append(dev.first);
	}

	ledDevices["available"] = availableLedDevices;

	return ledDevices;
}

QJsonArray JsonInfo::getAvailableScreenGrabbers()
{
	QJsonArray availableScreenGrabbers;
	for (const auto& grabber : GrabberWrapper::availableGrabbers(GrabberTypeFilter::SCREEN))
	{
		availableScreenGrabbers.append(grabber);
	}
	return availableScreenGrabbers;
}

QJsonArray JsonInfo::getAvailableVideoGrabbers()
{
	QJsonArray availableVideoGrabbers;
	for (const auto& grabber : GrabberWrapper::availableGrabbers(GrabberTypeFilter::VIDEO))
	{
		availableVideoGrabbers.append(grabber);
	}
	return availableVideoGrabbers;
}
QJsonArray JsonInfo::getAvailableAudioGrabbers()
{
	QJsonArray availableAudioGrabbers;
	for (const auto& grabber : GrabberWrapper::availableGrabbers(GrabberTypeFilter::AUDIO))
	{
		availableAudioGrabbers.append(grabber);
	}
	return availableAudioGrabbers;
}

QJsonObject JsonInfo::getGrabbers(const Hyperion* hyperion)
{
	QJsonObject grabbers;
	// SCREEN
	QJsonObject screenGrabbers;
	if (GrabberWrapper::getInstance() != nullptr)
	{
		const QStringList activeGrabbers = GrabberWrapper::getInstance()->getActive(hyperion->getInstanceIndex(), GrabberTypeFilter::SCREEN);
		QJsonArray activeGrabberNames;
		for (const auto& grabberName : activeGrabbers)
		{
			activeGrabberNames.append(grabberName);
		}

		screenGrabbers["active"] = activeGrabberNames;
	}
	screenGrabbers["available"] = getAvailableScreenGrabbers();

	// VIDEO
	QJsonObject videoGrabbers;
	if (GrabberWrapper::getInstance() != nullptr)
	{
		const QStringList activeGrabbers = GrabberWrapper::getInstance()->getActive(hyperion->getInstanceIndex(), GrabberTypeFilter::VIDEO);
		QJsonArray activeGrabberNames;
		for (const auto& grabberName : activeGrabbers)
		{
			activeGrabberNames.append(grabberName);
		}

		videoGrabbers["active"] = activeGrabberNames;
	}
	videoGrabbers["available"] = getAvailableVideoGrabbers();

	// AUDIO
	QJsonObject audioGrabbers;
	if (GrabberWrapper::getInstance() != nullptr)
	{
		const QStringList activeGrabbers = GrabberWrapper::getInstance()->getActive(hyperion->getInstanceIndex(), GrabberTypeFilter::AUDIO);

		QJsonArray activeGrabberNames;
		for (const auto& grabberName : activeGrabbers)
		{
			activeGrabberNames.append(grabberName);
		}

		audioGrabbers["active"] = activeGrabberNames;
	}
	audioGrabbers["available"] = getAvailableAudioGrabbers() ;

	grabbers.insert("screen", screenGrabbers);
	grabbers.insert("video", videoGrabbers);
	grabbers.insert("audio", audioGrabbers);

	return grabbers;
}

QJsonObject JsonInfo::getCecInfo()
{
	QJsonObject cecInfo;
#if defined(ENABLE_CEC)
	cecInfo["enabled"] = true;
#else
	cecInfo["enabled"] = false;
#endif
	return cecInfo;
}

QJsonArray JsonInfo::getServices()
{
	// get available services
	QJsonArray services;

#if defined(ENABLE_BOBLIGHT_SERVER)
	services.append("boblight");
#endif

#if defined(ENABLE_CEC)
	services.append("cec");
#endif

#if defined(ENABLE_EFFECTENGINE)
	services.append("effectengine");
#endif

#if defined(ENABLE_FORWARDER)
	services.append("forwarder");
#endif

#if defined(ENABLE_FLATBUF_SERVER)
	services.append("flatbuffer");
#endif

#if defined(ENABLE_PROTOBUF_SERVER)
	services.append("protobuffer");
#endif

#if defined(ENABLE_MDNS)
	services.append("mDNS");
#endif
	services.append("SSDP");

	if (!getAvailableScreenGrabbers().isEmpty() || !getAvailableVideoGrabbers().isEmpty() || services.contains("flatbuffer") || services.contains("protobuffer"))
	{
		services.append("borderdetection");
	}
	return services;
}

QJsonArray JsonInfo::getComponents(const Hyperion* hyperion)
{
	// get available components
	QJsonArray component;
	std::map<hyperion::Components, bool> components = hyperion->getComponentRegister()->getRegister();
	for (auto comp : components)
	{
		QJsonObject item;
		item["name"] = QString::fromStdString(hyperion::componentToIdString(comp.first));
		item["enabled"] = comp.second;

		component.append(item);
	}
	return component;
}

QJsonArray JsonInfo::getInstanceInfo()
{
	QJsonArray instanceInfo;
	for (const auto &entry : HyperionIManager::getInstance()->getInstanceData())
	{
		QJsonObject obj;
		obj.insert("friendly_name", entry["friendly_name"].toString());
		obj.insert("instance", entry["instance"].toInt());
		obj.insert("running", entry["running"].toBool());
		instanceInfo.append(obj);
	}
	return instanceInfo;
}

QJsonArray JsonInfo::getTransformationInfo(const Hyperion* hyperion)
{
	// TRANSFORM INFORMATION (DEFAULT VALUES)
	QJsonArray transformArray;
	for (const QString &transformId : hyperion->getAdjustmentIds())
	{
		QJsonObject transform;
		QJsonArray blacklevel;
		QJsonArray whitelevel;
		QJsonArray gamma;
		QJsonArray threshold;

		transform["id"] = transformId;
		transform["saturationGain"] = 1.0;
		transform["brightnessGain"] = 1.0;
		transform["saturationLGain"] = 1.0;
		transform["luminanceGain"] = 1.0;
		transform["luminanceMinimum"] = 0.0;

		for (int i = 0; i < 3; i++)
		{
			blacklevel.append(0.0);
			whitelevel.append(1.0);
			gamma.append(2.50);
			threshold.append(0.0);
		}

		transform.insert("blacklevel", blacklevel);
		transform.insert("whitelevel", whitelevel);
		transform.insert("gamma", gamma);
		transform.insert("threshold", threshold);

		transformArray.append(transform);
	}
	return transformArray;
}

QJsonArray JsonInfo::getActiveEffects(const Hyperion* hyperion)
{
	// ACTIVE EFFECT INFO
	QJsonArray activeEffects;
#if defined(ENABLE_EFFECTENGINE)
	for (const ActiveEffectDefinition &activeEffectDefinition : hyperion->getActiveEffects())
	{
		if (activeEffectDefinition.priority != PriorityMuxer::LOWEST_PRIORITY - 1)
		{
			QJsonObject activeEffect;
			activeEffect["script"] = activeEffectDefinition.script;
			activeEffect["name"] = activeEffectDefinition.name;
			activeEffect["priority"] = activeEffectDefinition.priority;
			activeEffect["timeout"] = activeEffectDefinition.timeout;
			activeEffect["args"] = activeEffectDefinition.args;
			activeEffects.append(activeEffect);
		}
	}
#endif
	return activeEffects;
}

QJsonArray JsonInfo::getActiveColors(const Hyperion* hyperion)
{
	// ACTIVE STATIC LED COLOR
	QJsonArray activeLedColors;
	const Hyperion::InputInfo &priorityInfo = hyperion->getPriorityInfo(hyperion->getCurrentPriority());
	if (priorityInfo.componentId == hyperion::COMP_COLOR && !priorityInfo.ledColors.empty())
	{
		// check if LED Color not Black (0,0,0)
		if ((priorityInfo.ledColors.begin()->red +
			 priorityInfo.ledColors.begin()->green +
			 priorityInfo.ledColors.begin()->blue !=
			 0))
		{
			QJsonObject LEDcolor;

			// add RGB Value to Array
			QJsonArray RGBValue;
			RGBValue.append(priorityInfo.ledColors.begin()->red);
			RGBValue.append(priorityInfo.ledColors.begin()->green);
			RGBValue.append(priorityInfo.ledColors.begin()->blue);
			LEDcolor.insert("RGB Value", RGBValue);

			uint16_t Hue;
			float Saturation;
			float Luminace;

			// add HSL Value to Array
			QJsonArray HSLValue;
			ColorSys::rgb2hsl(priorityInfo.ledColors.begin()->red,
							  priorityInfo.ledColors.begin()->green,
							  priorityInfo.ledColors.begin()->blue,
							  Hue, Saturation, Luminace);

			HSLValue.append(static_cast<double>(Hue));
			HSLValue.append(static_cast<double>(Saturation));
			HSLValue.append(static_cast<double>(Luminace));
			LEDcolor.insert("HSL Value", HSLValue);

			activeLedColors.append(LEDcolor);
		}
	}
	return activeLedColors;
}

QJsonObject JsonInfo::getSystemInfo(const Hyperion* hyperion)
{
	QJsonObject info;

	SysInfo::HyperionSysInfo data = SysInfo::get();
	QJsonObject systemInfo;
	systemInfo["kernelType"] = data.kernelType;
	systemInfo["kernelVersion"] = data.kernelVersion;
	systemInfo["architecture"] = data.architecture;
	systemInfo["cpuModelName"] = data.cpuModelName;
	systemInfo["cpuModelType"] = data.cpuModelType;
	systemInfo["cpuHardware"] = data.cpuHardware;
	systemInfo["cpuRevision"] = data.cpuRevision;
	systemInfo["wordSize"] = data.wordSize;
	systemInfo["productType"] = data.productType;
	systemInfo["productVersion"] = data.productVersion;
	systemInfo["prettyName"] = data.prettyName;
	systemInfo["hostName"] = data.hostName;
	systemInfo["domainName"] = data.domainName;
	systemInfo["isUserAdmin"] = data.isUserAdmin;
	systemInfo["qtVersion"] = data.qtVersion;
#if defined(ENABLE_EFFECTENGINE)
	systemInfo["pyVersion"] = data.pyVersion;
#endif
	info["system"] = systemInfo;

	QJsonObject hyperionInfo;
	hyperionInfo["version"] = QString(HYPERION_VERSION);
	hyperionInfo["build"] = QString(HYPERION_BUILD_ID);
	hyperionInfo["gitremote"] = QString(HYPERION_GIT_REMOTE);
	hyperionInfo["time"] = QString(__DATE__ " " __TIME__);
	hyperionInfo["id"] = AuthManager::getInstance()->getID();
	hyperionInfo["configDatabaseFile"] = DBManager::getFileInfo().absoluteFilePath();
	hyperionInfo["readOnlyMode"] = DBManager::isReadOnly();

	QCoreApplication* app = QCoreApplication::instance();
	hyperionInfo["isGuiMode"] = qobject_cast<QApplication*>(app) != nullptr;

	info["hyperion"] = hyperionInfo;

	return info;
}

QJsonObject JsonInfo::discoverSources(const QString& sourceType, const QJsonObject& params)
{
	QJsonObject inputSourcesDiscovered;
	inputSourcesDiscovered.insert("sourceType", sourceType);

	if (sourceType == "video") {
		QJsonArray videoInputs = discoverVideoInputs(params);
		inputSourcesDiscovered["video_sources"] = videoInputs;
	} else if (sourceType == "audio") {
		QJsonArray audioInputs = discoverAudioInputs(params);
		inputSourcesDiscovered["audio_sources"] = audioInputs;
	} else if (sourceType == "screen") {
		QJsonArray screenInputs = discoverScreenInputs(params);
		inputSourcesDiscovered["video_sources"] = screenInputs;
	}

	return inputSourcesDiscovered;
}

template<typename GrabberType>
void JsonInfo::discoverGrabber(QJsonArray& inputs, const QJsonObject& params) const
{
	GrabberType grabber;
	QJsonValue discoveryResult = grabber.discover(params);

	if (discoveryResult.isArray())
	{
		inputs = discoveryResult.toArray();
	}
	else
	{
		if (!discoveryResult.toObject().isEmpty())
		{
			inputs.append(discoveryResult);
		}
	}
}

QJsonArray JsonInfo::discoverVideoInputs(const QJsonObject& params) const
{
	QJsonArray videoInputs;

#ifdef ENABLE_V4L2
	discoverGrabber<V4L2Grabber>(videoInputs, params);
#endif

#ifdef ENABLE_MF
	discoverGrabber<MFGrabber>(videoInputs, params);
#endif

	return videoInputs;
}

QJsonArray JsonInfo::discoverAudioInputs(const QJsonObject& params) const
{
	QJsonArray audioInputs;

#ifdef ENABLE_AUDIO
#ifdef WIN32
	discoverGrabber<AudioGrabberWindows>(audioInputs, params);
#endif

#ifdef __linux__audioInputs
	discoverGrabber<AudioGrabberLinux>(audioInputs, params);
#endif

#endif

	return audioInputs;
}

QJsonArray JsonInfo::discoverScreenInputs(const QJsonObject& params) const
{
	QJsonArray screenInputs;

#ifdef ENABLE_QT
	discoverGrabber<QtGrabber>(screenInputs, params);
#endif

#ifdef ENABLE_DX
	discoverGrabber<DirectXGrabber>(screenInputs, params);
#endif

#ifdef ENABLE_DDA
	discoverGrabber<DDAGrabber>(screenInputs, params);
#endif

#ifdef ENABLE_X11
	discoverGrabber<X11Grabber>(screenInputs, params);
#endif

#ifdef ENABLE_XCB
	discoverGrabber<XcbGrabber>(screenInputs, params);
#endif

#if defined(ENABLE_FB) && !defined(ENABLE_AMLOGIC)
	discoverGrabber<FramebufferFrameGrabber>(screenInputs, params);
#endif

#ifdef ENABLE_DISPMANX
	discoverGrabber<DispmanxFrameGrabber>(screenInputs, params);
#endif

#ifdef ENABLE_AMLOGIC
	discoverGrabber<AmlogicGrabber>(screenInputs, params);
#endif

#ifdef ENABLE_OSX
	discoverGrabber<OsxFrameGrabber>(screenInputs, params);
#endif

	return screenInputs;
}

QJsonObject JsonInfo::getConfiguration(const QList<quint8>& instancesfilter, const QStringList& instanceFilteredTypes, const QStringList& globalFilterTypes )
{
	DBConfigManager configManager;
	return configManager.getConfiguration(instancesfilter, instanceFilteredTypes, globalFilterTypes );
}
