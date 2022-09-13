// proj incl
#include <api/JsonCB.h>

// hyperion
#include <hyperion/Hyperion.h>

// HyperionIManager
#include <hyperion/HyperionIManager.h>
// components

#include <hyperion/ComponentRegister.h>
// priorityMuxer

#include <hyperion/PriorityMuxer.h>

// utils
#include <utils/ColorSys.h>

// qt
#include <QDateTime>
#include <QVariant>

// Image to led map helper
#include <hyperion/ImageProcessor.h>

using namespace hyperion;

JsonCB::JsonCB(QObject* parent)
	: QObject(parent)
	, _hyperion(nullptr)
	, _componentRegister(nullptr)
	, _prioMuxer(nullptr)
{
	_availableCommands << "components-update" << "priorities-update" << "imageToLedMapping-update"
	<< "adjustment-update" << "videomode-update" << "settings-update" << "leds-update" << "instance-update" << "token-update";

	#if defined(ENABLE_EFFECTENGINE)
	_availableCommands << "effects-update";
	#endif

	qRegisterMetaType<PriorityMuxer::InputsMap>("InputsMap");
}

bool JsonCB::subscribeFor(const QString& type, bool unsubscribe)
{
	if(!_availableCommands.contains(type))
		return false;

	if(unsubscribe)
		_subscribedCommands.removeAll(type);
	else
		_subscribedCommands << type;

	if(type == "components-update")
	{
		if(unsubscribe)
			disconnect(_componentRegister, &ComponentRegister::updatedComponentState, this, &JsonCB::handleComponentState);
		else
			connect(_componentRegister, &ComponentRegister::updatedComponentState, this, &JsonCB::handleComponentState, Qt::UniqueConnection);
	}

	if(type == "priorities-update")
	{
		if (unsubscribe)
			disconnect(_prioMuxer, &PriorityMuxer::prioritiesChanged, this, &JsonCB::handlePriorityUpdate);
		else
			connect(_prioMuxer, &PriorityMuxer::prioritiesChanged, this, &JsonCB::handlePriorityUpdate, Qt::UniqueConnection);
	}

	if(type == "imageToLedMapping-update")
	{
		if(unsubscribe)
			disconnect(_hyperion, &Hyperion::imageToLedsMappingChanged, this, &JsonCB::handleImageToLedsMappingChange);
		else
			connect(_hyperion, &Hyperion::imageToLedsMappingChanged, this, &JsonCB::handleImageToLedsMappingChange, Qt::UniqueConnection);
	}

	if(type == "adjustment-update")
	{
		if(unsubscribe)
			disconnect(_hyperion, &Hyperion::adjustmentChanged, this, &JsonCB::handleAdjustmentChange);
		else
			connect(_hyperion, &Hyperion::adjustmentChanged, this, &JsonCB::handleAdjustmentChange, Qt::UniqueConnection);
	}

	if(type == "videomode-update")
	{
		if(unsubscribe)
			disconnect(_hyperion, &Hyperion::newVideoMode, this, &JsonCB::handleVideoModeChange);
		else
			connect(_hyperion, &Hyperion::newVideoMode, this, &JsonCB::handleVideoModeChange, Qt::UniqueConnection);
	}

#if defined(ENABLE_EFFECTENGINE)
	if(type == "effects-update")
	{
		if(unsubscribe)
			disconnect(_hyperion, &Hyperion::effectListUpdated, this, &JsonCB::handleEffectListChange);
		else
			connect(_hyperion, &Hyperion::effectListUpdated, this, &JsonCB::handleEffectListChange, Qt::UniqueConnection);
	}
#endif

	if(type == "settings-update")
	{
		if(unsubscribe)
			disconnect(_hyperion, &Hyperion::settingsChanged, this, &JsonCB::handleSettingsChange);
		else
			connect(_hyperion, &Hyperion::settingsChanged, this, &JsonCB::handleSettingsChange, Qt::UniqueConnection);
	}

	if(type == "leds-update")
	{
		if(unsubscribe)
			disconnect(_hyperion, &Hyperion::settingsChanged, this, &JsonCB::handleLedsConfigChange);
		else
			connect(_hyperion, &Hyperion::settingsChanged, this, &JsonCB::handleLedsConfigChange, Qt::UniqueConnection);
	}


	if(type == "instance-update")
	{
		if(unsubscribe)
			disconnect(HyperionIManager::getInstance(), &HyperionIManager::change, this, &JsonCB::handleInstanceChange);
		else
			connect(HyperionIManager::getInstance(), &HyperionIManager::change, this, &JsonCB::handleInstanceChange, Qt::UniqueConnection);
	}

	if (type == "token-update")
	{
		if (unsubscribe)
			disconnect(AuthManager::getInstance(), &AuthManager::tokenChange, this, &JsonCB::handleTokenChange);
		else
			connect(AuthManager::getInstance(), &AuthManager::tokenChange, this, &JsonCB::handleTokenChange, Qt::UniqueConnection);
	}

	return true;
}

