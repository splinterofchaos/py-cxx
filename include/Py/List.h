
#ifndef PYXX_LIST_H
#define PYXX_LIST_H

#include <Python.h>

#include <algorithm>
#include <iterator>

namespace Py {

struct List : Object
{
  using Object::Object;

  using value_type      = PyObject *;
  using difference_type = ptrdiff_t;

  using reference       = value_type &;
  using const_reference = const value_type &;

  using pointer       = PyObject **;
  using const_pointer = const PyObject **;

  using iterator               = pointer;
  using const_iterator         = const_pointer;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  using size_type = Py_ssize_t;

  List(size_type size) noexcept : Object(PyList_New(size), true) { }
  List(size_t size)     noexcept : List((size_type)size) { }

  template<typename Container>
  List(const Container &c) noexcept : List(c.size()) {
    std::transform(std::begin(c), std::end(c), begin(), object_ptr);
  }

  size_type size() const noexcept { return Py_SIZE(self); }
  bool empty() const noexcept { return size() == 0; }

  const PyListObject *cptr() const noexcept { return (PyListObject *) self; }
  const PyListObject * ptr() const noexcept { return (PyListObject *) self; }
  PyListObject       * ptr()       noexcept { return (PyListObject *) self; }

  pointer       data()       noexcept { return ptr()->ob_item; }
  const_pointer data() const noexcept { return (const_pointer) ptr()->ob_item; }

  const_iterator cbegin() const noexcept { return data(); }
  const_iterator begin()  const noexcept { return cbegin(); }
  iterator       begin()        noexcept { return data(); }

  const_iterator cend() const noexcept { return data() + size(); }
  const_iterator  end() const noexcept { return data() + size(); }
  iterator        end()       noexcept { return data() + size(); }

  reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(cend());
  }
  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(cend());
  }
  reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(cbegin());
  }
  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(cbegin());
  }

  reference operator[] (size_type i) noexcept {
    return PyList_GET_ITEM(self, i);
  }
  const_reference operator[] (size_type i) const noexcept {
    return PyList_GET_ITEM(self, i);
  }

  const_reference front() const noexcept { return (*this)[0]; }
  reference       front()       noexcept { return (*this)[0]; }
  const_reference back()  const noexcept { return (*this)[size()]; }
  reference       back()        noexcept { return (*this)[size()]; }

  reference       Get(size_type i)       noexcept { return (*this)[i]; }
  const_reference Get(size_type i) const noexcept { return (*this)[i]; }

  bool Set(size_type i, PyObject *op) noexcept {
    return PyList_SetItem(self, i, op) == 0;
  }

  // Slicing
  const List Get(size_type i, size_type j) const noexcept {
    return List(PyList_GetSlice(self, i, j), true);
  }
  List Get(size_type i, size_type j) noexcept {
    return List(PyList_GetSlice(self, i, j), true);
  }

  bool Set(size_type i, size_type j, PyObject *v) noexcept {
    return PyList_SetSlice(self, i, j, v) == 0;
  }
  bool Set(size_type i, size_type j, const List &l) noexcept {
    return PyList_SetSlice(self, i, j, const_cast<List &>(l)) == 0;
  }

  template<typename O>
  bool insert(size_type i, O &&o) noexcept {
    return PyList_Insert(self, i, Object(std::forward<O>(o))) == 0;
  }
  bool insert(size_type i, PyObject *o) {
    return PyList_Insert(self, i, o) == 0;
  }
  bool insert(size_type i, const List &l) noexcept {
    return PyList_Insert(self, i, const_cast<List &>(l));
  }

  template<typename O>
  bool push_back(O &&o) noexcept {
    return PyList_Append(self, Object(std::forward<O>(o))) == 0;
  }
  bool push_back(PyObject *o) noexcept {
    return PyList_Append(self, o) == 0;
  }
  bool push_back(const List &l) noexcept {
    return PyList_Append(self, const_cast<List &>(l)) == 0;
  }

  bool Sort() noexcept {
    return PyList_Sort(self) == 0;
  }

  bool Reverse() noexcept {
    return PyList_Reverse(self) == 0;
  }

  PyObject *AsTuple() const noexcept {
    return PyList_AsTuple(self);
  }
};

}  // namespace py

#endif  // PYXX_LIST_H
