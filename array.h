
/*
 * Proof of concept:
 * 1) There is no GC:
 *  a) Arbitrary growth will be disallowed. You may not grow an existing one of these.
 *  b) You can slice/shrink as desired.
 * 2) Supported operations:
 *  a) construction via initializer list, iterators, (fill with value?)
 *  b) subscripting
 *  c) slicing - both accessing and setting, including a full-range slice
 *  d) copy constructor/assignment is actually an aliasing operation, rebinding the internal pointers
 *  e) explicit dup/idup functions
 *  f) get non-owning raw pointer/array out
 *  g) length + an overload on a special "instance.length" stand-in type
 *  h) equality
 *  j) concatenation of arrays/elements
 *
 */

#pragma once

#include <initializer_list>
#include <algorithm>
#include <type_traits>
#include <new>
#include <memory>

//TODO: Evaluate whether viewEnd_ should really just be size_

namespace cppToD {
  namespace detail {
    struct DupTag {};

    template<typename ITR>
    void destroyRangeImpl(ITR start, ITR finish, std::true_type) {
      using T = typename std::remove_reference<decltype(*start)>::type;
      std::for_each(start, finish, [] (T& elt) { elt.~T(); });
    }

    template<typename ITR>
    void destroyRangeImpl(ITR start, ITR finish, std::false_type) {}

    template<typename ITR>
    void destroyRange(ITR start, ITR finish) {
      using TD = typename std::decay<decltype(*start)>::type;
      destroyRangeImpl(start, finish, typename std::is_class<TD>::type{});
    }
  }
  //A stand-in for myType.length
  struct L$ {};

  template<typename T>
  struct Array {
  private:
    using TMutable = typename std::remove_const<T>::type;
    using TConst = typename std::add_const<T>::type;
  public:
    constexpr Array() noexcept : viewStart_{nullptr}, viewEnd_{nullptr} {}
    Array(std::size_t size_in) {
      raw_ = std::shared_ptr<TMutable>(new TMutable[size_in],
                                       std::default_delete<TMutable[]>{});
      viewStart_ = raw_.get();
      viewEnd_ = raw_.get() + size_in;
    }
    Array(std::size_t size_in, TConst& value)
      : Array{size_in, UninitializedConstructionTag{}} {
      std::uninitialized_fill(viewStart_, viewEnd_, value);
    }
    Array(std::initializer_list<T> values) {
      setupFromSrc(std::begin(values), std::end(values), values.size());
    }
    Array(const Array&) noexcept = default;
    Array(Array&&) noexcept = default;
    Array& operator=(const Array&) noexcept = default;
    Array& operator=(Array&&) noexcept = default;

    void swap(Array& rhs) noexcept {
      std::swap(raw_, rhs.raw_);
      std::swap(viewStart_, rhs.viewStart_);
      std::swap(viewEnd_, rhs.viewEnd_);
    }

    using DupType = Array<TMutable>;
    using IDupType = Array<TConst>;
    DupType dup() const {
      return DupType{*this, detail::DupTag{}};
    }
    IDupType idup() const {
      return IDupType{*this, detail::DupTag{}};
    }

    T& operator[](std::size_t index) noexcept {
      return viewStart_[index];
    }
    TConst& operator[](std::size_t index) const noexcept {
      return viewStart_[index];
    }

    constexpr bool empty() const noexcept {
      return !size();
    }
    constexpr std::size_t size() const noexcept {
      return std::distance(viewStart_, viewEnd_);
    }

    T* data() noexcept {
      return viewStart_;
    }
    constexpr TConst* data() const noexcept {
      return viewStart_;
    }

    Array slice() const {
      return Array{*this};
    }
    Array slice(std::size_t start, L$) const {
      return slice(start, size());
    }
    Array slice(std::size_t start, std::size_t finish) const {
      auto result = Array{*this};
      result.sliceEq(start, finish);
      return result;
    }

