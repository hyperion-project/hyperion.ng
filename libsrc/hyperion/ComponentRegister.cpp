#include <hyperion/ComponentRegister.h>
#include <iostream>

#include <hyperion/Hyperion.h>

using namespace hyperion;

ComponentRegister::ComponentRegister(Hyperion* hyperion)
	: _hyperion(hyperion)
	, _log(Logger::getInstance("ComponentRegister"))
{
	// init all comps to false
	QVector<hyperion::Components> vect;
	vect << COMP_ALL << COMP_SMOOTHING << COMP_BLACKBORDER << COMP_FORWARDER << COMP_BOBLIGHTSERVER << COMP_GRABBER << COMP_V4L << COMP_LEDDEVICE;
	for(auto e : vect)
	{
		_componentStates.emplace(e, ((e == COMP_ALL) ? true : false));
	}

	connect(_hyperion, &Hyperion::compStateChangeRequest, this, &ComponentRegister::handleCompStateChangeRequest);
}

ComponentRegister::~ComponentRegister()
{
}

int ComponentRegister::isComponentEnabled(const hyperion::Components& comp) const
{
	return (_componentStates.count(comp)) ? _componentStates.at(comp) : -1;
}

void ComponentRegister::setNewComponentState(const hyperion::Components comp, const bool activated)
{
	if(_componentStates[comp] != activated)
	{
		Debug( _log, "%s: %s", componentToString(comp), (activated? "enabled" : "disabled"));
		_componentStates[comp] = activated;
		// emit component has changed state
	 	emit updatedComponentState(comp, activated);
		emit _hyperion->compStateChangeRequest(comp, activated);
	}
}

void ComponentRegister::handleCompStateChangeRequest(const hyperion::Components comp, const bool activated)
{
	if(comp == COMP_ALL && !_inProgress)
	{
		_inProgress = true;
		if(!activated && _prevComponentStates.empty())
		{
			Debug(_log,"Disable Hyperion, store current component states");
			for(const auto comp : _componentStates)
			{
				// save state
				_prevComponentStates.emplace(comp.first, comp.second);
				// disable if enabled
				if(comp.second)
					emit _hyperion->compStateChangeRequest(comp.first, false);
			}
			setNewComponentState(COMP_ALL, false);
		}
		else if(activated && !_prevComponentStates.empty())
		{
			Debug(_log,"Enable Hyperion, recover previous component states");
			for(const auto comp : _prevComponentStates)
			{
				// if comp was enabled, enable again
				if(comp.second)
					emit _hyperion->compStateChangeRequest(comp.first, true);

			}
			_prevComponentStates.clear();
			setNewComponentState(COMP_ALL, true);
		}
		_inProgress = false;
	}
}
