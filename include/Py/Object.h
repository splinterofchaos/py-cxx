
#ifndef PYXX_OBJECT_H
#define PYXX_OBJECT_H

#include <Python.h>

namespace Py {

struct Object
{
  PyObject *o;

  Object() {
    o = nullptr;
  }

  explicit Object(PyObject *o, bool own=false) noexcept : o(o) {
    if (!own)
      incref();
  }

  explicit Object(Object& obj)  noexcept : Object(obj.o) { }
  explicit Object(Object&& obj) noexcept : Object(obj.release(), true) { }
  explicit Object(bool b)       noexcept : Object(b ? Py_True : Py_False) { }

  // Strings
  explicit Object(const char *s) noexcept { o = PyString_FromString(s); }
  explicit Object(const std::string &s) noexcept : Object(s.c_str()) { }
  explicit Object(const char *s, Py_ssize_t size) noexcept {
    o = PyString_FromStringAndSize(s, size);
  }
  explicit Object(const std::string &s, Py_ssize_t size) noexcept
    : Object(s.c_str(), size) {
  }

  // Unicode
  explicit Object(const Py_UNICODE *s, Py_ssize_t size) noexcept {
    o = PyUnicode_FromUnicode(s, size);
  }

  // Python numbers
  Object(size_t x)             noexcept { o = PyInt_FromSize_t(x); }
  Object(Py_ssize_t x)         noexcept { o = PyInt_FromSsize_t(x); }
  Object(long long x)          noexcept { o = PyLong_FromLongLong(x); }
  Object(unsigned long long x) noexcept { o = PyLong_FromUnsignedLongLong(x); }
  Object(double x)             noexcept { o = PyFloat_FromDouble(x); }

  // PyComplex
  Object(double x, double y)   noexcept { o = PyComplex_FromDoubles(x, y); }
  Object(Py_complex c)         noexcept { o = PyComplex_FromCComplex(c); }
  template<typename T>
  Object(const std::complex<T> &c)
    : Object(std::real(c), std::imag(c)) {
  }

  ~Object() noexcept {
    decref();
  }

  void incref() noexcept
  {
    Py_XINCREF(o);
  }

  void decref() noexcept
  {
    Py_XDECREF(o);
  }

  PyObject *release() noexcept
  {
    PyObject *tmp = o;
    o = nullptr;
    return tmp;
  }

  /// For simplicity with Python API.
  operator PyObject * () & noexcept
  {
    return o;
  }

  operator PyObject * () && noexcept
  {
    return release();
  }
};

}  // namespace py

#endif  // PYXX_OBJECT_H
