#pragma once

#include <QTimer>

#include <utils/Logger.h>
#include <hyperion/Hyperion.h>
#include <utils/settings.h>
#include <utils/ApiSync.h>

///
/// @brief Handle the command sync between instances
///
class InstanceSync : public QObject
{
	Q_OBJECT

public:
	InstanceSync(Hyperion *hyperion)
		: QObject(hyperion), _hyperion(hyperion)
	{
		// listen for config changes
		connect(_hyperion, &Hyperion::settingsChanged, this, &InstanceSync::handleSettingsUpdate);

		// request pending registrations
		connect(ApiSync::getInstance(), &ApiSync::answerActiveRegister, this, &InstanceSync::answerActiveRegister);
		QMetaObject::invokeMethod(ApiSync::getInstance(), "requestActiveRegister", Qt::QueuedConnection, Q_ARG(QObject *, _hyperion));

		// clear Priority
		connect(ApiSync::getInstance(), &ApiSync::clearPriority, this, &InstanceSync::clearPriority);

		// register & setInputImage, delay to prevent race conditions with requestActiveRegister
		QTimer::singleShot(250, [&]() {
			connect(ApiSync::getInstance(), &ApiSync::registerInput, this, &InstanceSync::registerInput);
			connect(ApiSync::getInstance(), &ApiSync::setInputImage, this, &InstanceSync::setInputImage);
		});

		// init
		handleSettingsUpdate(settings::INSTSYNC, _hyperion->getSetting(settings::INSTSYNC));
	};

private slots:
	///
	/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
	/// @param type   settingyType from enum
	/// @param config configuration object
	///
	void handleSettingsUpdate(const settings::type &type, const QJsonDocument &config)
	{
		if (type == settings::INSTSYNC)
		{
			updateSyncs(config.object()["syncTypes"].toArray());
			updateComponents(config.object()["compTypes"].toArray());
		}
	};

	void clearPriority(QObject *inst, const int &priority, const hyperion::Components &callerComp)
	{
		if (_hyperion != inst)
		{
			if (_reqComps.count(callerComp))
				_hyperion->clear(priority);
		}
	};

	void setColor(QObject *inst, const int &priority, const std::vector<ColorRgb> &ledColors, const int &timeout_ms, const QString &origin, const hyperion::Components &callerComp)
	{
		if (_hyperion != inst)
		{
			if (_reqComps.count(callerComp))
				_hyperion->setColor(priority, ledColors, timeout_ms, origin);
		}
	};

	void setEffect(QObject *inst, const QString &effectName, const int &priority, const int &duration, const QString &origin, const hyperion::Components &callerComp)
	{
		if (_hyperion != inst)
		{
			if (_reqComps.count(callerComp))
				_hyperion->setEffect(effectName, priority, duration, origin);
		}
	}

	void compStateChangeRequest(QObject *inst, const hyperion::Components &component, const int &priority, const hyperion::Components &callerComp)
	{
		if (_hyperion != inst)
		{
			if (_reqComps.count(callerComp))
				emit _hyperion->compStateChangeRequest(component, priority);
		}
	};

	void setLedMappingType(QObject *inst, const int &type, const hyperion::Components &callerComp)
	{
		if (_hyperion != inst)
		{
			if (_reqComps.count(callerComp))
				_hyperion->setLedMappingType(type);
		}
	};

	void setVideoMode(QObject *inst, const VideoMode &mode, const hyperion::Components &callerComp)
	{
		if (_hyperion != inst)
		{
			if (_reqComps.count(callerComp))
				_hyperion->setVideoMode(mode);
		}
	};

	void setSourceAutoSelect(QObject *inst, const bool state, const hyperion::Components &callerComp)
	{
		if (_hyperion != inst)
		{
			if (_reqComps.count(callerComp))
				_hyperion->setSourceAutoSelect(state);
		}
	};

	void setVisiblePriority(QObject *inst, const int &priority, const hyperion::Components &callerComp)
	{
		if (_hyperion != inst)
		{
			if (_reqComps.count(callerComp))
				_hyperion->setVisiblePriority(priority);
		}
	};

	void registerInput(QObject *inst, const int priority, const hyperion::Components &component, const QString &origin, const QString &owner, const hyperion::Components &callerComp)
	{
		if (_hyperion != inst)
		{
			if (_reqComps.count(callerComp))
				_hyperion->registerInput(priority, component, origin, owner);
		}
	};

