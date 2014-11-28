
#ifndef PY_H
#define PY_H

#include <Python.h>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <string>
#include <complex>

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

  /// Overload of `make(args, kwds)`, safe only if `make(NULL, NULL)` is safe.
  static PyObject *make(T x) noexcept
  {
    PyObject *o = type.tp_new(&type, nullptr, nullptr);
    if (o) new (((Extention *)o)->ptr()) T(std::move(x));
    return o;
  }

  /// Convenience casts.
  operator       T& ()       { return get(); }
  operator const T& () const { return get(); }
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

template<typename T>
struct NumExtention : Extention<T>
{
  static PyNumberMethods numMethods;
};

/// Default definitions of binary operators.
///
/// The default for when the type has the operator, `sym`bol, uses `int` and
/// `float` for otherwise, so the `int` version is always preferred, when
/// available.
#define DEFAULT_BIN(sym, op)                                                   \
  template<typename T,                                                         \
           typename = std::enable_if_t<                                        \
              std::is_same<T, std::decay_t<                                    \
                decltype(std::declval<T>() sym std::declval<T>())              \
              >>::value>                                                       \
           >                                                                   \
  auto default_##op(int)                                                       \
  {                                                                            \
    return [](PyObject *a, PyObject *b) {                                      \
      using Num = NumExtention<T>;                                             \
      return Num::make(((Num *) a)->get() sym ((Num *) b)->get());             \
    };                                                                         \
  }                                                                            \
                                                                               \
  template<typename T>                                                         \
  std::nullptr_t default_##op(float) {                                         \
    return nullptr;                                                            \
  }                                                                            \

DEFAULT_BIN(+,  plus);
DEFAULT_BIN(-,  subtract);
DEFAULT_BIN(*,  multiply);
DEFAULT_BIN(/,  divide);
DEFAULT_BIN(%,  modulus);
DEFAULT_BIN(^,  xor);

DEFAULT_BIN(&&, and);
DEFAULT_BIN(||, or);

#undef DEFAULT_BIN

// TODO: Logical operators.

template<typename T>
PyNumberMethods NumExtention<T>::numMethods = {
  default_plus<T>(0),         // nb_add
  default_subtract<T>(0),     // nb_subtract
  default_multiply<T>(0),     // nb_multiply;
  default_divide<T>(0),       // nb_divide;
  default_modulus<T>(0),      // nb_remainder;
  nullptr,                    // nb_divmod;
  nullptr,                    // nb_power;
  nullptr,                    // nb_negative;
  nullptr,                    // nb_positive;
  nullptr,                    // nb_absolute;
  nullptr,                    // nb_nonzero;
  nullptr,                    // nb_invert;
  nullptr,                    // nb_lshift;
  nullptr,                    // nb_rshift;
  default_and<T>(0),          // nb_and;
  default_xor<T>(0),          // nb_xor;
  default_or<T>(0),           // nb_or;
  nullptr,                    // nb_coerce;
  nullptr,                    // nb_int;
  nullptr,                    // nb_long;
  nullptr,                    // nb_float;
  nullptr,                    // nb_oct;
  nullptr,                    // nb_hex;

  nullptr,                    // nb_inplace_add;
  nullptr,                    // nb_inplace_subtract;
  nullptr,                    // nb_inplace_multiply;
  nullptr,                    // nb_inplace_divide;
  nullptr,                    // nb_inplace_remainder;
  nullptr,                    // nb_inplace_power;
  nullptr,                    // nb_inplace_lshift;
  nullptr,                    // nb_inplace_rshift;
  nullptr,                    // nb_inplace_and;
  nullptr,                    // nb_inplace_xor;
  nullptr,                    // nb_inplace_or;

  nullptr,                    // nb_floor_divide;
  nullptr,                    // nb_true_divide;
  nullptr,                    // nb_inplace_floor_divide;
  nullptr,                    // nb_inplace_true_divide;

  nullptr                     // nb_index;
};


}  // namespace py

#endif  // PY_H
