#pragma once

#include <utils/Components.h>
#include <utils/Logger.h>

// STL includes
#include <map>

#include <QObject>

class Hyperion;

///
/// @brief The component register reflects and manages the current state of all components and Hyperion as a whole
/// It emits also real component state changes (triggert from the specific component), which can be used for listening APIs (Network Clients/Plugins)
///
class ComponentRegister : public QObject
{
	Q_OBJECT

public:
	ComponentRegister(Hyperion* hyperion);
	~ComponentRegister();

	///
	/// @brief  Check if a component is currently enabled
	/// @param  comp   The component from enum
	/// @return        True if component is running else false. Not found is -1
	///
	int isComponentEnabled(const hyperion::Components& comp) const;

	/// contains all components and their state
	std::map<hyperion::Components, bool> getRegister() { return _componentStates; };

signals:
	///
	///	@brief Emits whenever a component changed (really) the state
	///	@param comp   The component
	///	@param state  The new state of the component
	///
	void updatedComponentState(const hyperion::Components comp, const bool state);

public slots:
	///
	/// @brief is called whenever a component change a state, DO NOT CALL FROM API, use signal hyperion->compStateChangeRequest
	///	@param comp   The component
	///	@param state  The new state of the component
	///
	void setNewComponentState(const hyperion::Components comp, const bool activated);

private slots:
	///
	/// @brief Handle COMP_ALL changes from Hyperion->compStateChangeRequest
	///
	void handleCompStateChangeRequest(const hyperion::Components comps, const bool activated);

private:
	///  Hyperion instance
	Hyperion * _hyperion;
	/// Logger instance
	Logger * _log;
	/// current state of all components
	std::map<hyperion::Components, bool> _componentStates;
	/// on hyperion off we save the previous states of all components
	std::map<hyperion::Components, bool> _prevComponentStates;
	// helper to prevent self emit chains
	bool _inProgress = false;
};
