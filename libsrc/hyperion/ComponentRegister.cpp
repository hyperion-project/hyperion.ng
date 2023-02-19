#include <hyperion/ComponentRegister.h>
#include <iostream>

#include <hyperion/Hyperion.h>

#include <hyperion/GrabberWrapper.h>

using namespace hyperion;

ComponentRegister::ComponentRegister(Hyperion* hyperion)
	: _hyperion(hyperion)
	, _log(nullptr)
{
	QString subComponent = hyperion->property("instance").toString();
	_log= Logger::getInstance("COMPONENTREG", subComponent);

	// init all comps to false
	QVector<hyperion::Components> vect;
	vect << COMP_ALL << COMP_SMOOTHING << COMP_LEDDEVICE;

	bool areScreenGrabberAvailable = !GrabberWrapper::availableGrabbers(GrabberTypeFilter::VIDEO).isEmpty();
	bool areVideoGrabberAvailable = !GrabberWrapper::availableGrabbers(GrabberTypeFilter::VIDEO).isEmpty();
	bool areAudioGrabberAvailable = !GrabberWrapper::availableGrabbers(GrabberTypeFilter::AUDIO).isEmpty();
	bool flatBufServerAvailable { false };
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

	for(auto e : qAsConst(vect))
	{
		_componentStates.emplace(e, (e == COMP_ALL));
	}

	connect(_hyperion, &Hyperion::compStateChangeRequest, this, &ComponentRegister::handleCompStateChangeRequest);
	connect(_hyperion, &Hyperion::compStateChangeRequestAll, this, &ComponentRegister::handleCompStateChangeRequestAll);
}

ComponentRegister::~ComponentRegister()
{
}

int ComponentRegister::isComponentEnabled(hyperion::Components comp) const
{
	return (_componentStates.count(comp)) ? _componentStates.at(comp) : -1;
}

void ComponentRegister::setNewComponentState(hyperion::Components comp, bool isActive)
{

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
		handleCompStateChangeRequestAll(isActive,{});
	}
}

void ComponentRegister::handleCompStateChangeRequestAll(bool isActive, const ComponentList& excludeList)
{
	if (!_inProgress)
	{
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

			for(const auto &comp : _componentStates)
			{
				if (!excludeList.contains(comp.first) && comp.first != COMP_ALL)
				{
					// save state
					_prevComponentStates.emplace(comp.first, comp.second);
					// disable if enabled
					if(comp.second)
					{
						emit _hyperion->compStateChangeRequest(comp.first, false);
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

				for(const auto &comp : _prevComponentStates)
				{
					if (!excludeList.contains(comp.first) && comp.first != COMP_ALL)
					{
						// if comp was enabled, enable again
						if(comp.second)
						{
							emit _hyperion->compStateChangeRequest(comp.first, true);
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
}
