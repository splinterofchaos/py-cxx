
#include <Python.h>

#include "Py/Py.h"
#include "Py/Tuple.h"
#include "Py/String.h"

/// This module is roughly equivalent to the following Python code:
///
/// class Vec:
///   def __init__(self, x, y, z):
///     self.x = x
///     self.y = y
///     self.z = z
///
///   def __negative__(self):
///     return Vec(-x, -y, -z)
///
///   def __positive__(self):
///     return self
///
///   def __add__(self, other):
///     return Vec(x+other.x, y+other.y, z+other.z)
///
///   def __sub__(self, other):
///     return Vec(x-other.x, y-other.y, z-other.z)
///
///   def __iadd__(self, other):
///     x += other.x
///     y += other.y
///     z += other.z
///
///   def __isub__(self, other):
///     x -= other.x
///     y -= other.y
///     z -= other.z
///
///   # dot product
///   def __mul__(self, other):
///     return x*other.x + y*other.y + z*other.z
///
///   # cross product
///   def __xor__(self, other):
///     ...

struct Vec {
  float x, y, z;
};

constexpr Vec operator- (const Vec &v) {
  return {-v.x, -v.y, -v.z};
}

constexpr Vec &operator+ (Vec &v) {
  return v;
}

constexpr const Vec &operator+ (const Vec &v) {
  return v;
}

Vec &operator+= (Vec &a, const Vec &b) {
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;
  return a;
}

Vec &operator-= (Vec &a, const Vec &b) {
  a.x -= b.x;
  a.y -= b.y;
  a.z -= b.z;
  return a;
}

constexpr Vec operator+ (const Vec &a, const Vec &b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

constexpr Vec operator- (const Vec &a, const Vec &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

constexpr float operator* (const Vec &a, const Vec &b) {
  return a.x*b.x + a.y*b.y + a.z*b.z;
}

constexpr Vec operator^ (const Vec &a, const Vec &b) {
  return { a.y*b.z - a.z*b.y,
           a.z*b.x - a.x*b.z,
           a.x*b.y - a.y*b.x };
}

using PyVec = Py::NumExtention<Vec>;

int init_vec(PyVec *self, PyObject *args, PyObject *)
{
  Vec &v = self->get();
  if (!Py::ParseTuple(args, v.x, v.y, v.z))
    return -1;
  return 0;
}

PyObject *vec_str(PyVec *self)
{
  return Py::String("<"  + std::to_string(self->get().x) +
                    ", " + std::to_string(self->get().y) +
                    ", " + std::to_string(self->get().z) +
                    ">");
}

PyObject *cross(PyObject *self, PyObject *args)
{
  PyObject *o1, *o2;
  if (!Py::ParseTuple(args, o1, o2))
    return nullptr;

  // Ensure o1 and 2 are the right types.
  if (!PyVec::type.IsSubtype(o1) || !PyVec::type.IsSubtype(o2))
    return nullptr;
  
  Vec &v = ((PyVec *) o1)->get(), &w = ((PyVec *) o2)->get();
  float i = v.y*w.z - v.z*w.y;
  float j = v.z*w.x - v.x*w.z;
  float k = v.x*w.y - v.y*w.x;

  // >>> vec.Vec(i,j,k)
  PyObject *val = Py::BuildValue(i, j, k);
  PyObject *ret = PyVec::make(val);
  Py_DECREF(val);

  return ret;
}

static PyMethodDef vecMethods[] = {
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initvec()
{
  PyVec::type.tp_name = "vec.Vec";
  Py::Register(PyVec::type.tp_init, init_vec);
  Py::Register(PyVec::type.tp_str, vec_str);
  Py::Register(PyVec::type.tp_repr, vec_str);
  PyVec::type.tp_as_number = &PyVec::numMethods;
  if (PyType_Ready(&PyVec::type) < 0)
    return;

  PyObject *m = Py_InitModule("vec", vecMethods);
  if (!m)
    return;

  Py_INCREF(&PyVec::type);
  PyModule_AddObject(m, "Vec", (PyObject *) &PyVec::type);
}

