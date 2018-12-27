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
	vect << COMP_ALL << COMP_SMOOTHING << COMP_BLACKBORDER << COMP_FORWARDER << COMP_UDPLISTENER << COMP_BOBLIGHTSERVER << COMP_GRABBER << COMP_V4L << COMP_LEDDEVICE;
	for(auto e : vect)
	{
		_componentStates.emplace(e, ((e == COMP_ALL) ? true : false));
	}
}

ComponentRegister::~ComponentRegister()
{
}

bool ComponentRegister::setHyperionEnable(const bool& state)
{
	if(!state && _prevComponentStates.empty())
	{
		Debug(_log,"Disable Hyperion, store current component states");
		for(const auto comp : _componentStates)
		{
			// save state
			_prevComponentStates.emplace(comp.first, comp.second);
			// disable if enabled
			if(comp.second)
				_hyperion->setComponentState(comp.first, false);
		}
		componentStateChanged(COMP_ALL, false);
		return true;
	}
	else if(state && !_prevComponentStates.empty())
	{
		Debug(_log,"Enable Hyperion, recover previous component states");
		for(const auto comp : _prevComponentStates)
		{
			// if comp was enabled, enable again
			if(comp.second)
				_hyperion->setComponentState(comp.first, true);

		}
		_prevComponentStates.clear();
		componentStateChanged(COMP_ALL, true);
		return true;
	}
	return false;
}

bool ComponentRegister::isComponentEnabled(const hyperion::Components& comp) const
{
	return _componentStates.at(comp);
}

void ComponentRegister::componentStateChanged(const hyperion::Components comp, const bool activated)
{
	if(_componentStates[comp] != activated)
	{
		Debug( _log, "%s: %s", componentToString(comp), (activated? "enabled" : "disabled"));
		_componentStates[comp] = activated;
		// emit component has changed state
	 	emit updatedComponentState(comp, activated);
	}
}
