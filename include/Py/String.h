
#ifndef PYXX_STRING_H
#define PYXX_STRING_H

#include <Python.h>

#include <sstream>

#include "Py/Object.h"

namespace Py {

struct String : Object
{
  explicit String(const char *str) noexcept
    : Object(PyString_FromString(str), true)
  {
  }

  explicit String(const char *str, Py_ssize_t size) noexcept
    : Object(PyString_FromStringAndSize(str, size), true)
  {
  }

  String(const std::string &s) noexcept : String(s.c_str())
  {
  }

  String(const std::stringstream &s) noexcept : String(s.str())
  {
  }

  Py_ssize_t size() const noexcept
  {
    return PyString_Size(o);
  }

  const char *c_str() const noexcept
  {
    return PyString_AsString(o);
  }

  const char *begin() const noexcept
  {
    return c_str();
  }

  const char *end() const noexcept
  {
    return c_str() + size();
  }

  String & operator += (String &s) noexcept
  {
    PyString_Concat(&o, s);
    return *this;
  }

  String & operator += (String &&s) noexcept
  {
    PyString_ConcatAndDel(&o, s.release());
    return *this;
  }
};

template<typename...Args>
String format(const char *fmt, Args*...args)
{
  return String(PyString_FromFormat(fmt, args...), true);
}

} // namespace py

#endif  // PYXX_STRING_H
