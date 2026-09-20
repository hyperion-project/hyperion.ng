#pragma once

// Python headers can conflict with Qt's slots macro and with feature-test
// macros already set by system headers. Normalize around the include.
#if defined(slots)
#pragma push_macro("slots")
#undef slots
#define HYPERION_RESTORE_QT_SLOTS_MACRO
#endif

#if defined(_POSIX_C_SOURCE)
#pragma push_macro("_POSIX_C_SOURCE")
#undef _POSIX_C_SOURCE
#define HYPERION_RESTORE_POSIX_C_SOURCE_MACRO
#endif

#if defined(_XOPEN_SOURCE)
#pragma push_macro("_XOPEN_SOURCE")
#undef _XOPEN_SOURCE
#define HYPERION_RESTORE_XOPEN_SOURCE_MACRO
#endif

#include <Python.h>

#if defined(HYPERION_RESTORE_XOPEN_SOURCE_MACRO)
#pragma pop_macro("_XOPEN_SOURCE")
#undef HYPERION_RESTORE_XOPEN_SOURCE_MACRO
#endif

#if defined(HYPERION_RESTORE_POSIX_C_SOURCE_MACRO)
#pragma pop_macro("_POSIX_C_SOURCE")
#undef HYPERION_RESTORE_POSIX_C_SOURCE_MACRO
#endif

#if defined(HYPERION_RESTORE_QT_SLOTS_MACRO)
#pragma pop_macro("slots")
#undef HYPERION_RESTORE_QT_SLOTS_MACRO
#endif