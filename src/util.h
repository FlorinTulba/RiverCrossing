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
#include <memory>

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

namespace rc {

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

/**
Provides access to a certain decorator type within a hierarchy.
IfDecorator is the base interface of the entire hierarchy,
AbsDecorator is the base of the actual decorators.
*/
template<class AbsDecorator, class IfDecorator>
struct DecoratorManager {

  /**
  Selects the requested `Decorator`, if any, among the collection
  pointed by the decorator collection parameter `ext`.

  @return the requested `Decorator` or NULL if `ext` does not contain it
  */
  template<class Decorator, typename =
      std::enable_if_t<std::is_convertible<Decorator*, AbsDecorator*>::value>>
  static std::shared_ptr<const Decorator>
      selectExt(const std::shared_ptr<const IfDecorator> &ext) {
    std::shared_ptr<const Decorator> resExt =
          std::dynamic_pointer_cast<const Decorator>(ext);
    if(nullptr != resExt)
      return resExt;

    std::shared_ptr<const AbsDecorator> hostExt =
          std::dynamic_pointer_cast<const AbsDecorator>(ext);
    while(nullptr != hostExt) {
      resExt = std::dynamic_pointer_cast<const Decorator>(hostExt->nextExt);
      if(nullptr != resExt)
        return resExt;

      hostExt = std::dynamic_pointer_cast<const AbsDecorator>(hostExt->nextExt);
    }

    return nullptr;
  }
};

} // namespace rc

#endif // H_UTIL
