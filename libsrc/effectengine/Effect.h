#pragma once

// Python includes
#include <Python.h>

// Qt includes
#include <QThread>

// Hyperion includes
#include <hyperion/ImageProcessor.h>

class Effect : public QThread
{
	Q_OBJECT

public:
	Effect(int priority, int timeout, const std::string & script, const Json::Value & args = Json::Value());
	virtual ~Effect();

	virtual void run();

	int getPriority() const;

	bool isAbortRequested() const;

public slots:
	void abort();

signals:
	void effectFinished(Effect * effect);

	void setColors(int priority, const std::vector<ColorRgb> &ledColors, const int timeout_ms);

private slots:
	void effectFinished();

private:
	PyObject * json2python(const Json::Value & json) const;

	// Wrapper methods for Python interpreter extra buildin methods
	static PyMethodDef effectMethods[];
	static PyObject* wrapSetColor(PyObject *self, PyObject *args);
	static PyObject* wrapSetImage(PyObject *self, PyObject *args);
	static PyObject* wrapAbort(PyObject *self, PyObject *args);
	static Effect * getEffect(PyObject *self);

private:
	const int _priority;

	const int _timeout;

	const std::string & _script;

	const Json::Value & _args;

	int64_t _endTime;

	PyThreadState * _interpreterThreadState;

	bool _abortRequested;

	/// The processor for translating images to led-values
	ImageProcessor * _imageProcessor;
};
