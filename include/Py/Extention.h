
#ifndef PYXX_EXTENTION_H
#define PYXX_EXTENTION_H

#include <Python.h>

#include "Py/Object.h"

namespace Py {

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
/// `...` for otherwise, so the `int` version is always preferred, when
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
  /* For if the result can construct an Object... */                           \
  template<typename T,                                                         \
           typename = std::enable_if_t<                                        \
              std::is_constructible<Object,                                    \
                decltype(std::declval<T>() sym std::declval<T>())              \
              >::value>                                                        \
           >                                                                   \
  binaryfunc default_##op(int)                                                 \
  {                                                                            \
    return [](PyObject *a, PyObject *b) -> PyObject * {                        \
      using Num = NumExtention<T>;                                             \
      return Object(((Num *) a)->get() sym ((Num *) b)->get());                \
    };                                                                         \
  }                                                                            \
                                                                               \
  template<typename T>                                                         \
  std::nullptr_t default_##op(...) {                                           \
    return nullptr;                                                            \
  }                                                                            \

/// In-place binary operators.
#define DEFAULT_IBIN(sym, op)                                                  \
  template<typename T>                                                         \
  auto default_##op(int)                                                       \
   -> decltype(std::declval<T>() sym std::declval<T>(), binaryfunc())          \
  {                                                                            \
    return [](PyObject *a, PyObject *b) {                                      \
      using Num = NumExtention<T>;                                             \
      ((Num *) a)->get() sym ((Num *) b)->get();                               \
      return a;                                                                \
    };                                                                         \
  }                                                                            \
                                                                               \
  template<typename T>                                                         \
  std::nullptr_t default_##op(...) {                                           \
    return nullptr;                                                            \
  }                                                                            \

/// In-place binary operators.
#define DEFAULT_UNARY(sym, op)                                                 \
  template<typename T>                                                         \
  auto default_##op(int)                                                       \
   -> decltype(sym std::declval<T>(), unaryfunc())                             \
  {                                                                            \
    return [](PyObject *o) {                                                   \
      using Num = NumExtention<T>;                                             \
      return Num::make(sym ((Num *) o)->get());                                \
    };                                                                         \
  }                                                                            \
                                                                               \
  template<typename T>                                                         \
  std::nullptr_t default_##op(...) {                                           \
    return nullptr;                                                            \
  }                                                                            \

template<typename T, typename U>
auto default_conversion(int) 
  -> decltype(static_cast<U>(std::declval<T>()), unaryfunc())
{
  return [](PyObject *o) -> PyObject * {
    using Num = NumExtention<T>;
    return Object(static_cast<U>(((Num *) o)->get()));
  };
}

template<typename T, typename U>
std::nullptr_t default_conversion(...) {
  return nullptr;
}


DEFAULT_BIN(+,  plus);
DEFAULT_BIN(-,  subtract);
DEFAULT_BIN(*,  multiply);
DEFAULT_BIN(/,  divide);
DEFAULT_BIN(%,  modulus);
DEFAULT_BIN(^,  xor);
DEFAULT_BIN(<<, lshift);
DEFAULT_BIN(>>, rshift);
DEFAULT_BIN(&&, and);
DEFAULT_BIN(||, or);

DEFAULT_IBIN(+=,  iadd);
DEFAULT_IBIN(-=,  isubtract);
DEFAULT_IBIN(*=,  imultiply);
DEFAULT_IBIN(/=,  idivide);
DEFAULT_IBIN(%=,  imodulus);
DEFAULT_IBIN(<<=, ilshift);
DEFAULT_IBIN(>>=, irshift);
DEFAULT_IBIN(&=,  iand);
DEFAULT_IBIN(^=,  ixor);
DEFAULT_IBIN(|=,  ior);

DEFAULT_UNARY(+, positive);
DEFAULT_UNARY(-, negative);
DEFAULT_UNARY(~, invert);

#undef DEFAULT_BIN
#undef DEFAULT_IBIN
#undef DEFAULT_UNARY

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
  default_negative<T>(0),     // nb_negative;
  default_positive<T>(0),     // nb_positive;
  nullptr,                    // nb_absolute;
  default_conversion<T, bool>(0),  // nb_nonzero;
  default_invert<T>(0),       // nb_invert;
  default_lshift<T>(0),       // nb_lshift;
  default_rshift<T>(0),       // nb_rshift;
  default_and<T>(0),          // nb_and;
  default_xor<T>(0),          // nb_xor;
  default_or<T>(0),           // nb_or;
  nullptr,                    // nb_coerce;
  default_conversion<T, long>(0),       // nb_int;
  default_conversion<T, long long>(0),  // nb_long;
  default_conversion<T, double>(0),     // nb_float;
  nullptr,                    // nb_oct;
  nullptr,                    // nb_hex;

  default_iadd<T>(0),         // nb_inplace_add;
  default_isubtract<T>(0),    // nb_inplace_subtract;
  default_imultiply<T>(0),    // nb_inplace_multiply;
  default_idivide<T>(0),      // nb_inplace_divide;
  default_imodulus<T>(0),     // nb_inplace_remainder;
  nullptr,                    // nb_inplace_power;
  default_ilshift<T>(0),      // nb_inplace_lshift;
  default_irshift<T>(0),      // nb_inplace_rshift;
  default_iand<T>(0),         // nb_inplace_and;
  default_ixor<T>(0),         // nb_inplace_xor;
  default_ior<T>(0),          // nb_inplace_or;

  nullptr,                    // nb_floor_divide;
  nullptr,                    // nb_true_divide;
  nullptr,                    // nb_inplace_floor_divide;
  nullptr,                    // nb_inplace_true_divide;

  nullptr                     // nb_index;
};

}  // namespace py

#endif  // PYXX_EXTENTION_H