void JsonCB::resetSubscriptions()
{
	for(const auto & entry : getSubscribedCommands())
	{
		subscribeFor(entry, true);
	}
}

void JsonCB::setSubscriptionsTo(Hyperion* hyperion)
{
	assert(hyperion);
	//std::cout << "JsonCB::setSubscriptions for instance [" << static_cast<int>(hyperion->getInstanceIndex()) << "] " << std::endl;

	// get current subs
	QStringList currSubs(getSubscribedCommands());

	// stop subs
	resetSubscriptions();

	// update pointer
	_hyperion = hyperion;
	_componentRegister = _hyperion->getComponentRegister();
	_prioMuxer = _hyperion->getMuxerInstance();

	// re-apply subs
	for(const auto & entry : currSubs)
	{
		subscribeFor(entry);
	}
}

void JsonCB::doCallback(const QString& cmd, const QVariant& data)
{
	QJsonObject obj;
	obj["command"] = cmd;

	if (data.userType() == QMetaType::QJsonArray)
		obj["data"] = data.toJsonArray();
	else
		obj["data"] = data.toJsonObject();

	//std::cout << "JsonCB::doCallback | [" << static_cast<int>(_hyperion->getInstanceIndex()) << "] Send: [" << QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString() << "]" << std::endl;

	emit newCallback(obj);
}

void JsonCB::handleComponentState(hyperion::Components comp, bool state)
{
	QJsonObject data;
	data["name"] = componentToIdString(comp);
	data["enabled"] = state;

	doCallback("components-update", QVariant(data));
}

void JsonCB::handlePriorityUpdate(int currentPriority, const PriorityMuxer::InputsMap& activeInputs)
{
	QJsonObject data;
	QJsonArray priorities;
	uint64_t now = QDateTime::currentMSecsSinceEpoch();
	QList<int> activePriorities = activeInputs.keys();

	activePriorities.removeAll(PriorityMuxer::LOWEST_PRIORITY);

	for (int priority : qAsConst(activePriorities)) {

		const Hyperion::InputInfo& priorityInfo = activeInputs[priority];

		QJsonObject item;
		item["priority"] = priority;

		if (priorityInfo.timeoutTime_ms > 0 )
		{
			item["duration_ms"] = int(priorityInfo.timeoutTime_ms - now);
		}

		// owner has optional informations to the component
		if(!priorityInfo.owner.isEmpty())
		{
			item["owner"] = priorityInfo.owner;
		}

		item["componentId"] = QString(hyperion::componentToIdString(priorityInfo.componentId));
		item["origin"] = priorityInfo.origin;
		item["active"] = (priorityInfo.timeoutTime_ms >= -1);
		item["visible"] = (priority == currentPriority);

		if(priorityInfo.componentId == hyperion::COMP_COLOR && !priorityInfo.ledColors.empty())
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

			HSLValue.append(Hue);
			HSLValue.append(Saturation);
			HSLValue.append(Luminace);
			LEDcolor.insert("HSL", HSLValue);

			item["value"] = LEDcolor;
		}
		priorities.append(item);
	}

	data["priorities"] = priorities;
	data["priorities_autoselect"] = _hyperion->sourceAutoSelectEnabled();

	doCallback("priorities-update", QVariant(data));
}

