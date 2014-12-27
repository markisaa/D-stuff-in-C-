
#include "unittest.h"

#include <iostream>
#include <exception>

using std::cerr;
using std::endl;
using std::begin;
using std::end;
using std::vector;
using std::exception;
using std::runtime_error;

namespace unittest {
  namespace detail {
    vector<UnitTestFunction>& unitTests() {
      static vector<UnitTestFunction> value;
      return value;
    }

    bool registerUnitTest(UnitTestFunction func) {
      unitTests().push_back(func);
      return true;
    }
  } //namespace detail

  ExpectationFailed::ExpectationFailed() : runtime_error("Expectation failed") {}

  void expect_true(bool condition) {
    if (!condition) {
      throw ExpectationFailed{};
    }
  }

  void runUnitTests() {
    //Reverse order to promote top-down coding style and have lower level tests
    //get called first.
    for(auto itr = detail::unitTests().rbegin(); itr != detail::unitTests().rend(); ++itr) {
      (*itr)();
    }
  }

} //namespace unittest
