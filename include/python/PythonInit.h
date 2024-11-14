#pragma once

#undef slots
#include <Python.h>
#define slots Q_SLOTS

///
/// @brief Handle the PythonInit, module registers and DeInit
///
class PythonInit
{
private:
	friend class HyperionDaemon;

	PythonInit();
	~PythonInit();

	void handlePythonError(PyStatus status, PyConfig& config);
};
