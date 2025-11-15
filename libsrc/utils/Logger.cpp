#include <utils/Logger.h>

#include <iostream>

#ifndef _WIN32
#include <syslog.h>
#elif _WIN32
#include <windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#endif
#include <QDateTime>
#include <QFileInfo>
#include <QMutexLocker>
#include <QThreadStorage>
#include <QJsonObject>

#include <utils/FileUtils.h>
#include <utils/MemoryTracker.h>

Q_LOGGING_CATEGORY(memory_logger_create, "memory.logger.create");
Q_LOGGING_CATEGORY(memory_logger_destroy, "memory.logger.destroy");

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
QRecursiveMutex        Logger::MapLock;
#else
QMutex                 Logger::MapLock{ QMutex::Recursive };
#endif

QMap<QString, QWeakPointer<Logger>> Logger::LoggerMap{};
QAtomicInteger<int> Logger::GLOBAL_MIN_LOG_LEVEL{ static_cast<int>(Logger::LogLevel::Unset) };

namespace
{
    const std::array<const char*, 6> LogLevelStrings = {{ "", "DEBUG", "INFO", "WARNING", "ERROR", "OFF" }};
#ifndef _WIN32
    const auto LogLevelSysLog = [] {
        std::array<int, 5> arr{};
        arr[static_cast<int>(Logger::LogLevel::Unset)] = LOG_DEBUG;
        arr[static_cast<int>(Logger::LogLevel::Debug)] = LOG_DEBUG;
        arr[static_cast<int>(Logger::LogLevel::Info)] = LOG_INFO;
        arr[static_cast<int>(Logger::LogLevel::Warning)] = LOG_WARNING;
        arr[static_cast<int>(Logger::LogLevel::Error)] = LOG_ERR;
        return arr;
    }();
#endif

    const size_t MAX_IDENTIFICATION_LENGTH = 22;

	QAtomicInteger<unsigned int> LoggerCount = 0;
	QAtomicInteger<unsigned int> LoggerId = 0;

	const int MAX_LOG_MSG_BUFFERED = 500;
	const int MaxRepeatCountSize = 200;
	QThreadStorage<int> RepeatCount;
	QThreadStorage<Logger::T_LOG_MESSAGE> RepeatMessage;
} // namespace

QSharedPointer<Logger> Logger::getInstance(const QString& name, const QString& subName, Logger::LogLevel minLevel)
{
    QMutexLocker lock(&MapLock);
    QString key = name + subName;

    // Try to resurrect logger from weak_ptr
    if (LoggerMap.contains(key))
    {
        if (auto sp = LoggerMap.value(key).lock())
        {
            return sp;
        }
    }

    // Not found or expired, create a new one.
    QSharedPointer<Logger> newLog = MAKE_TRACKED_SHARED_STATIC(Logger, name, subName, minLevel);

    LoggerMap.insert(key, newLog);
    connect(newLog.get(), &Logger::newLogMessage, LoggerManager::getInstance().data(), &LoggerManager::handleNewLogMessage);

    return newLog;
}

void Logger::deleteInstance(const QString& name, const QString& subName)
{
    QMutexLocker lock(&MapLock);

    if (name.isEmpty())
    {
        for (const auto& weakLogger : std::as_const(LoggerMap)) {
            if (auto strongLogger = weakLogger.lock())
            {
				TRACK_SCOPE_CATEGORY(memory_logger_destroy) << QString("|%1| Delete logger %2").arg(strongLogger->getSubName(), strongLogger->getName());
                strongLogger->deleteLater();
            }
        }

        LoggerMap.clear();
    }
    else
    {
        // This will cause the weak_ptr in the map to expire.
        // The logger will be deleted when the last shared_ptr is destroyed.
        LoggerMap.remove(name + subName);
    }

}

void Logger::setLogLevel(LogLevel level, const QString& name, const QString& subName)
{
	if (name.isEmpty())
	{
		GLOBAL_MIN_LOG_LEVEL = static_cast<int>(level);
	}
	else
	{
		auto log = Logger::getInstance(name, subName, level);
		log->setMinLevel(level);
	}
}

Logger::LogLevel Logger::getLogLevel(const QString& name, const QString& subName)
{
	if (name.isEmpty())
	{
		return static_cast<Logger::LogLevel>(int(GLOBAL_MIN_LOG_LEVEL));
	}

    const auto log = Logger::getInstance(name + subName);
    return log->getMinLevel();
}

