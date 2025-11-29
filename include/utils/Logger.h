#pragma once

#ifdef _WIN32
    #include <stdexcept>
#endif

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QMap>
#include <QAtomicInteger>
#include <QLoggingCategory>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    #include <QRecursiveMutex>
#else
    #include <QMutex>
#endif

#include <utils/global_defines.h>
#include <utils/MemoryTracker.h>

Q_DECLARE_LOGGING_CATEGORY(hyperion_logger_track)

// Forward declaration
class LoggerManager;

// ================================================================

#define LOG_MESSAGE(severity, logger, ...)   (logger)->Message(severity, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

// standard log messages
#define Debug(logger, ...)   LOG_MESSAGE(Logger::LogLevel::Debug  , logger, __VA_ARGS__)
#define Info(logger, ...)    LOG_MESSAGE(Logger::LogLevel::Info   , logger, __VA_ARGS__)
#define Warning(logger, ...) LOG_MESSAGE(Logger::LogLevel::Warning, logger, __VA_ARGS__)
#define Error(logger, ...)   LOG_MESSAGE(Logger::LogLevel::Error , logger, __VA_ARGS__)

// conditional log messages
#define DebugIf(condition, logger, ...)   if (condition) Debug(logger,   __VA_ARGS__)
#define InfoIf(condition, logger, ...)    if (condition) Info(logger,    __VA_ARGS__)
#define WarningIf(condition, logger, ...) if (condition) Warning(logger, __VA_ARGS__)
#define ErrorIf(condition, logger, ...)   if (condition) Error(logger,   __VA_ARGS__)

// ================================================================

class Logger : public QObject
{
    Q_OBJECT

    // Grant friendship to the memory tracking factory (with category param)
    template<typename T, typename Creator, typename... Args>
    friend QSharedPointer<T> makeTrackedShared(Creator creator, const QLoggingCategory& category, Args&&... args);
    // Grant friendship to custom deleters so they can access protected destructor
    template<typename T>
    friend void objectDeleter(T* ptr, const QString& subComponent, const QString& typeName, const QLoggingCategory& category);
public:
    enum class LogLevel
    {
        Unset = 0,
        Debug,
        Info,
        Warning,
        Error,
        Off
    };
    Q_ENUM(LogLevel)

    struct T_LOG_MESSAGE
	{
		QString      loggerName;
		QString      loggerSubName;
		QString      function;
		unsigned int line;
		QString      fileName;
		qint64       utime;
		QString      message;
		LogLevel     level;
		QString      levelString;
    };

    /**
     * Get a Logger instance for a given name
     * @param name The name of the logger
     * @param subName The sub name of the logger
     * @param minLevel The minimum level of this logger
     * @return The logger
     */
    static QSharedPointer<Logger> getInstance(const QString& name = "", const QString& subName = "__", LogLevel minLevel = LogLevel::Info);

    /**
     * Delete a Logger instance for a given name
     * @param name The name of the logger
     * @param subName The sub name of the logger
    */

    static void     deleteInstance(const QString & name = "", const QString & subName = "__");
    static void     setLogLevel(LogLevel level, const QString & name = "", const QString & subName = "__");
    static LogLevel getLogLevel(const QString & name = "", const QString & subName = "__");

	void     Message(LogLevel level, const char* sourceFile, const char* func, unsigned int line, const char* fmt, ...);
	void     setMinLevel(LogLevel level) { _minLevel = static_cast<int>(level); }
	LogLevel getMinLevel() const { return static_cast<LogLevel>(int(_minLevel)); }
	QString  getName() const { return _name; }
	QString  getSubName() const { return _subName; }

signals:
	void newLogMessage(Logger::T_LOG_MESSAGE);

public: // made public to allow generic tracked factory usage
    explicit Logger(const QString & name="", const QString & subName = "__", LogLevel minLevel = LogLevel::Info);
    ~Logger() override;

private:
	void write(const Logger::T_LOG_MESSAGE & message);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	static QRecursiveMutex       MapLock;
#else
	static QMutex                MapLock;
#endif
    static QMap<QString, QWeakPointer<Logger>> LoggerMap;
    static QAtomicInteger<int>   GLOBAL_MIN_LOG_LEVEL;

    const QString                _name;
    const QString                _subName;
    const bool                   _syslogEnabled;
    const unsigned               _loggerId;

	/* Only non-const member, hence the atomic */
	QAtomicInteger<int>			 _minLevel;
};

/**
 * @brief The LoggerManager class
 * Handles the management of multiple loggers and their configuration
 */
class LoggerManager : public QObject
{
    Q_OBJECT
	
	private:
	// Run LoggerManager as singleton
	LoggerManager();
	LoggerManager(const LoggerManager&) = delete;
	LoggerManager(LoggerManager&&) = delete;
	LoggerManager& operator=(const LoggerManager&) = delete;
	LoggerManager& operator=(LoggerManager&&) = delete;

	static QScopedPointer<LoggerManager> instance;

public:
	~LoggerManager() override;

    /**
     * @brief Get the singleton logger manager instance
     */
    static QScopedPointer<LoggerManager>& getInstance();

public slots:
    /**
     * Handle a new log message
     * @param msg The log message
     */
    void handleNewLogMessage(const Logger::T_LOG_MESSAGE&);

	/**
     * @brief Get the message buffer of all loggers
     * @param filter A filter to apply on the buffer
     * @return The buffer as a QJsonArray
     */
    QJsonArray getLogMessageBuffer(Logger::LogLevel filter= Logger::LogLevel::Unset) const;


signals:
	void newLogMessage(const Logger::T_LOG_MESSAGE&);

private:

	QList<Logger::T_LOG_MESSAGE>   _logMessageBuffer;
	const int                      _loggerMaxMsgBufferSize;
};

Q_DECLARE_METATYPE(Logger::T_LOG_MESSAGE)
