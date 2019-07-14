// proj incl
#include <api/JsonCB.h>

// hyperion
#include <hyperion/Hyperion.h>

// HyperionIManager
#include <hyperion/HyperionIManager.h>
// components

#include <hyperion/ComponentRegister.h>
// bonjour wrapper

#include <bonjour/bonjourbrowserwrapper.h>
// priorityMuxer

#include <hyperion/PriorityMuxer.h>

// utils
#include <utils/ColorSys.h>

// qt
#include <QDateTime>

// Image to led map helper
#include <hyperion/ImageProcessor.h>

using namespace hyperion;

JsonCB::JsonCB(Hyperion* hyperion, QObject* parent)
	: QObject(parent)
	, _hyperion(hyperion)
	, _componentRegister(& _hyperion->getComponentRegister())
	, _bonjour(BonjourBrowserWrapper::getInstance())
	, _prioMuxer(_hyperion->getMuxerInstance())
{
	_availableCommands << "components-update" << "sessions-update" << "priorities-update" << "imageToLedMapping-update"
	<< "adjustment-update" << "videomode-update" << "effects-update" << "settings-update" << "instance-update";
}

bool JsonCB::subscribeFor(const QString& type)
{
	if(!_availableCommands.contains(type))
		return false;

	if(type == "components-update")
	{
		_subscribedCommands << type;
		connect(_componentRegister, &ComponentRegister::updatedComponentState, this, &JsonCB::handleComponentState, Qt::UniqueConnection);
	}

	if(type == "sessions-update")
	{
		_subscribedCommands << type;
		connect(_bonjour, &BonjourBrowserWrapper::browserChange, this, &JsonCB::handleBonjourChange, Qt::UniqueConnection);
	}

	if(type == "priorities-update")
	{
		_subscribedCommands << type;
		connect(_prioMuxer, &PriorityMuxer::prioritiesChanged, this, &JsonCB::handlePriorityUpdate, Qt::UniqueConnection);
		connect(_prioMuxer, &PriorityMuxer::autoSelectChanged, this, &JsonCB::handlePriorityUpdate, Qt::UniqueConnection);
	}

	if(type == "imageToLedMapping-update")
	{
		_subscribedCommands << type;
		connect(_hyperion, &Hyperion::imageToLedsMappingChanged, this, &JsonCB::handleImageToLedsMappingChange, Qt::UniqueConnection);
	}

	if(type == "adjustment-update")
	{
		_subscribedCommands << type;
		connect(_hyperion, &Hyperion::adjustmentChanged, this, &JsonCB::handleAdjustmentChange, Qt::UniqueConnection);
	}

	if(type == "videomode-update")
	{
		_subscribedCommands << type;
		connect(_hyperion, &Hyperion::newVideoMode, this, &JsonCB::handleVideoModeChange, Qt::UniqueConnection);
	}

	if(type == "effects-update")
	{
		_subscribedCommands << type;
		connect(_hyperion, &Hyperion::effectListUpdated, this, &JsonCB::handleEffectListChange, Qt::UniqueConnection);
	}

	if(type == "settings-update")
	{
		_subscribedCommands << type;
		connect(_hyperion, &Hyperion::settingsChanged, this, &JsonCB::handleSettingsChange, Qt::UniqueConnection);
	}

	if(type == "instance-update")
	{
		_subscribedCommands << type;
		connect(HyperionIManager::getInstance(), &HyperionIManager::change, this, &JsonCB::handleInstanceChange, Qt::UniqueConnection);
	}

	return true;
}

void JsonCB::doCallback(const QString& cmd, const QVariant& data)
{
	QJsonObject obj;
	obj["command"] = cmd;

	if(static_cast<QMetaType::Type>(data.type()) == QMetaType::QJsonArray)
		obj["data"] = data.toJsonArray();
	else
		obj["data"] = data.toJsonObject();

	emit newCallback(obj);
}

void JsonCB::handleComponentState(const hyperion::Components comp, const bool state)
{
	QJsonObject data;
	data["name"] = componentToIdString(comp);
	data["enabled"] = state;

	doCallback("components-update", QVariant(data));
}

void JsonCB::handleBonjourChange(const QMap<QString,BonjourRecord>& bRegisters)
{
	QJsonArray data;
	for (const auto & session: bRegisters)
	{
		if (session.port<0) continue;
		QJsonObject item;
		item["name"]   = session.serviceName;
		item["type"]   = session.registeredType;
		item["domain"] = session.replyDomain;
		item["host"]   = session.hostName;
		item["address"]= session.address;
		item["port"]   = session.port;
		data.append(item);
	}

	doCallback("sessions-update", QVariant(data));
}

void JsonCB::handlePriorityUpdate()
{
	QJsonObject data;
	QJsonArray priorities;
	uint64_t now = QDateTime::currentMSecsSinceEpoch();
	QList<int> activePriorities = _prioMuxer->getPriorities();
	activePriorities.removeAll(255);
	int currentPriority = _prioMuxer->getCurrentPriority();

	foreach (int priority, activePriorities) {
		const Hyperion::InputInfo priorityInfo = _prioMuxer->getInputInfo(priority);
		QJsonObject item;
		item["priority"] = priority;
		if (priorityInfo.timeoutTime_ms > 0 )
			item["duration_ms"] = int(priorityInfo.timeoutTime_ms - now);

		// owner has optional informations to the component
		if(!priorityInfo.owner.isEmpty())
			item["owner"] = priorityInfo.owner;

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
			float Saturation, Luminace;

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

void JsonCB::handleImageToLedsMappingChange(const int& mappingType)
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

void JsonCB::handleVideoModeChange(const VideoMode& mode)
{
	QJsonObject data;
	data["videomode"] = QString(videoMode2String(mode));
	doCallback("videomode-update", QVariant(data));
}

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

void JsonCB::handleSettingsChange(const settings::type& type, const QJsonDocument& data)
{
	QJsonObject dat;
	if(data.isObject())
		dat[typeToString(type)] = data.object();
	else
		dat[typeToString(type)] = data.array();

	doCallback("settings-update", QVariant(dat));
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