Logger::Logger(const QString& name, const QString& subName, LogLevel minLevel)
	: _name(name)
	, _subName(subName)
	, _syslogEnabled(true)
	, _loggerId(LoggerId++)
	, _minLevel(static_cast<int>(minLevel))
{
	TRACK_SCOPE_CATEGORY(memory_logger_create) << QString("|%1| Create %2 logger").arg(_subName,_name);
	qRegisterMetaType<Logger::T_LOG_MESSAGE>();

#ifndef _WIN32
	if (LoggerCount.fetchAndAddOrdered(1) == 1 && _syslogEnabled)
	{
		openlog(nullptr, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
	}
#else
	// To keep LoggerCount behaviour consistent across platforms
	LoggerCount.fetchAndAddOrdered(1);
#endif
}

Logger::~Logger()
{
	TRACK_SCOPE_CATEGORY(memory_logger_destroy) << QString("|%1| Destroy %2 logger").arg(_subName,_name);
#ifndef _WIN32
	if (LoggerCount.fetchAndSubOrdered(1) == 0 && _syslogEnabled)
	{
		closelog();
	}
#else
	// To keep LoggerCount behaviour consistent across platforms
	LoggerCount.fetchAndSubOrdered(1);
#endif
}

void Logger::write(const Logger::T_LOG_MESSAGE& message)
{
    QString location;
    if (message.level == Logger::LogLevel::Debug)
    {
        location = QString("%1:%2:%3() | ")
            .arg(message.fileName)
            .arg(message.line)
            .arg(message.function);
    }

	QString name = "|" + message.loggerSubName + "| " + message.loggerName;
	name.resize(MAX_IDENTIFICATION_LENGTH, ' ');

	const QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(message.utime);
	QString const msg = QString("%1 %2 : <%3> %4%5\n")
		.arg(timestamp.toString(Qt::ISODateWithMs), name, message.levelString, location, message.message);

#ifdef _WIN32
	if (IsDebuggerPresent())
	{
		OutputDebugStringA(QSTRING_CSTR(msg));
	}
	else
#endif
		std::cout << msg.toStdString();

	emit newLogMessage(message);
}

void Logger::Message(LogLevel level, const char* sourceFile, const char* func, unsigned int line, const char* fmt, ...)
{
    auto const globalLevel = static_cast<LogLevel>(int(GLOBAL_MIN_LOG_LEVEL));

	// Determine the effective minimum level: prefer global if set, otherwise this logger's level
	const auto effectiveMinLevel = (globalLevel == Logger::LogLevel::Unset)
		? getMinLevel()
		: globalLevel;

	if (level < effectiveMinLevel)
	{
		return;
	}

	const size_t max_msg_length = 1024;
	char msg[max_msg_length];
	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, max_msg_length, fmt, args);
	va_end(args);

	const auto repeatedSummary = [&]
		{
			Logger::T_LOG_MESSAGE repMsg = RepeatMessage.localData();
			repMsg.message = "Previous line repeats " + QString::number(RepeatCount.localData()) + " times";
			repMsg.utime = QDateTime::currentMSecsSinceEpoch();

			write(repMsg);
#ifndef _WIN32
			if (_syslogEnabled && repMsg.level >= Logger::LogLevel::Warning)
			{
				syslog(LogLevelSysLog[static_cast<int>(repMsg.level)], "Previous line repeats %d times", RepeatCount.localData());
			}
#endif

			RepeatCount.setLocalData(0);
		};

	if (RepeatMessage.localData().loggerName == _name &&
		RepeatMessage.localData().loggerSubName == _subName &&
		RepeatMessage.localData().function == func &&
		RepeatMessage.localData().message == msg &&
		RepeatMessage.localData().line == line)
	{
		if (RepeatCount.localData() >= MaxRepeatCountSize)
		{
			repeatedSummary();
		}
		else
		{
			RepeatCount.setLocalData(RepeatCount.localData() + 1);
		}
	}
	else
	{
		if (RepeatCount.localData() != 0)
		{
			repeatedSummary();
		}

		Logger::T_LOG_MESSAGE logMsg;

		logMsg.loggerName = _name;
		logMsg.loggerSubName = _subName;
		logMsg.function = QString(func);
		logMsg.line = line;
		logMsg.fileName = FileUtils::getBaseName(sourceFile);
		logMsg.utime = QDateTime::currentMSecsSinceEpoch();
		logMsg.message = QString(msg);
		logMsg.level = level;
		logMsg.levelString = LogLevelStrings[static_cast<int>(level)];

		write(logMsg);
#ifndef _WIN32
        if (_syslogEnabled && level >= Logger::LogLevel::Warning)
        {
            syslog(LogLevelSysLog[static_cast<int>(level)], "%s", msg);
        }
#endif
        RepeatMessage.setLocalData(logMsg);
	}
}

QScopedPointer<LoggerManager> LoggerManager::instance;

LoggerManager::LoggerManager()
	: _loggerMaxMsgBufferSize(MAX_LOG_MSG_BUFFERED)
{
	TRACK_SCOPE_CATEGORY(memory_logger_create) << "Create LoggerManager";
	_logMessageBuffer.reserve(_loggerMaxMsgBufferSize);
}

LoggerManager::~LoggerManager()
{
	TRACK_SCOPE_CATEGORY(memory_logger_destroy) << "Destroy LoggerManager";
	// delete components
	Logger::deleteInstance();

	_logMessageBuffer.clear();
}

QJsonArray LoggerManager::getLogMessageBuffer(Logger::LogLevel filter) const
{
	QJsonArray messageArray;
	{
		for (const auto& logLine : std::as_const(_logMessageBuffer))
		{
			if (logLine.level >= filter)
			{
				QJsonObject message;
				message["loggerName"] = logLine.loggerName;
				message["loggerSubName"] = logLine.loggerSubName;
				message["function"] = logLine.function;
				message["line"] = QString::number(logLine.line);
				message["fileName"] = logLine.fileName;
				message["message"] = logLine.message;
				message["levelString"] = logLine.levelString;
				message["utime"] = QString::number(logLine.utime);

				messageArray.append(message);
			}
		}
	}
	return messageArray;
}

void LoggerManager::handleNewLogMessage(const Logger::T_LOG_MESSAGE& msg)
{
	_logMessageBuffer.push_back(msg);
	if (_logMessageBuffer.length() > _loggerMaxMsgBufferSize)
	{
		_logMessageBuffer.pop_front();
	}

	emit newLogMessage(msg);
}

QScopedPointer<LoggerManager>& LoggerManager::getInstance()
{
	if (!instance)
	{
		instance.reset(new LoggerManager());
	}

	return instance;
}
