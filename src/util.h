/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_UTIL
#define H_UTIL

#include <iostream>

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

/**
The template parameter represents a visitor displaying extended information
about a host object.

The extra information can be placed:

- partially in front of the previous details with ExtendedInfo::toString(false)
- and the rest at the end of the previous details with ExtendedInfo::toString(true)

Ensures that these 2 calls to ExtendedInfo::toString(bool) wrap the block
displaying the previous details.
*/
template<class ExtendedInfo>
class ToStringManager {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  const ExtendedInfo &ext;
  std::ostringstream &oss;

public:

  ToStringManager(const ExtendedInfo &ext_, std::ostringstream &oss_) :
      ext(ext_), oss(oss_) {
    oss<<ext.toString(false);
  }
  ~ToStringManager() {
    oss<<ext.toString(true);
  }
};

#endif // H_UTIL
