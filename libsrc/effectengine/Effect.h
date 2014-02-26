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
    Effect(PyThreadState * mainThreadState, int priority, int timeout, const std::string & script, const Json::Value & args = Json::Value());
	virtual ~Effect();

	virtual void run();

	int getPriority() const;

	bool isAbortRequested() const;

    /// This function registers the extension module in Python
    static void registerHyperionExtensionModule();

public slots:
	void abort();

signals:
	void effectFinished(Effect * effect);

	void setColors(int priority, const std::vector<ColorRgb> &ledColors, const int timeout_ms, bool clearEffects);

private slots:
	void effectFinished();

private:
	PyObject * json2python(const Json::Value & json) const;

	// Wrapper methods for Python interpreter extra buildin methods
    static PyMethodDef effectMethods[];
    static PyObject* wrapSetColor(PyObject *self, PyObject *args);
	static PyObject* wrapSetImage(PyObject *self, PyObject *args);
	static PyObject* wrapAbort(PyObject *self, PyObject *args);
    static Effect * getEffect();

#if PY_MAJOR_VERSION >= 3
    static struct PyModuleDef moduleDef;
    static PyObject* PyInit_hyperion();
#else
    static void PyInit_hyperion();
#endif

private:
    PyThreadState * _mainThreadState;

	const int _priority;

	const int _timeout;

	const std::string _script;

	const Json::Value _args;

	int64_t _endTime;

	PyThreadState * _interpreterThreadState;

	bool _abortRequested;

	/// The processor for translating images to led-values
	ImageProcessor * _imageProcessor;

	/// Buffer for colorData
	std::vector<ColorRgb> _colors;
};
