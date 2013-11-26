#pragma once

// Qt includes
#include <QThread>

// Python includes
#include <Python.h>

class Effect : public QThread
{
	Q_OBJECT

public:
	Effect(int priority, int timeout);
	virtual ~Effect();

	virtual void run();

	int getPriority() const;

public slots:
	void abort();

signals:
	void effectFinished(Effect * effect);

private slots:
	void effectFinished();

private:
	const int _priority;

	const int _timeout;

	PyThreadState * _interpreterThreadState;

	bool _abortRequested;
};
