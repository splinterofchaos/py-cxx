
#include <Python.h>

#include <new>
#include <vector>
#include <string>
#include <iostream>
#include <iterator>

#include "Py/Py.h"
#include "Py/String.h"
#include "Py/Tuple.h"
#include "Py/List.h"

static PyObject *cppError;

using Ints = Py::Extention<std::vector<int>>;
using X  = Py::Extention<int>;

struct Pi {
  Pi(int, int) {
  }
};
using PT = Py::Extention<Pi>;

int init_ints(Ints *self, PyObject *args, PyObject *kwds)
{
  int x, y, z;
  if (!Py::ParseTuple(args, x, y, std::tie(z)))
    return -1;

  self->get() = {x, y, z};

  return 0;
}

PyObject *int_str(PyObject *self)
{
  Ints *ints = (Ints *) self;
  std::string s;
  for (int x : ints->ext) {
    if (s.size()) s += ", ";
    s += std::to_string(x);
  }

  return Py::String("[" + s + "]");
}

PyObject *primes(PyObject *, PyObject *)
{
  std::vector<int> v{1,3,5,7};
  return Py::List(v);
}

static PyMethodDef cppMethods[] = {
  {"primes",  primes, METH_VARARGS,
   "prime numbers under ten: "},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initcpp(void)
{
  Ints::type.tp_name = "cpp.Ints";
  Ints::type.tp_init = (initproc)init_ints;
  Ints::type.tp_str = int_str;

  X::type.tp_name = "cpp.X";
  if (PyType_Ready(&Ints::type) < 0)
    return;
  if (PyType_Ready(&X::type) < 0)
    return;

  PyObject *m = Py_InitModule("cpp", cppMethods);
  if (m == NULL)
    return;

  Py_INCREF(&Ints::type);
  PyModule_AddObject(m, "Ints", (PyObject *) &Ints::type);
  Py_INCREF(&X::type);
  PyModule_AddObject(m, "X", (PyObject *) &X::type);

  cppError = PyErr_NewException((char *)"cpp.error", NULL, NULL);
  Py_INCREF(cppError);
  PyModule_AddObject(m, "error", cppError);
}
