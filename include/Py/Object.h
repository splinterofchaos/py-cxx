
#ifndef PYXX_OBJECT_H
#define PYXX_OBJECT_H

#include <Python.h>

namespace Py {

struct Object
{
  PyObject *self;

  Object() {
    self = nullptr;
  }

  explicit Object(PyObject *self, bool own=false) noexcept : self(self) {
    if (!own)
      incref();
  }

  explicit Object(Object& obj)  noexcept : Object(obj.self) { }
  explicit Object(Object&& obj) noexcept : Object(obj.release(), true) { }
  explicit Object(bool b)       noexcept : Object(b ? Py_True : Py_False) { }

  // Strings
  explicit Object(const char *s) noexcept { self = PyString_FromString(s); }
  explicit Object(const std::string &s) noexcept : Object(s.c_str()) { }
  explicit Object(const char *s, Py_ssize_t size) noexcept {
    self = PyString_FromStringAndSize(s, size);
  }
  explicit Object(const std::string &s, Py_ssize_t size) noexcept
    : Object(s.c_str(), size) {
  }

  // Unicode
  explicit Object(const Py_UNICODE *s, Py_ssize_t size) noexcept {
    self = PyUnicode_FromUnicode(s, size);
  }

  // Python numbers
  explicit Object(size_t x)             noexcept { self = PyInt_FromSize_t(x); }
  explicit Object(Py_ssize_t x)         noexcept { self = PyInt_FromSsize_t(x); }
  explicit Object(int x)                noexcept { self = PyInt_FromLong(x); }
  explicit Object(long long x)          noexcept { self = PyLong_FromLongLong(x); }
  explicit Object(unsigned long long x) noexcept { self = PyLong_FromUnsignedLongLong(x); }
  explicit Object(double x)             noexcept { self = PyFloat_FromDouble(x); }

  // PyComplex
  explicit Object(float x, float y)     noexcept { self = PyComplex_FromDoubles(x, y); }
  explicit Object(double x, double y)   noexcept { self = PyComplex_FromDoubles(x, y); }
  explicit Object(Py_complex c)         noexcept { self = PyComplex_FromCComplex(c); }
  template<typename T>
  explicit Object(const std::complex<T> &c)
    : Object(std::real(c), std::imag(c)) {
  }

  ~Object() noexcept {
    decref();
  }

  void incref() noexcept
  {
    Py_XINCREF(self);
  }

  void decref() noexcept
  {
    Py_XDECREF(self);
  }

  PyObject *release() noexcept
  {
    PyObject *tmp = self;
    self = nullptr;
    return tmp;
  }

  /// For simplicity with Python API.
  operator PyObject * () & noexcept
  {
    return self;
  }

  operator PyObject * () && noexcept
  {
    return release();
  }
};

}  // namespace py

#endif  // PYXX_OBJECT_H
