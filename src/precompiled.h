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

/*
Below is a set of official headers imported in at least 2 translation units.
The ones imported by "util.h" are not enumerated here again.

Better performance could be achieved by directly importing
implementation-specific, non-official headers, (wrapped by the required official
headers mentioned in code) if many official headers (in)directly import them.

Proof:
- appended '-DH_PRECOMPILED -H -Winvalid-pch' to COMMON_CXXFLAGS in the Makefile
- ran 'make CXX=g++14 tests >private/build.log 2>&1'
- the top of most frequently imported headers based on the resulted 'build.log'
showed:
  * '/usr/include/c++/14/bits/version.h' was first, included 578 times by all
the import paths
  * the first official header '/usr/include/c++/14/climits' only on 12-th
position with just 69 inclusions.
  * there are compiler/version/library-specific includes

Since the project supports several compilers and environments, it would be
difficult to maintain an optimal set of (implementation-specific) headers
customized for each combination of the mentioned factors.

The most maintainable/portable solution was enumerating only official headers,
based on their use count throughout the project.
*/

#include "util.h"  // IWYU pragma: export

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
