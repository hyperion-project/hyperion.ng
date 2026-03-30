#pragma once

#include <python/PythonCompat.h>

///
/// @brief Handle the PythonInit, module registers and DeInit
///
class PythonInit
{
private:
	friend class HyperionDaemon;

	PythonInit();
	~PythonInit();

#if (PY_VERSION_HEX >= 0x03080000)
	void handlePythonError(PyStatus status, PyConfig& config);
#endif
};
