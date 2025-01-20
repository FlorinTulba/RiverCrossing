/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_UTIL
#define H_UTIL

#include <concepts>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

#ifdef __cpp_lib_source_location
#include <source_location>
#else
#include <boost/assert/source_location.hpp>
#endif  // defined(__cpp_lib_source_location)

#include <gsl/pointers>

using namespace std::literals;

#ifdef UNIT_TESTING

constexpr bool UT{true};
#define PROTECTED public
#define PRIVATE public

#else  // UNIT_TESTING not defined

constexpr bool UT{};
#define PROTECTED protected
#define PRIVATE private

#endif  // UNIT_TESTING

/// Less typing when specifying container ranges
#define CBOUNDS(cont) std::cbegin(cont), std::cend(cont)
#define CRBOUNDS(cont) std::crbegin(cont), std::crend(cont)
#define BOUNDS(cont) std::begin(cont), std::end(cont)
#define RBOUNDS(cont) std::rbegin(cont), std::rend(cont)

// Access to source location information(file, line, col, function)
#ifdef __cpp_lib_source_location
#define HERE std::source_location::current()
#else
#define HERE BOOST_CURRENT_LOCATION
#endif  // defined(__cpp_lib_source_location)

/// Forwarding a checked (smart) pointer. Throwing provided exception with given
/// message if NULL
#define CP_EX_MSG(ptr, exc, msg) \
  ((ptr) ? (ptr) : (throw exc{HERE.function_name() + " - "s + (msg)}))
/// Forwarding a checked (smart) pointer. Throwing provided exception if NULL
#define CP_EX(ptr, exc) CP_EX_MSG((ptr), exc, "NULL value!"s)

/// Forwarding a checked (smart) pointer. Throwing invalid_argument with given
/// message if NULL
#define CP_MSG(ptr, msg) CP_EX_MSG((ptr), std::invalid_argument, (msg))

/// Forwarding a checked (smart) pointer. Throwing invalid_argument if NULL
#define CP(ptr) CP_EX((ptr), std::invalid_argument)

namespace rc {

/**
Takes the working directory and hopes
it is (a subfolder of) RiverCrossing directory.

@return RiverCrossing folder if found; otherwise an empty path
*/
[[nodiscard]] const std::filesystem::path projectFolder() noexcept;

/**
The template parameter represents a visitor displaying extended information
about a host object.

The extra information can be placed:

- partially in front of the previous details with ExtendedInfo::toString(false)
- and the rest at the end of the previous details with
ExtendedInfo::toString(true)

Ensures that these 2 calls to ExtendedInfo::toString(bool) wrap the block
displaying the previous details.
*/
template <class ExtendedInfo>
  requires requires(ExtendedInfo& i) {
    { i.toString(std::declval<bool>()) } -> std::same_as<std::string>;
  }
class ToStringManager {
 public:
  ToStringManager(const ExtendedInfo& ext_, std::ostringstream& oss_)
      : ext{&ext_}, oss{&oss_} {
    (*oss) << ext->toString(false);
  }

  ~ToStringManager() noexcept { (*oss) << ext->toString(true); }

  // Makes no sense to copy / move
  ToStringManager(const ToStringManager&) = delete;
  ToStringManager(ToStringManager&&) = delete;
  void operator=(const ToStringManager&) = delete;
  void operator=(ToStringManager&&) = delete;

  PROTECTED :

      gsl::not_null<const ExtendedInfo*>
          ext;
  gsl::not_null<std::ostringstream*> oss;
};

/**
Provides access to a certain decorator type within a hierarchy.
IfDecorator is the base interface of the entire hierarchy,
AbsDecorator is the base of the actual decorators.
*/
template <class AbsDecorator, class IfDecorator>
class DecoratorManager {
 public:
  /**
  Selects the requested `Decorator`, if any, among the collection
  pointed by the decorator collection parameter `ext`.

  @return the requested `Decorator` or NULL if `ext` does not contain it
  */
  template <class Decorator>
    requires std::convertible_to<Decorator*, AbsDecorator*>
  [[nodiscard]] static std::shared_ptr<const Decorator> selectExt(
      const std::shared_ptr<const IfDecorator>& ext) noexcept {
    std::shared_ptr<const Decorator> resExt{
        std::dynamic_pointer_cast<const Decorator>(ext)};
    if (resExt)
      return resExt;

    std::shared_ptr<const AbsDecorator> hostExt{
        std::dynamic_pointer_cast<const AbsDecorator>(ext)};
    while (hostExt) {
      // hostExt->nextExt is gsl::not_null<shared_ptr<...>>
      // get() takes the smart pointer out of not_null
      resExt =
          std::dynamic_pointer_cast<const Decorator>(hostExt->nextExt.get());
      if (resExt)
        return resExt;

      hostExt =
          std::dynamic_pointer_cast<const AbsDecorator>(hostExt->nextExt.get());
    }

    return {};
  }

  /**
  Selects the requested `Decorator`, if any, among the collection
  pointed by the decorator collection parameter `ext`.

  @return the requested `Decorator` or NULL if `ext` does not contain it
  */
  template <class Decorator>
    requires std::convertible_to<Decorator*, AbsDecorator*>
  [[nodiscard]] static const Decorator* selectExt(
      const IfDecorator* ext) noexcept {
    const Decorator* resExt{dynamic_cast<const Decorator*>(ext)};
    if (resExt)
      return resExt;

    const AbsDecorator* hostExt{dynamic_cast<const AbsDecorator*>(ext)};
    while (hostExt) {
      // hostExt->nextExt is unique_ptr<...>
      // The get() takes the naked pointer out of the smart pointer
      resExt = dynamic_cast<const Decorator*>(hostExt->nextExt.get());
      if (resExt)
        return resExt;

      hostExt = dynamic_cast<const AbsDecorator*>(hostExt->nextExt.get());
    }

    return {};
  }
};

}  // namespace rc

#endif  // H_UTIL
