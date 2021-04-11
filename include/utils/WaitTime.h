#ifndef WAITTIME_H
#define WAITTIME_H

#include <QEventLoop>
#include <QTimer>

#include <chrono>

inline void wait(std::chrono::milliseconds millisecondsWait)
{
	QEventLoop loop;
	QTimer t;
	t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
	t.start(millisecondsWait.count());
	loop.exec();
}

inline void wait(int millisecondsWait)
{
	QEventLoop loop;
	QTimer t;
	t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
	t.start(millisecondsWait);
	loop.exec();
}

#endif // WAITTIME_H
