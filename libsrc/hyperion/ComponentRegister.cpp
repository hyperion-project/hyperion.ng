#include <hyperion/ComponentRegister.h>

#include <hyperion/Hyperion.h>

#include <hyperion/GrabberWrapper.h>

using namespace hyperion;

ComponentRegister::ComponentRegister(const QSharedPointer<Hyperion>& hyperionInstance)
	: QObject()
	, _hyperionWeak(hyperionInstance)
	, _log(nullptr)
{
	QString subComponent{ "__" };

	QSharedPointer<Hyperion> const hyperion = _hyperionWeak.toStrongRef();
	if (hyperion)
	{
		subComponent = hyperion->property("instance").toString();
	}
	_log = Logger::getInstance("COMPONENTREG", subComponent);
	TRACK_SCOPE_SUBCOMPONENT();

	// init all comps to false
	QVector<hyperion::Components> vect;
	vect << COMP_ALL << COMP_SMOOTHING << COMP_LEDDEVICE;

	bool const areScreenGrabberAvailable = !GrabberWrapper::availableGrabbers(GrabberTypeFilter::SCREEN).isEmpty();
	bool const areVideoGrabberAvailable = !GrabberWrapper::availableGrabbers(GrabberTypeFilter::VIDEO).isEmpty();
	bool const areAudioGrabberAvailable = !GrabberWrapper::availableGrabbers(GrabberTypeFilter::AUDIO).isEmpty();
	bool flatBufServerAvailable{ false };
	bool protoBufServerAvailable{ false };

#if defined(ENABLE_FLATBUF_SERVER)
	flatBufServerAvailable = true;
#endif

#if defined(ENABLE_PROTOBUF_SERVER)
	protoBufServerAvailable = true;
#endif

	if (areScreenGrabberAvailable)
	{
		vect << COMP_GRABBER;
	}

	if (areAudioGrabberAvailable)
	{
		vect << COMP_AUDIO;
	}

	if (areVideoGrabberAvailable)
	{
		vect << COMP_V4L;
	}

	if (areScreenGrabberAvailable || areVideoGrabberAvailable || flatBufServerAvailable || protoBufServerAvailable)
	{
		vect << COMP_BLACKBORDER;
	}


#if defined(ENABLE_BOBLIGHT_SERVER)
	vect << COMP_BOBLIGHTSERVER;
#endif

#if defined(ENABLE_FORWARDER)
	vect << COMP_FORWARDER;
#endif

	for (auto e : std::as_const(vect))
	{
		_componentStates.emplace(e, (e == COMP_ALL));
	}

	if (hyperion)
	{
		connect(hyperion.get(), &Hyperion::compStateChangeRequest, this, &ComponentRegister::handleCompStateChangeRequest);
		connect(hyperion.get(), &Hyperion::compStateChangeRequestAll, this, &ComponentRegister::handleCompStateChangeRequestAll);
	}
}

ComponentRegister::~ComponentRegister()
{
	TRACK_SCOPE_SUBCOMPONENT();
}

int ComponentRegister::isComponentEnabled(hyperion::Components comp) const
{
	auto iter = _componentStates.find(comp);
	return (iter != _componentStates.end()) ? int(iter->second) : -1;
}

void ComponentRegister::setNewComponentState(hyperion::Components comp, bool isActive)
{
	TRACK_SCOPE_SUBCOMPONENT() << "Set component:" << componentToString(comp) << "to" << (isActive ? "ENABLED" : "DISABLED");
	if (_componentStates.count(comp) > 0)
	{
		if (_componentStates[comp] != isActive)
		{
			Debug(_log, "%s: %s", componentToString(comp), (isActive ? "enabled" : "disabled"));
			_componentStates[comp] = isActive;
			// emit component has changed state
			emit updatedComponentState(comp, isActive);
		}
	}
}

void ComponentRegister::handleCompStateChangeRequest(hyperion::Components comps, bool isActive)
{
	if(comps == COMP_ALL )
	{
		TRACK_SCOPE_SUBCOMPONENT() << "Set all components to" << componentToString(comps) << "to" << (isActive ? "ENABLED" : "DISABLED");

		handleCompStateChangeRequestAll(isActive,{});
	}
}

void ComponentRegister::handleCompStateChangeRequestAll(bool isActive, const ComponentList& excludeList)
{
	TRACK_SCOPE_SUBCOMPONENT() << "Set all components to" << (isActive ? "ENABLED" : "DISABLED") << "with exclusions:" << [&excludeList]() 
	{
		QStringList exclNames;
		for (const auto& comp : excludeList)
		{
			exclNames << componentToString(comp);
		}
		return exclNames.join(", ");
	}();

	if (_inProgress)
	{
		return;
	}
	
	_inProgress = true;
	if(!isActive)
	{
		if (excludeList.isEmpty())
		{
			Debug(_log,"Disable Hyperion instance, store current components' state");
		}
		else
		{
			Debug(_log,"Disable selected Hyperion components, store their current state");
		}

		for (const auto& [component, isEnabled] : _componentStates)
		{
			if (!excludeList.contains(component) && component != COMP_ALL)
			{
				// save state
				_prevComponentStates.emplace(component, isEnabled);
				// disable if enabled
				if(isEnabled)
				{
					QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
					if (hyperion)
					{
						emit hyperion->compStateChangeRequest(component, false);
					}
				}
			}
		}

		if (excludeList.isEmpty())
		{
			setNewComponentState(COMP_ALL, false);
		}
	}
	else
	{
		if(isActive && !_prevComponentStates.empty())
		{
			if (excludeList.isEmpty())
			{
				Debug(_log,"Enable Hyperion instance, restore components' previous state");
			}
			else
			{
				Debug(_log,"Enable selected Hyperion components, restore their previous state");
			}

			for (const auto& [component, wasEnabled] : _prevComponentStates)
			{
				if (!excludeList.contains(component) && component != COMP_ALL && wasEnabled)
				{
				// if comp was enabled, enable again
					QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
					if (hyperion)
					{
						emit hyperion->compStateChangeRequest(component, true);
					}
				}
			}
			_prevComponentStates.clear();
			if (excludeList.isEmpty())
			{
				setNewComponentState(COMP_ALL, true);
			}
		}
	}
	_inProgress = false;
}
