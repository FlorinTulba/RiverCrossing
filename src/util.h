/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_UTIL
#define H_UTIL

/// Less typing when specifying container ranges
#define CBOUNDS(cont) std::cbegin(cont), std::cend(cont)
#define CRBOUNDS(cont) std::crbegin(cont), std::crend(cont)
#define BOUNDS(cont) std::begin(cont), std::end(cont)
#define RBOUNDS(cont) std::rbegin(cont), std::rend(cont)

/// Validating a (smart) pointer. Throwing provided exception with given message if NULL
#define VP_EX_MSG(ptr, exc, msg) \
  ((nullptr != (ptr)) ? (ptr) : throw exc(std::string(__func__) + " - " + (msg)))

/// Validating a (smart) pointer. Throwing provided exception if NULL
#define VP_EX(ptr, exc) VP_EX_MSG((ptr), exc, "NULL value!")

/// Validating a (smart) pointer. Throwing invalid_argument with given message if NULL
#define VP_MSG(ptr, msg) VP_EX_MSG((ptr), std::invalid_argument, (msg))

/// Validating a (smart) pointer. Throwing invalid_argument if NULL
#define VP(ptr) VP_EX((ptr), std::invalid_argument)

#endif // H_UTIL
