#pragma once

#undef slots
#include <Python.h>
#define slots Q_SLOTS

// decl
extern PyThreadState* mainThreadState;

// py path seperator
#ifdef TARGET_WINDOWS
	#define PY_PATH_SEP ";";
#else // not windows
	#define PY_PATH_SEP ":";
#endif
