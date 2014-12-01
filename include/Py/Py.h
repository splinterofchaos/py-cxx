
#ifndef PY_H
#define PY_H

#include <Python.h>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <string>
#include <complex>

#include "Py/Object.h"
#include "Py/Extention.h"

namespace Py {

template<typename R, typename...X>
constexpr int arity(R(*)(X...)) {
  return sizeof...(X);
}

template<typename R, typename...X>
constexpr bool returns_PyObject(R(*)(X...)) {
  // Value is either a PyObject, or a subclass of one.
  return std::is_convertible<R, PyObject *>::value;
}

template<typename R, typename...X>
constexpr bool is_PyCFunction(R(*)(X...)) {
  return false;
}

template<>
constexpr bool is_PyCFunction(PyCFunction) {
  return true;
}

template<typename F>
constexpr int MethodType(F f) {
  return arity(f) == 3     ? METH_KEYWORDS
       : is_PyCFunction(f) ? METH_VARARGS
                           : METH_O;
}

template<typename F>
constexpr PyMethodDef MethodDef(const char *name, const char *doc,
                                 int type, F f)
{
  static_assert(arity(F()) == 2 || arity(F()) == 3,
                "Methods must have an arity of 2 or 3");
  static_assert(returns_PyObject(F()), "Methods must return a PyObject *.");
  return {name, (PyCFunction)f, type, doc};
}

template<typename F>
constexpr PyMethodDef MethodDef(const char *name, const char *doc, const F f)
{
  return MethodDef(name, doc, MethodType(f), f);
}

template<typename R, typename X, typename...Y>
constexpr bool is_object_method(R(*)(X, Y...))
{
  return std::is_convertible<X, PyObject *>::value;
}

template<typename F>
void Register(initproc &proc, F f)
{
  static_assert(arity(F()) == 3, "__init__ must have an arity of 2 or 3");
  static_assert(is_object_method(F()),
                "First argument must be PyObject-compatible.");
  proc = (initproc) f;
}

template<typename F>
void Register(reprfunc &proc, F f)
{
  static_assert(arity(F()) == 1, "__str/repr__ must have an arity of 1");
  static_assert(is_object_method(F()),
                "First argument must be PyObject-compatible.");
  proc = (reprfunc) f;
}


}  // namespace py

#endif  // PY_H
