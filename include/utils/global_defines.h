#pragma once

#include <cstdint>
#include <limits>

#include <QList>

#define QSTRING_CSTR(str) str.toUtf8().constData()
typedef QList< int > QIntList;

constexpr double DOUBLE_MAX_SQUARED = static_cast<double>(std::numeric_limits<unsigned char>::max()) * std::numeric_limits<unsigned char>::max();
