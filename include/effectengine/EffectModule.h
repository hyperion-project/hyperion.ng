#pragma once

#undef slots
// Don't use debug Python APIs on Windows (GitHub Actions only)
#if defined(GITHUB_ACTIONS) && defined(_MSC_VER) && defined(_DEBUG)
#if _MSC_VER >= 1930
#include <corecrt.h>
#endif
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#define slots Q_SLOTS

#include <QJsonValue>

class Effect;

class EffectModule : public QObject
{
	Q_OBJECT

public:
	// Register module once
	static void registerHyperionExtensionModule();

	// json 2 python
	static PyObject* json2python(const QJsonValue& jsonData);

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
	static PyObject* wrapLowestUpdateInterval  (PyObject* self, PyObject* args);
};
