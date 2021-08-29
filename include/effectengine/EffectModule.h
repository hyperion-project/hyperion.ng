#pragma once

#undef slots
#include <Python.h>
#define slots

#include <QJsonValue>

class Effect;

class EffectModule: public QObject
{
	Q_OBJECT

public:
	// Python 3 module def
	static struct PyModuleDef moduleDef;

	// Init module
	static PyObject* PyInit_hyperion();

	// Register module once
	static void registerHyperionExtensionModule();

	// json 2 python
	static PyObject * json2python(const QJsonValue & jsonData);

	// Wrapper methods for Python interpreter extra buildin methods
	static PyMethodDef effectMethods[];
	static PyObject* wrapSetColor              (PyObject *self, PyObject *args);
	static PyObject* wrapSetImage              (PyObject *self, PyObject *args);
	static PyObject* wrapGetImage              (PyObject *self, PyObject *args);
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
};
