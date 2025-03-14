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

// decl
extern PyThreadState* mainThreadState;

// py path seperator
#ifdef TARGET_WINDOWS
	#define PY_PATH_SEP ";";
#else // not windows
	#define PY_PATH_SEP ":";
#endif
