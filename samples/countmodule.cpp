
#include <Python.h>

#include "Py.h"

PyObject *count(PyObject *self, PyObject *args)
{
  static int i = 0;
  PySys_WriteStdout("%i\n", ++i);  // Just like printf.
  return PyInt_FromLong(i);
}

static PyMethodDef countMethods[] = {
  Py::MethodDef("count", "Returns the number of times called.", count),
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initcount()
{
  Py_InitModule("count", countMethods);
}