    const Array& sliceEq() const noexcept {
      return *this;
    }
    Array& sliceEq() noexcept {
      return *this;
    }
    Array& sliceEq(std::size_t start, L$) noexcept {
      return sliceEq(start, size());
    }
    Array& sliceEq(std::size_t start, std::size_t finish) noexcept {
      assert(finish >= start);
      assert(size() >= finish);
      viewEnd_ = viewStart_ + finish;
      viewStart_ += start;
      return *this;
    }

    void popFront() noexcept {
      sliceEq(1, L${});
    }
    void popBack() noexcept {
      sliceEq(0, size() - 1);
    }

    T front() noexcept {
      return *viewStart_;
    }
    constexpr TConst front() const noexcept {
      return *viewStart_;
    }

    T back() noexcept {
      return *(viewEnd_ - 1);
    }
    constexpr TConst back() const noexcept {
      return *(viewEnd_ - 1);
    }

    Array concat(TMutable&& newElt) const {
      auto resultSize = size() + 1;
      auto result = Array{resultSize, UninitializedConstructionTag{}};
      result.initializeFromSrc(viewStart_, viewEnd_);
      auto lastEltPtr = result.viewStart_ + size();
      new(lastEltPtr) TMutable{std::forward<TMutable>(newElt)};
      return result;
    }

    template<typename ELT>
    Array concat(const Array<ELT>& rhs) {
      ensureSameStorageType<ELT>();
      auto resultSize = size() + rhs.size();
      auto result = Array{resultSize, UninitializedConstructionTag{}};
      result.initializeFromSrc(viewStart_, viewEnd_);
      result.initializeFromSrc(rhs.viewStart_, rhs.viewEnd_,
                               result.viewStart_ + size());
      return result;
    }

  private:
    friend Array<TMutable>;
    friend Array<TConst>;

    template<typename SRCT>
    void ensureSameStorageType() {
      static_assert(std::is_same<TConst, typename Array<SRCT>::TConst>::value,
                    "Basically, constness can change, nothing else");
    }

    template<typename SRCT>
    Array(const Array<SRCT>& src, detail::DupTag) {
      ensureSameStorageType<SRCT>();
      setupFromSrc(src.viewStart_, src.viewEnd_, src.size());
    }

    struct UninitializedConstructionTag {};
    Array(std::size_t size_in, UninitializedConstructionTag) {
      setupUninitialized(size_in);
    }

    template<typename ITR>
    void setupFromSrc(ITR start, ITR finish, std::size_t size_in) {
      assert(std::distance(start, finish) == size_in);
      setupUninitialized(size_in);
      initializeFromSrc(start, finish);
    }

    template<typename ITR>
    void initializeFromSrc(ITR start, ITR finish) {
      initializeFromSrc(start, finish, viewStart_);
    }

    template<typename ITR>
    void initializeFromSrc(ITR start, ITR finish, TMutable* startingAt) {
      std::uninitialized_copy(start, finish, startingAt);
    }

    void setupUninitialized(std::size_t size_in) {
      const auto allocationSize = sizeof(TMutable) * size_in;
      auto deleterCourrier = [size_in] (const T* ptr) {
        Array::deleter(size_in, ptr);
      };

      raw_ = std::shared_ptr<TMutable>([&] {
        return static_cast<TMutable*>(::operator new(allocationSize));
      }(), deleterCourrier);
      viewStart_ = raw_.get();
      viewEnd_ = raw_.get() + size_in;
    }

    static void deleter(std::size_t numElts, TConst* ptr) {
      //Safe because the underlying storage is always mutable:
      auto mutPtr = const_cast<TMutable*>(ptr);
      auto mutEnd = mutPtr + numElts;
      detail::destroyRange(mutPtr, mutEnd);
      ::operator delete(static_cast<void*>(mutPtr));
    }

    std::shared_ptr<TMutable> raw_;
    TMutable* viewStart_;
    TMutable* viewEnd_;
  };

}
