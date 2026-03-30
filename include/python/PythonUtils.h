#pragma once

#include <python/PythonCompat.h>

// decl
extern PyThreadState* mainThreadState;

// py path seperator
#ifdef TARGET_WINDOWS
	#define PY_PATH_SEP ";";
#else // not windows
	#define PY_PATH_SEP ":";
#endif
