
#pragma once

#include <string>
#include <memory>
#include <type_traits>
#include <initializer_list>

/*
 *********************************OVERVIEW*************************************
 * This file contains implementations of stuff that will make it into the C++14
 * or C++17 standards. It also is (less so) a place for me to put highly
 * reusable functions that are widely applicable.
 * Implementation details are borrowed directly from WG21 standards proposals
 * where applicable. N4280 and N4022, among others, are borrowed from.
 */

namespace mex {

template<class T>
constexpr auto cbegin(const T& t)->decltype(t.cbegin()) { return t.cbegin(); }
template <class T, size_t N>
constexpr const T* cbegin(const T (&array)[N]) noexcept { return array; }

template<class T>
constexpr auto cend(const T& t)->decltype(t.cend()) { return t.cend(); }
template <class T, size_t N>
constexpr const T* cend(const T (&array)[N]) noexcept { return array + N; }


template <class C>
constexpr auto size(const C& c) -> decltype(c.size()) { return c.size(); }
template <class T, size_t N>
constexpr size_t size(const T (&array)[N]) noexcept { return N; }

template <class C>
constexpr auto empty(const C& c) -> decltype(c.empty()) { return c.empty(); }
template <class T, size_t N>
constexpr bool empty(const T (&array)[N]) noexcept { return false; }
template <class E>
constexpr bool empty(std::initializer_list<E> il) noexcept { return !il.size(); }

template <class C>
constexpr auto data(C& c) -> decltype(c.data()) { return c.data(); }
template <class C>
constexpr auto data(const C& c) -> decltype(c.data()) { return c.data(); }
template <class T, size_t N>
constexpr T* data(T (&array)[N]) noexcept { return array; }
template <class E>
constexpr const E* data(std::initializer_list<E> il) noexcept { return begin(il); }

std::string operator"" _s (const char* cstr, std::size_t sz);

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&& ...args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

namespace detail {
  template<typename FD> struct NotFnImpl;
}

//Note: Does not include the typedefs specified in N4022
template <class F>
auto not_fn(F&& fun)
noexcept(std::is_nothrow_constructible<typename std::decay<F>::type, F>::value)
  -> detail::NotFnImpl<typename std::decay<F>::type> {

  using FD = typename std::decay<F>::type;
  using FN = detail::NotFnImpl<FD>;
  static_assert(std::is_constructible<FD, F>::value, "N4022 20.10.9");
  static_assert(std::is_move_constructible<FN>::value, "N4022 20.10.9");

  return FN{std::forward<F>(fun)};
}

template<typename ForwardIt, typename VAL_TYPE, typename Compare>
std::pair<ForwardIt, bool> lower_bound_find(ForwardIt first, ForwardIt last, const VAL_TYPE& value, Compare comp) {
  auto result = std::make_pair(std::lower_bound(first, last, value, comp), true);
  if(result.first == last || comp(value, *result.first))
    result.second = false;
  return result;
}

template<typename ForwardIt, typename VAL_TYPE>
std::pair<ForwardIt, bool> lower_bound_find(ForwardIt first, ForwardIt last, const VAL_TYPE& value) {
  return lower_bound_find(first, last, value, std::less<VAL_TYPE>());
}

namespace detail {
template<typename FD>
struct NotFnImpl {
  NotFnImpl(FD fun) : fun_{std::move(fun)} {}

  template<typename... Args>
  auto operator()(Args&&... args) -> decltype(!std::declval<typename std::result_of<FD(Args...)>::type>()) {
    return !fun_(std::forward<Args>(args)...);
  }
private:
  FD fun_;
};

} //namespace detail

} //namespace mex
