/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.83) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2026 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

// For platforms without precompiled header support
// or to temporarily disable this support,
// just define H_PRECOMPILED
#ifndef H_PRECOMPILED
#define H_PRECOMPILED

// NOLINTBEGIN(misc-include-cleaner) : PCH-s don't need to use the headers

#include "util.h"  // IWYU pragma: export

// Set of includes not imported by "util.h"
#include <cassert>
#include <cfloat>
#include <climits>
#include <cmath>

#include <array>
#include <initializer_list>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <print>
#include <set>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <gsl/assert>
#include <gsl/span>
#include <gsl/util>

#include <boost/core/demangle.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>

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

// NOLINTEND(misc-include-cleaner)

#endif  // !H_PRECOMPILED
