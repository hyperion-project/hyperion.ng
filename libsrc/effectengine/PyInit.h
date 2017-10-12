#pragma once

#undef slots
#include <Python.h>
#define slots
#include <utils/Logger.h>

class PyInit
{
public:
	inline PyInit(){
		if(!Py_IsInitialized())
		{
			Py_InitializeEx(0);
			PyEval_InitThreads(); // Create the GIL
			_mainThreadState = PyEval_SaveThread();
			Info(Logger::getInstance("PYTHON"),"Python interpreter initialized");
		}
	};
	inline ~PyInit(){
		if(!_mainThreadState)
		{
			PyEval_RestoreThread(_mainThreadState);
			_mainThreadState = nullptr;
			Py_Finalize();
			Info(Logger::getInstance("PYTHON"),"Cleanup Python interpreter");
		}
	};

	PyThreadState* getMainState(){ return _mainThreadState; };

private:
	static PyThreadState* _mainThreadState;
};
