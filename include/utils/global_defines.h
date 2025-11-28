#pragma once

#include <cstdint>

#include <QList>
#include <QVector>
#include <QDebug>
#include <QtGlobal>

#define QSTRING_CSTR(str) str.toUtf8().constData()
using QIntList = QList<int>;

constexpr double DOUBLE_UINT8_MAX_SQUARED = static_cast<double>(UINT8_MAX) * UINT8_MAX;

template <typename Container>
struct DebugLimitedListWrapper
{
    const Container& list;
    const int limit;
};

template <typename Container>
DebugLimitedListWrapper<Container> limitForDebug(const Container& list, int limit = -1)
{
    return {list, limit};
}

template <typename Container>
QDebug operator<<(QDebug dbg, const DebugLimitedListWrapper<Container>& wrapper)
{
    const int max = wrapper.limit <= 0 ? wrapper.list.size() : wrapper.limit;
    const int size = wrapper.list.size();
    const int printCount = qMin(size, max);

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
