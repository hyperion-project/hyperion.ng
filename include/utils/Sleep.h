#pragma once

#include <QThread>

class Sleep : protected QThread {
public:
	static inline void msleep(unsigned long msecs) {
		QThread::msleep(msecs);
	}
};
