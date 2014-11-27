
#ifndef PY_H
#define PY_H

#include <Python.h>
#include <type_traits>
#include <utility>
#include <string>

namespace Py {


struct Object
{
  PyObject *o;

  Object() {
    o = nullptr;
  }

  explicit Object(PyObject *o, bool own=false) noexcept
    : o(o)
  {
    if (!own)
      incref();
  }

  explicit Object(Object& obj) noexcept
    : Object(obj.o)
  {
  }

  explicit Object(Object&& obj) noexcept
    : Object(obj.release())
  {
  }

  ~Object() noexcept
  {
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
  operator PyObject * () noexcept
  {
    return o;
  }
};

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

struct Type : PyTypeObject {
  Type(PyTypeObject ty)
    : PyTypeObject(std::move(ty))
  {
  }

  bool IsSubtype(PyTypeObject *ty)
  {
    return PyType_IsSubtype(ty, this);
  }

  bool IsSubtype(PyObject *o)
  {
    return IsSubtype(o->ob_type);
  }
};

template<typename T>
struct Extention : PyObject
{
  static Type type;

  T ext;

  T       &get()       & { return this->ext; }
  const T &get() const & { return this->ext; }

  T       *ptr()       & { return &this->ext; }
  const T *ptr() const & { return &this->ext; }

  static PyObject *make(PyObject *args=nullptr, PyObject *kwds=nullptr)
  {
    PyObject *o = type.tp_new(&type, args, kwds);
    if (o) type.tp_init(o, args, kwds);
    return o;
  }
};

template<typename T,
         typename = std::enable_if_t<std::is_default_constructible<T>::value>>
newfunc default_new()
{
  return [](PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    using Self = Extention<T>;
    Self *self = (Self *) type->tp_alloc(type, 0);
    if (self)
      new (self->ptr()) T();
    return (PyObject *) self;
  };
}

template<typename T,
         typename = std::enable_if_t<!std::is_default_constructible<T>::value>>
auto default_new() {
  return [](PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    return type->tp_alloc(type, 0);
  };
}

template<typename T>
Type Extention<T>::type((PyTypeObject) {
  PyObject_HEAD_INIT(NULL)
  0,                         // ob_size
  0,                         // tp_name
  sizeof(Extention<T>),      // tp_basicsize
  0,                         // tp_itemsize
  destructor([](PyObject *self) {
    ((Extention *) self)->get().T::~T();
    self->ob_type->tp_free(self);
  }),
  0,                         // tp_print
  0,                         // tp_getattr
  0,                         // tp_setattr
  0,                         // tp_compare
  0,                         // tp_repr
  0,                         // tp_as_number
  0,                         // tp_as_sequence
  0,                         // tp_as_mapping
  0,                         // tp_hash 
  0,                         // tp_call
  0,                         // tp_str
  0,                         // tp_getattro
  0,                         // tp_setattro
  0,                         // tp_as_buffer
  Py_TPFLAGS_DEFAULT,        // tp_flags
  0,                         // tp_doc 
  0,                         // tp_traverse 
  0,  	                     // tp_clear 
  0,  	                     // tp_richcompare 
  0,  	                     // tp_weaklistoffset 
  0,  	                     // tp_iter 
  0,  	                     // tp_iternext 
  0,                         // tp_methods 
  0,                         // tp_members 
  0,                         // tp_getset 
  0,                         // tp_base 
  0,                         // tp_dict 
  0,                         // tp_descr_get 
  0,                         // tp_descr_set 
  0,                         // tp_dictoffset 
  0,                         // tp_init 
  0,                         // tp_alloc 
  default_new<T>(),          // tp_new
});

}  // namespace py

#endif  // PY_H
