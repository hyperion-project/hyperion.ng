#pragma once

#include <cstdint>

#include <QList>

#define QSTRING_CSTR(str) str.toUtf8().constData()
typedef QList< int > QIntList;

constexpr double DOUBLE_UINT8_MAX_SQUARED = static_cast<double>(UINT8_MAX) * UINT8_MAX;
