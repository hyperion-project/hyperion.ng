#pragma once

#undef slots
// Don't use debug Python APIs on Windows
#if defined(_MSC_VER) && defined(_DEBUG)
#if _MSC_VER >= 1930
#include <corecrt.h>
#endif
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
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

#if (PY_VERSION_HEX >= 0x03080000)
	void handlePythonError(PyStatus status, PyConfig& config);
#endif
};