void JsonCB::handleImageToLedsMappingChange(int mappingType)
{
	QJsonObject data;
	data["imageToLedMappingType"] = ImageProcessor::mappingTypeToStr(mappingType);

	doCallback("imageToLedMapping-update", QVariant(data));
}

void JsonCB::handleAdjustmentChange()
{
	QJsonArray adjustmentArray;
	for (const QString& adjustmentId : _hyperion->getAdjustmentIds())
	{
		const ColorAdjustment * colorAdjustment = _hyperion->getAdjustment(adjustmentId);
		if (colorAdjustment == nullptr)
		{
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
		adjustment["backlightColored"]   = colorAdjustment->_rgbTransform.getBacklightColored();
		adjustment["brightness"] = colorAdjustment->_rgbTransform.getBrightness();
		adjustment["brightnessCompensation"] = colorAdjustment->_rgbTransform.getBrightnessCompensation();
		adjustment["gammaRed"]   = colorAdjustment->_rgbTransform.getGammaR();
		adjustment["gammaGreen"] = colorAdjustment->_rgbTransform.getGammaG();
		adjustment["gammaBlue"]  = colorAdjustment->_rgbTransform.getGammaB();

		adjustmentArray.append(adjustment);
	}

	doCallback("adjustment-update", QVariant(adjustmentArray));
}

void JsonCB::handleVideoModeChange(VideoMode mode)
{
	QJsonObject data;
	data["videomode"] = QString(videoMode2String(mode));
	doCallback("videomode-update", QVariant(data));
}

#if defined(ENABLE_EFFECTENGINE)
void JsonCB::handleEffectListChange()
{
	QJsonArray effectList;
	QJsonObject effects;
	const std::list<EffectDefinition> & effectsDefinitions = _hyperion->getEffects();
	for (const EffectDefinition & effectDefinition : effectsDefinitions)
	{
		QJsonObject effect;
		effect["name"] = effectDefinition.name;
		effect["file"] = effectDefinition.file;
		effect["script"] = effectDefinition.script;
		effect["args"] = effectDefinition.args;
		effectList.append(effect);
	};
	effects["effects"] = effectList;
	doCallback("effects-update", QVariant(effects));
}
#endif

void JsonCB::handleSettingsChange(settings::type type, const QJsonDocument& data)
{
	QJsonObject dat;
	if(data.isObject())
		dat[typeToString(type)] = data.object();
	else
		dat[typeToString(type)] = data.array();

	doCallback("settings-update", QVariant(dat));
}

void JsonCB::handleLedsConfigChange(settings::type type, const QJsonDocument& data)
{
	if(type == settings::LEDS)
	{
		QJsonObject dat;
		dat[typeToString(type)] = data.array();
		doCallback("leds-update", QVariant(dat));
	}
}

void JsonCB::handleInstanceChange()
{
	QJsonArray arr;

	for(const auto & entry : HyperionIManager::getInstance()->getInstanceData())
	{
		QJsonObject obj;
		obj.insert("friendly_name", entry["friendly_name"].toString());
		obj.insert("instance", entry["instance"].toInt());
		//obj.insert("last_use", entry["last_use"].toString());
		obj.insert("running", entry["running"].toBool());
		arr.append(obj);
	}
	doCallback("instance-update", QVariant(arr));
}

void JsonCB::handleTokenChange(const QVector<AuthManager::AuthDefinition> &def)
{
	QJsonArray arr;
	for (const auto &entry : def)
	{
		QJsonObject sub;
		sub["comment"] = entry.comment;
		sub["id"] = entry.id;
		sub["last_use"] = entry.lastUse;
		arr.push_back(sub);
	}
	doCallback("token-update", QVariant(arr));
}
