
#ifndef PY_TUPLE_H
#define PY_TUPLE_H

#include <Python.h>

#include <tuple>
#include <utility>

namespace Py {

template<char...cs>
using CharList = std::integer_sequence<char, cs...>;

template<typename...T>
struct CharListConcat;

template<typename T>
struct CharListConcat<T> {
  using type = T;
};

template<typename...U, char...cs, char...cs2>
struct CharListConcat<CharList<cs...>, CharList<cs2...>, U...> {
  using type = typename CharListConcat<CharList<cs..., cs2...>, U...>::type;
};

template<typename...T>
using CharListConcat_t = typename CharListConcat<T...>::type;


/// A type to signify the rest of the parameters are optional.
struct Optional { };

template<typename...T>
struct PTCharListOf { };

template<> struct PTCharListOf<const char *> {
  using type = CharList<'s'>;
};

template<> struct PTCharListOf<Py_buffer> {
  using type = CharList<'s', '*'>;
};

template<> struct PTCharListOf<unsigned char> {
  using type = CharList<'b'>;
};

template<> struct PTCharListOf<unsigned int> {
  using type = CharList<'h'>;
};

template<> struct PTCharListOf<int> {
  using type = CharList<'i'>;
};

template<> struct PTCharListOf<Py_ssize_t> {
  using type = CharList<'n'>;
};

template<> struct PTCharListOf<float> {
  using type = CharList<'f'>;
};

template<> struct PTCharListOf<double> {
  using type = CharList<'d'>;
};

template<> struct PTCharListOf<PyObject *> {
  using type = CharList<'O'>;
};

template<> struct PTCharListOf<PyStringObject *> {
  using type = CharList<'S'>;
};

template<>
struct PTCharListOf<Optional> {
  using type = CharList<'|'>;
};

template<typename...Ts>
struct PTCharListOf<std::tuple<Ts...>> {
  using type =
    CharListConcat_t<CharList<'('>,
                     typename PTCharListOf<std::decay_t<Ts>>::type...,
                     CharList<')'>>;
};

template<typename T>
using PTCharListOf_t = typename PTCharListOf<T>::type;

template<typename...Ts>
struct ParseTupleBuilder { };

template<typename CL, typename T, typename...Ts>
struct ParseTupleBuilder<CL, T, Ts...> {
  using type = ParseTupleBuilder<CharListConcat_t<CL, PTCharListOf_t<T>>,
                                 Ts...>;
  constexpr static const char *fmt = type::fmt;
};

template<char...cs>
struct ParseTupleBuilder<CharList<cs...>> {
  using type = CharList<cs...>;

  static const char fmt[sizeof...(cs) + 1];
};

template<char...cs>
const char ParseTupleBuilder<CharList<cs...>>::fmt[] = { cs..., '\0' };

template<typename...Ts>
constexpr const char *ParseTupleFormat(Ts...) {
  return ParseTupleBuilder<CharList<>, std::decay_t<Ts>...>::fmt;
}


template<typename F, typename T, size_t...Is>
decltype(auto) apply_tuple(F&& f, T&& t, std::index_sequence<Is...>) {
  return std::forward<F>(f)(std::get<Is>(std::forward<T>(t))...);
}

template<typename F, typename T, size_t...Is>
decltype(auto) map_tuple(F&& f, T&& t, std::index_sequence<Is...>) {
  return std::make_tuple(std::forward<F>(f)(std::get<Is>(std::forward<T>(t)))...);
}

template<typename F, typename...Ts,
         typename Is = std::make_index_sequence<sizeof...(Ts)>>
decltype(auto) map_tuple(F&& f, std::tuple<Ts...> &t) {
  return map_tuple(std::forward<F>(f), t, Is());
}

template<typename...Bound,
         typename Indicies = std::make_index_sequence<sizeof...(Bound)>>
bool ParseTuple_impl(std::tuple<Bound...> &&bound) {
  return apply_tuple(PyArg_ParseTuple, bound, Indicies());
}

template<typename...Bound, typename Arg, typename...Args>
bool ParseTuple_impl(std::tuple<Bound...> &&bound, Arg &a, Args &...as) {
  return ParseTuple_impl(std::tuple_cat(std::move(bound), std::make_tuple(&a)),
                          as...);
}

template<typename...Bound, typename...Args>
bool ParseTuple_impl(std::tuple<Bound...> &&bound, Optional, Args &...as) {
  return ParseTuple_impl(std::move(bound), as...);
}

template<typename...Bound, typename...Ts, typename...Args>
bool ParseTuple_impl(std::tuple<Bound...> &&bound, std::tuple<Ts &...> &t,
                     Args &...as) {
  auto &&mapped = map_tuple([](auto &x) { return &x; }, t);
  return ParseTuple_impl(std::tuple_cat(bound, std::move(mapped)),
                         as...);
}

template<typename...Args>
bool ParseTuple(PyObject *args, Args &&...as) {
  return ParseTuple_impl(std::make_tuple(args, ParseTupleFormat(as...)),
                          as...);
}

template<typename...Bound,
         typename Indicies = std::make_index_sequence<sizeof...(Bound)>>
PyObject *BuildValue_impl(std::tuple<Bound...> &&bound) {
  return apply_tuple(Py_BuildValue, bound, Indicies());
}

template<typename...Bound, typename Arg, typename...Args>
PyObject *BuildValue_impl(std::tuple<Bound...> &&bound, Arg a, Args ...as) {
  return BuildValue_impl(std::tuple_cat(std::move(bound), std::make_tuple(a)),
                         as...);
}

template<typename...Bound, typename...Args>
PyObject *BuildValue_impl(std::tuple<Bound...> &&bound, Optional, Args &...as) {
  return BuildValue_impl(std::move(bound), as...);
}

template<typename...Bound, typename...Ts, typename...Args>
PyObject *BuildValue_impl(std::tuple<Bound...> &&bound, std::tuple<Ts...> &t,
                          Args &...as) {
  return BuildValue_impl(std::tuple_cat(bound, std::move(t)), as...);
}

template<typename...Args>
PyObject *BuildValue(Args &...as) {
  return BuildValue_impl(std::make_tuple(ParseTupleFormat(as...)),
                          as...);
}

}  // namespace py

#endif  // PY_TUPLE_H
