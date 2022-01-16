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

	for(auto e : vect)
	{
		_componentStates.emplace(e, (e == COMP_ALL));
	}

	connect(_hyperion, &Hyperion::compStateChangeRequest, this, &ComponentRegister::handleCompStateChangeRequest);
}

ComponentRegister::~ComponentRegister()
{
}

int ComponentRegister::isComponentEnabled(hyperion::Components comp) const
{
	return (_componentStates.count(comp)) ? _componentStates.at(comp) : -1;
}

void ComponentRegister::setNewComponentState(hyperion::Components comp, bool activated)
{

	if (_componentStates.count(comp) > 0)
	{
		if (_componentStates[comp] != activated)
		{
			Debug(_log, "%s: %s", componentToString(comp), (activated ? "enabled" : "disabled"));
			_componentStates[comp] = activated;
			// emit component has changed state
			emit updatedComponentState(comp, activated);
		}
	}
}

void ComponentRegister::handleCompStateChangeRequest(hyperion::Components comps, bool activated)
{
	if(comps == COMP_ALL && !_inProgress)
	{
		_inProgress = true;
		if(!activated && _prevComponentStates.empty())
		{
			Debug(_log,"Disable Hyperion, store current component states");
			for(const auto &comp : _componentStates)
			{
				// save state
				_prevComponentStates.emplace(comp.first, comp.second);
				// disable if enabled
				if(comp.second)
				{
					emit _hyperion->compStateChangeRequest(comp.first, false);
				}
			}
			setNewComponentState(COMP_ALL, false);
		}
		else
		{
			if(activated && !_prevComponentStates.empty())
			{
				Debug(_log,"Enable Hyperion, recover previous component states");
				for(const auto &comp : _prevComponentStates)
				{
					// if comp was enabled, enable again
					if(comp.second)
					{
						emit _hyperion->compStateChangeRequest(comp.first, true);
					}
				}
				_prevComponentStates.clear();
				setNewComponentState(COMP_ALL, true);
			}
		}
		_inProgress = false;
	}
}
