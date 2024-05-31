#pragma once

#define QSTRING_CSTR(str) str.toUtf8().constData()
typedef QList< int > QIntList;

constexpr uint32_t UINT8_MAX_SQUARED = static_cast<uint32_t>(std::numeric_limits<unsigned char>::max()) * static_cast<uint32_t>(std::numeric_limits<unsigned char>::max());