	void setInputImage(QObject *inst, const int priority, const Image<ColorRgb> &image, const int64_t timeout_ms, const hyperion::Components &callerComp)
	{
		if (_hyperion != inst)
		{
			if (_reqComps.count(callerComp))
				_hyperion->setInputImage(priority, image, timeout_ms);
		}
	};

	void answerActiveRegister(QObject *callerInstance, std::map<int, API::registerData> data)
	{
		if (_hyperion == callerInstance)
		{
			for (const auto &entry : data)
			{
				if (_reqComps.count(entry.second.callerComp))
					_hyperion->registerInput(entry.first, entry.second.component, entry.second.origin, entry.second.owner);
			}
		}
	};

private:
	void updateSyncs(const QJsonArray &data)
	{
		// TODO get from schema avail types for single source of truth
		QStringList availTypes{"effect", "color", "compstate", "ledmapping", "videomode", "sourceauto", "visiblesource"};
		bool all = false;
		// if data contains "all" we skip specific check
		if (data.contains("all"))
			all = true;

		for (const auto &entry : availTypes)
		{
			setSyncState(entry, all ? true : data.contains(entry));
		}
	};

	void updateComponents(const QJsonArray &data)
	{
		_reqComps.clear();
		// add default comp
		if (!data.contains("ALLCO"))
			_reqComps.emplace(hyperion::COMP_INVALID, true);

		for (const auto &entry : data)
		{
			_reqComps.emplace(hyperion::stringToComponent(entry.toString()), true);
		}
	};

	void setSyncState(const QString &type, bool activate)
	{
		if (type == "effect")
		{
			if (activate)
				connect(ApiSync::getInstance(), &ApiSync::setEffect, this, &InstanceSync::setEffect, Qt::UniqueConnection);
			else
				disconnect(ApiSync::getInstance(), &ApiSync::setEffect, this, &InstanceSync::setEffect);
		}

		if (type == "color")
		{
			if (activate)
				connect(ApiSync::getInstance(), &ApiSync::setColor, this, &InstanceSync::setColor, Qt::UniqueConnection);
			else
				disconnect(ApiSync::getInstance(), &ApiSync::setColor, this, &InstanceSync::setColor);
		}

		if (type == "compstate")
		{
			if (activate)
				connect(ApiSync::getInstance(), &ApiSync::compStateChangeRequest, this, &InstanceSync::compStateChangeRequest, Qt::UniqueConnection);
			else
				disconnect(ApiSync::getInstance(), &ApiSync::compStateChangeRequest, this, &InstanceSync::compStateChangeRequest);
		}

		if (type == "ledmapping")
		{
			if (activate)
				connect(ApiSync::getInstance(), &ApiSync::setLedMappingType, this, &InstanceSync::setLedMappingType, Qt::UniqueConnection);
			else
				disconnect(ApiSync::getInstance(), &ApiSync::setLedMappingType, this, &InstanceSync::setLedMappingType);
		}

		if (type == "videomode")
		{
			if (activate)
				connect(ApiSync::getInstance(), &ApiSync::setVideoMode, this, &InstanceSync::setVideoMode, Qt::UniqueConnection);
			else
				disconnect(ApiSync::getInstance(), &ApiSync::setVideoMode, this, &InstanceSync::setVideoMode);
		}

		if (type == "sourceauto")
		{
			if (activate)
				connect(ApiSync::getInstance(), &ApiSync::setSourceAutoSelect, this, &InstanceSync::setSourceAutoSelect, Qt::UniqueConnection);
			else
				disconnect(ApiSync::getInstance(), &ApiSync::setSourceAutoSelect, this, &InstanceSync::setSourceAutoSelect);
		}

		if (type == "visiblesource")
		{
			if (activate)
				connect(ApiSync::getInstance(), &ApiSync::setVisiblePriority, this, &InstanceSync::setVisiblePriority, Qt::UniqueConnection);
			else
				disconnect(ApiSync::getInstance(), &ApiSync::setVisiblePriority, this, &InstanceSync::setVisiblePriority);
		}
	};

	/// Hyperion instance pointer
	Hyperion *_hyperion;
	/// Contains all requested components
	std::map<hyperion::Components, bool> _reqComps;
};
