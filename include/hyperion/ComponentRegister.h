#pragma once

#include <utils/Components.h> 
#include <utils/Logger.h>

// STL includes
#include <map>

#include <QObject>

class ComponentRegister : public QObject
{
	Q_OBJECT

public:
	ComponentRegister();
	~ComponentRegister();

	std::map<hyperion::Components, bool> getRegister() { return _componentStates; };

public slots:
	void componentStateChanged(const hyperion::Components comp, const bool activated);

private:
	std::map<hyperion::Components, bool> _componentStates;
	Logger * _log;
};

