#pragma once

// Python includes
#include <Python.h>

// Qt includes
#include <QThread>
#include <QSize>
#include <QImage>
#include <QPainter>
#include <QMap>

// Hyperion includes
#include <hyperion/ImageProcessor.h>
#include <utils/Components.h>

class Effect : public QThread
{
	Q_OBJECT

public:
	Effect(PyThreadState * mainThreadState, int priority, int timeout, const QString & script, const QString & name, const QJsonObject & args = QJsonObject(), const QString & origin="System", unsigned smoothCfg=0);
	virtual ~Effect();

	virtual void run();

	int getPriority() const;
	
	QString getScript() const { return _script; }
	QString getName() const { return _name; }
	
	int getTimeout() const {return _timeout; }
	
	QJsonObject getArgs() const { return _args; }

	bool isAbortRequested() const;

    /// This function registers the extension module in Python
    static void registerHyperionExtensionModule();

public slots:
	void abort();

signals:
	void effectFinished(Effect * effect);

	void setColors(int priority, const std::vector<ColorRgb> &ledColors, const int timeout_ms, bool clearEffects, hyperion::Components componentconst, QString origin, unsigned smoothCfg);

private slots:
	void effectFinished();

private:
	PyObject * json2python(const QJsonValue & jsonData) const;

	// Wrapper methods for Python interpreter extra buildin methods
	static PyMethodDef effectMethods[];
	static PyObject* wrapSetColor              (PyObject *self, PyObject *args);
	static PyObject* wrapSetImage              (PyObject *self, PyObject *args);
	static PyObject* wrapAbort                 (PyObject *self, PyObject *args);
	static PyObject* wrapImageShow             (PyObject *self, PyObject *args);
	static PyObject* wrapImageLinearGradient   (PyObject *self, PyObject *args);
	static PyObject* wrapImageConicalGradient  (PyObject *self, PyObject *args);
	static PyObject* wrapImageRadialGradient   (PyObject *self, PyObject *args);
	static PyObject* wrapImageSolidFill        (PyObject *self, PyObject *args);
	static PyObject* wrapImageDrawLine         (PyObject *self, PyObject *args);
	static PyObject* wrapImageDrawPoint        (PyObject *self, PyObject *args);
	static PyObject* wrapImageDrawRect         (PyObject *self, PyObject *args);
	static PyObject* wrapImageDrawPolygon      (PyObject *self, PyObject *args);
	static PyObject* wrapImageDrawPie          (PyObject *self, PyObject *args);
	static PyObject* wrapImageSetPixel         (PyObject *self, PyObject *args);
	static PyObject* wrapImageGetPixel         (PyObject *self, PyObject *args);
	static PyObject* wrapImageSave             (PyObject *self, PyObject *args);
	static PyObject* wrapImageMinSize          (PyObject *self, PyObject *args);
	static PyObject* wrapImageWidth            (PyObject *self, PyObject *args);
	static PyObject* wrapImageHeight           (PyObject *self, PyObject *args);
	static PyObject* wrapImageCRotate          (PyObject *self, PyObject *args);
	static PyObject* wrapImageCOffset          (PyObject *self, PyObject *args);
	static PyObject* wrapImageCShear           (PyObject *self, PyObject *args);
	static PyObject* wrapImageResetT           (PyObject *self, PyObject *args);
	static Effect * getEffect();

#if PY_MAJOR_VERSION >= 3
	static struct PyModuleDef moduleDef;
	static PyObject* PyInit_hyperion();
#else
	static void PyInit_hyperion();
#endif

	void addImage();

	PyThreadState * _mainThreadState;

	const int _priority;

	const int _timeout;

	const QString _script;
	const QString _name;
	unsigned _smoothCfg;

	const QJsonObject _args;

	int64_t _endTime;

	PyThreadState * _interpreterThreadState;

	bool _abortRequested;

	/// The processor for translating images to led-values
	ImageProcessor * _imageProcessor;

	/// Buffer for colorData
	QVector<ColorRgb> _colors;
	
	
	QString         _origin;
	QSize           _imageSize;
	QImage          _image;
	QPainter*       _painter;
	QVector<QImage> _imageStack;
};
	
