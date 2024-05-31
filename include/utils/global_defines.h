#pragma once

#include <cstdint>
#include <limits>

#include <QList>

#define QSTRING_CSTR(str) str.toUtf8().constData()
typedef QList< int > QIntList;

// Undefine the max macro if it's defined (Windows-specific)
#ifdef max
#undef max
#endif

// Define your constexpr variable
constexpr uint32_t UINT8_MAX_SQUARED = static_cast<uint32_t>(std::numeric_limits<unsigned char>::max()) * static_cast<uint32_t>(std::numeric_limits<unsigned char>::max());

// Restore the max macro only if it was previously defined (Windows-specific)
#ifdef _MSC_VER
#define NOMINMAX  // Prevent Windows.h from defining min and max macros
#endif

// Restore the max macro if needed (Windows-specific)
#ifdef _MSC_VER
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#endif


