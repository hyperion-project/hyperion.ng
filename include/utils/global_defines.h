#pragma once

#include <cstdint>

#include <QList>

#define QSTRING_CSTR(str) str.toUtf8().constData()
using QIntList = QList<int>;

constexpr double DOUBLE_UINT8_MAX_SQUARED = static_cast<double>(UINT8_MAX) * UINT8_MAX;

template <typename T>
struct DebugLimitedListWrapper
{
    const QList<T>& list;
    const int limit;
};

template <typename T>
DebugLimitedListWrapper<T> limitForDebug(const QList<T>& list, int limit = -1)
{
    return {list, limit};
}

template <typename T>
QDebug operator<<(QDebug dbg, const DebugLimitedListWrapper<T>& wrapper)
{
    const int max = wrapper.limit <= 0 ? wrapper.list.size() : wrapper.limit;
    const int size = wrapper.list.size();
    const int printCount = std::min(size, max);

    if (wrapper.limit  <= 0)
    {
        dbg.noquote().nospace() << "Items (" << size << ") [";
    }
    else
    {
        dbg.noquote().nospace() << "Items (" << printCount << " of " << size << ") [";
    }

    for (int i = 0; i < printCount; ++i)
    {
        dbg.nospace() << wrapper.list[i];
    }
    
    if (size > printCount)
    {
        dbg.nospace() << "...";
    }

    dbg.noquote().nospace() << "]";
    return dbg.space();
}
