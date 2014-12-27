
#include "std_oversights.h"

using std::string;
using std::size_t;

namespace mex {

  string operator"" _s (const char* cstr, size_t sz) {
    return string{cstr, sz};
  }

} //!mex
