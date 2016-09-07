#include <hyperion/ComponentRegister.h>
#include <iostream>

ComponentRegister::ComponentRegister()
	: _log(Logger::getInstance("ComponentRegister"))
{
}

ComponentRegister::~ComponentRegister()
{
}


void ComponentRegister::componentStateChanged(const hyperion::Components comp, const bool activated)
{
	Info(_log, "%s: %s", componentToString(comp), (activated? "activated" : "off"));
	_componentStates.emplace(comp,activated);
	_componentStates[comp] = activated;

/*	for(auto comp : _componentStates)
	{
		std::cout << hyperion::componentToIdString(comp.first) << " " << comp.second << std::endl;
	}
	std::cout << "\n";
	*/
}
 
