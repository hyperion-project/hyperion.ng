#pragma once

#undef slots
#include <Python.h>
#define slots

// decl
extern PyThreadState* mainThreadState;

// py path seperator
#ifdef TARGET_WINDOWS
	#define PY_PATH_SEP ";";
#else // not windows
	#define PY_PATH_SEP ":";
#endif
