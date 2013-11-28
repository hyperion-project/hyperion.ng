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
	// Wrapper methods for Python interpreter extra buildin methods
	static PyMethodDef effectMethods[];
	static PyObject* wrapSetColor(PyObject *self, PyObject *args);
	static PyObject* wrapSetImage(PyObject *self, PyObject *args);
	static PyObject* wrapGetLedCount(PyObject *self, PyObject *args);
	static PyObject* wrapAbort(PyObject *self, PyObject *args);

private:
	const int _priority;

	const int _timeout;

	PyThreadState * _interpreterThreadState;

	bool _abortRequested;
};
