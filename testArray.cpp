
#include <iostream>
#include <memory>
#include <cassert>

#include "array.h"
#include "std_oversights.h"
#include "unittest.h"

using std::cout;
using std::endl;
using std::shared_ptr;
using std::make_shared;
using std::is_const;
using std::remove_reference;
using std::remove_pointer;

using cppToD::Array;
using cppToD::L$;

using mex::empty;


int main() {
  unittest::runUnitTests();
  return 0;
}

//TODO: Test void array - consider adding conversions

MEX_UNIT_TEST
  //Basics
  auto arr = Array<int>{1, 2, 3, 4, 5};
  assert(arr.size() == 5);
  assert(!empty(arr));
  assert(arr[0] == 1);
  assert(arr[4] == 5);
  arr[0] = -1;
  assert(arr[0] == -1);
  assert(*arr.data() == -1);
  *arr.data() = -5;
  assert(*arr.data() == -5);
MEX_END_UNIT_TEST

MEX_UNIT_TEST
  //Destructor check
  auto refCounted = make_shared<int>(5);
  {
    auto arr = Array<shared_ptr<int>>{ refCounted, refCounted };
  }
  assert(refCounted.unique());
MEX_END_UNIT_TEST

MEX_UNIT_TEST
  //Copy operations have reference semantics
  auto arr = Array<int>{1, 2, 3, 4, 5};
  auto alias = arr;
  alias[0] = -1;
  assert(arr[0] == -1);
MEX_END_UNIT_TEST

MEX_UNIT_TEST
  //Swap
  auto arr = Array<int>{1, 2, 3, 4, 5};
  auto empty = Array<int>{};
  arr.swap(empty);
  assert(empty.size() == 5);
  assert(empty[0] == 1);
MEX_END_UNIT_TEST

MEX_UNIT_TEST
  //Const correctness
  //Note: Would not compile with vector b.c. of internal copying!
  auto arr = Array<const int>{1, 2, 3, 4, 5};
  assert(arr[0] == 1);
  assert(*arr.data() == 1);
  using AccessorType = typename remove_reference<decltype(arr[0])>::type;
  static_assert(is_const<AccessorType>::value, "Can't change elements");
  using PtrType = typename remove_pointer<decltype(arr.data())>::type;
  static_assert(is_const<AccessorType>::value, "Can't change elements");
MEX_END_UNIT_TEST

MEX_UNIT_TEST
  //Dup has copy semantics
  auto arr = Array<int>{1, 2, 3, 4, 5};
  auto copy = arr.dup();
  copy[0] = -1;
  assert(arr[0] == 1);
  assert(copy[0] == -1);
  assert(copy[4] == 5);
MEX_END_UNIT_TEST

MEX_UNIT_TEST
  //Const->non const (dup) and non-const->const (idup)
  auto arr = Array<int>{1, 2, 3, 4, 5};
  auto icopy = arr.idup();
  using IAccessorType = typename remove_reference<decltype(icopy[0])>::type;
  static_assert(is_const<IAccessorType>::value, "Can't change elements");
  auto mutCopy = icopy.dup();
  using MutAccessorType = typename remove_reference<decltype(mutCopy[0])>::type;
  static_assert(!is_const<MutAccessorType>::value, "Can change elements");
  mutCopy[0] = 75;
MEX_END_UNIT_TEST

MEX_UNIT_TEST
  //Slicing
  Array<int> uninit;
  {
    auto arr = Array<int>{1, 2, 3, 4, 5};
    arr.sliceEq();
    assert(arr.size() == 5);
    uninit = arr.slice(1, L${});
  }
  assert(uninit.size() == 4);
  assert(uninit[0] == 2);

  uninit = uninit.slice(1, 3);
  assert(uninit.size() == 2);
  assert(uninit[0] == 3);
  assert(uninit[1] == 4);

  uninit = uninit.slice();
  assert(uninit.size() == 2);
  assert(uninit[0] == 3);
  assert(uninit[1] == 4);

  uninit.sliceEq(1, L${});
  assert(uninit.size() == 1);
  assert(uninit[0] == 4);

  uninit.sliceEq(0, 0);
  assert(uninit.size() == 0);
MEX_END_UNIT_TEST

MEX_UNIT_TEST
  //Range interface - walk
  auto arr = Array<int>{1, 2, 3};
  assert(!arr.empty());
  assert(arr.front() == 1);
  assert(arr.back() == 3);
  arr.popFront();
  assert(!arr.empty());
  assert(arr.front() == 2);
  assert(arr.back() == 3);
  arr.popFront();
  assert(!arr.empty());
  assert(arr.front() == 3);
  assert(arr.back() == 3);
  arr.popFront();
  assert(arr.empty());
MEX_END_UNIT_TEST

MEX_UNIT_TEST
  //Range interface - reverse walk
  auto arr = Array<int>{1, 2, 3};
  assert(!arr.empty());
  assert(arr.front() == 1);
  assert(arr.back() == 3);
  arr.popBack();
  assert(!arr.empty());
  assert(arr.front() == 1);
  assert(arr.back() == 2);
  arr.popBack();
  assert(!arr.empty());
  assert(arr.front() == 1);
  assert(arr.back() == 1);
  arr.popBack();
  assert(arr.empty());
MEX_END_UNIT_TEST


/*
 * Proof of concept:
 * 1) There is no GC:
 *  * a) Arbitrary growth will be disallowed. You may not grow an existing one of these.
 *  * b) You can slice/shrink as desired.
 * 2) Supported operations:
 *  * a) construction via initializer list, iterators, (fill with value?)
 *  * b) subscripting
 *  * c) slicing - both accessing and setting, including a full-range slice
 *  * d) copy constructor/assignment is actually an aliasing operation, rebinding the internal pointers
 *  * e) explicit dup/idup functions
 *  * f) get non-owning raw pointer/array out
 *  * g) length + an overload on a special "instance.length" stand-in type
 *  h) equality
 *  j) concatenation of arrays/elements
 *  * k) Range interface
 *
 */
