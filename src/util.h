/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.83) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2026 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_UTIL
#define H_UTIL

#include "warnings.h"

#include <cstdio>

#include <concepts>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <ranges>
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

#define PROTECTED public
#define PRIVATE public

#else  // UNIT_TESTING not defined

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
#define LOC_INFO std::source_location
#define HERE std::source_location::current()

#else
#define LOC_INFO boost::source_location
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
Invokes 'f' callable. If 'f' throws, terminate() is called, simulating a
noexcept 'f'. Additionally, makeNoexcept logs a thrown exception.

@param f the callable to invoke
@param where location information used for exception reporting
@return the result of 'f' if it does not throw
*/
template <typename Func>
  requires(!noexcept(std::invoke(std::declval<Func>())))
decltype(auto) makeNoexcept(Func&& f, const LOC_INFO where = HERE) noexcept {
  // Default error message for the exception thrown by 'f', if any
  const char* errMsg{"unknown exception"};

  try {
    return std::invoke(std::forward<Func>(f));

  } catch (const std::exception& e) {
    if (e.what())
      errMsg = e.what();

  } catch (...) {
    // Empty because errMsg is already set to "unknown exception"
  }

  MUTE_UNSAFE_BUFF_USE_WARN
  std::fprintf(stderr,
               "\nFatal Error: Noexcept-marked code threw!\n"
               "Exception: %s\n"
               "Function: %s\n"
               "File: %s:%u\n",
               errMsg, where.function_name(), where.file_name(), where.line());
  UNMUTE_WARNING

  std::terminate();
}

/**
Copy ctors are not noexcept by default.
This function allows making a copy of an object whose copy ctor is not noexcept,
by invoking it inside makeNoexcept. It reports the exception thrown
by the copy ctor, if any, and terminates the program,
since the copy operation is expected to succeed.

@param val the value to copy
@param where location information used for exception reporting
@return a copy of 'val' if the copy ctor does not throw; otherwise, the program
terminates.
*/
template <typename T>
decltype(auto) makeCopyNoexcept(const T& val,
                                const LOC_INFO where = HERE) noexcept {
  if constexpr (std::is_nothrow_copy_constructible_v<T>)
    return val;
  else
    // Keep the trailing result type to force the copy ctor inside makeNoexcept
    return makeNoexcept([&val]() -> T { return val; }, where);
}

/**
Move ctors should be noexcept.
Notable exceptions: std::set and std::map in MSVC, but not in GCC.
This function allows moving an object whose move ctor is not noexcept,
by invoking it inside makeNoexcept. It reports the exception thrown
by the move ctor, if any, and terminates the program,
since the move operation is expected to succeed.

@param val the value to move
@param where location information used for exception reporting
@return 'val' as rvalue if the move ctor does not throw; otherwise, the program
terminates.
*/
template <typename T>
decltype(auto) makeMoveNoexcept(T&& val, const LOC_INFO where = HERE) noexcept {
  if constexpr (std::is_nothrow_move_constructible_v<T>)
    return std::forward<T>(val);
  else
    return makeNoexcept([&val]() -> T { return std::move(val); }, where);
}

/**
Takes the working directory and hopes
it is (a subfolder of) RiverCrossing directory.

@return RiverCrossing folder if found; otherwise an empty path
*/
[[nodiscard]] std::filesystem::path projectFolder() noexcept;

/// Delimiters when displaying a container
struct ContViewDelims {
  std::string_view before{};
  std::string_view between{};
  std::string_view after{};
};

/// Helper for displaying a container
template <class FwCont, class Proj = std::identity>
  requires std::ranges::forward_range<FwCont>
class ContView {
 public:
  explicit ContView(
      const FwCont& cont,
      const ContViewDelims& delims_ = {},
      const Proj& proj_ =
          {}) noexcept(std::is_nothrow_copy_constructible_v<Proj> &&
                       std::is_nothrow_default_constructible_v<Proj>)
      : pCont{&cont},
        delims{delims_},
        proj{proj_} {}

  ContView(const ContView&) = default;
  ContView(ContView&&) noexcept = delete;
  ~ContView() noexcept = default;

  ContView& operator=(const ContView&) = delete;
  ContView& operator=(ContView&&) noexcept = delete;

  std::string toString() const {
    std::ostringstream oss;
    oss << delims.before;

    // Using begin() and end() since FwCont.empty() and FwCont.size()
    // might not be available.
    const auto itBeg{pCont->begin()}, itEnd{pCont->end()};
    if (itBeg != itEnd) {
      oss << proj(*pCont->begin());

      // The saved view prevents getting -Wdangling-reference in GCC
      for (auto view{*pCont | std::views::drop(1)}; auto&& elem : view)
        oss << delims.between << proj(elem);
    }

    oss << delims.after;
    return oss.str();
  }

 private:
  gsl::not_null<const FwCont*> pCont;  // not_null is not movable
  ContViewDelims delims;
  Proj proj;
};

template <class FwCont, class Proj>
ContView(const FwCont&, const ContViewDelims&, const Proj&)
    -> ContView<FwCont, Proj>;

template <class FwCont>
ContView(const FwCont&) -> ContView<FwCont, std::identity>;

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
      : ext{&ext_},
        oss{&oss_} {
    (*oss) << ext->toString(false);
  }

  ~ToStringManager() noexcept try {
    (*oss) << ext->toString(true);
  } catch (...) {
    // Ensures a noexcept dtor
  }

  // Makes no sense to copy / move
  ToStringManager(const ToStringManager&) = delete;
  ToStringManager(ToStringManager&&) = delete;
  ToStringManager& operator=(const ToStringManager&) = delete;
  ToStringManager& operator=(ToStringManager&&) noexcept = delete;

  PROTECTED :
  gsl::not_null<const ExtendedInfo*> ext;
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

namespace std {

template <class Cont, class Proj>
inline auto& operator<<(auto& os, const rc::ContView<Cont, Proj>& cont) {
  return os << cont.toString();
}

}  // namespace std

#endif  // H_UTIL
