#ifndef QSTRINGUTILS_H
#define QSTRINGUTILS_H

#include <QString>
#include <QStringList>

namespace QStringUtils {

enum class SplitBehavior {
	KeepEmptyParts,
	SkipEmptyParts,
};

inline QStringList split (const QString &string, const QString &sep, SplitBehavior behavior = SplitBehavior::KeepEmptyParts, Qt::CaseSensitivity cs = Qt::CaseSensitive)
{
	#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	return behavior == SplitBehavior::SkipEmptyParts ? string.split(sep, Qt::SkipEmptyParts , cs) : string.split(sep, Qt::KeepEmptyParts , cs);
	#else
	return behavior == SplitBehavior::SkipEmptyParts ? string.split(sep, QString::SkipEmptyParts , cs) : string.split(sep, QString::KeepEmptyParts , cs);
	#endif
}

inline QStringList split (const QString &string, QChar sep, SplitBehavior behavior = SplitBehavior::KeepEmptyParts, Qt::CaseSensitivity cs = Qt::CaseSensitive)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	return behavior == SplitBehavior::SkipEmptyParts ? string.split(sep, Qt::SkipEmptyParts , cs) : string.split(sep, Qt::KeepEmptyParts , cs);
#else
	return behavior == SplitBehavior::SkipEmptyParts ? string.split(sep, QString::SkipEmptyParts , cs) : string.split(sep, QString::KeepEmptyParts , cs);
#endif
}

inline QStringList split (const QString &string, const QRegExp &rx, SplitBehavior behavior = SplitBehavior::KeepEmptyParts)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	return behavior == SplitBehavior::SkipEmptyParts ? string.split(rx, Qt::SkipEmptyParts) : string.split(rx, Qt::KeepEmptyParts);
#else
	return behavior == SplitBehavior::SkipEmptyParts ? string.split(rx, QString::SkipEmptyParts) : string.split(rx, QString::KeepEmptyParts);
#endif
}
}

#endif // QSTRINGUTILS_H
