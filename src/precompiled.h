/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

// For platforms without precompiled header support
// or to temporarily disable this support,
// just define H_PRECOMPILED
#ifndef H_PRECOMPILED
#define H_PRECOMPILED

#include "nanConcerns.h"
#include "util.h"

#include <cassert>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstring>

#include <algorithm>
#include <concepts>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#ifdef __cpp_lib_source_location
#include <source_location>
#else
#include <boost/assert/source_location.hpp>
#endif  // defined(__cpp_lib_source_location)

#include <boost/core/demangle.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

#include <gsl/assert>
#include <gsl/pointers>
#include <gsl/util>

// GSL before 4.1.0 doesn't have <gsl/zstring>
#if __has_include(<gsl/zstring>)
#include <gsl/zstring>

#else
#include <gsl/span>
#include <gsl/string_span>

#endif  // <zstring> available or not

#ifdef UNIT_TESTING

/*
Don't include <boost/test/...> here, because the unit tests depend on the define
BOOST_TEST_MODULE, which needs to be defined:
- before including any <boost/test/...>
- in exactly one unit test source (*.cpp), so not everywhere

This is not possible, since the precompiled header inclusion is mandatory for
every source (*.cpp) and ignores anything preceding its inclusion.

For details, see:
https://www.boost.org/doc/libs/1_87_0/libs/test/doc/html/boost_test/utf_reference/link_references/link_boost_test_module_macro.html
*/

#endif  // UNIT_TESTING

#endif  // !H_PRECOMPILED
