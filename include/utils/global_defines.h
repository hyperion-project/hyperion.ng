#pragma once

#include <cstdint>
#include <limits>

#include <QList>

#define QSTRING_CSTR(str) str.toUtf8().constData()
typedef QList< int > QIntList;

constexpr uint32_t UINT8_MAX_SQUARED = static_cast<uint32_t>(UINT8_MAX) * UINT8_MAX;
