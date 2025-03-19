#ifndef WAITTIME_H
#define WAITTIME_H

#include <QEventLoop>
#include <QTimer>

#include <chrono>

inline void wait(std::chrono::milliseconds millisecondsWait)
{
	QEventLoop loop;
	QTimer timer;
	timer.setTimerType(Qt::PreciseTimer);
	QTimer::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	timer.start(millisecondsWait.count());
	loop.exec();
}

inline void wait(int millisecondsWait)
{
	QEventLoop loop;
	QTimer timer;
	timer.setTimerType(Qt::PreciseTimer);
	QTimer::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	timer.start(millisecondsWait);
	loop.exec();
}

#endif // WAITTIME_H
