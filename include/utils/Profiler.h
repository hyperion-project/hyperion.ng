#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <map>
#include <utils/Logger.h>
#include <HyperionConfig.h>

#ifndef ENABLE_PROFILER
	#error "Profiler is not for productive code, enable it via cmake or remove header include"
#endif

// profiler
#define PROFILER_BLOCK_EXECUTION_TIME Profiler DEBUG_PROFILE__BLOCK__EXECUTION__TIME_messure_object(__FILE__, _FUNCNAME_, __LINE__ );
#define PROFILER_TIMER_START(stopWatchName)   Profiler::TimerStart(stopWatchName, __FILE__, _FUNCNAME_, __LINE__);
#define PROFILER_TIMER_GET(stopWatchName)    Profiler::TimerGetTime(stopWatchName, __FILE__, _FUNCNAME_, __LINE__);
#define PROFILER_TIMER_GET_IF(condition, stopWatchName) { if (condition) {Profiler::TimerGetTime(stopWatchName, __FILE__, _FUNCNAME_, __LINE__);} }


class Profiler
{
public:
	Profiler(const char* sourceFile, const char* func, unsigned int line);
	~Profiler();

	static void TimerStart(const QString stopWatchName, const char* sourceFile, const char* func, unsigned int line);
	static void TimerGetTime(const QString stopWatchName, const char* sourceFile, const char* func, unsigned int line);

private:
	static void initLogger();
	
	static Logger*  _logger;
	const char*     _file;
	const char*     _func;
	unsigned int    _line;
	unsigned int    _blockId;
	clock_t         _startTime;
};
