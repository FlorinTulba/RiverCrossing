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

#include <cstddef>
#include <cstdio>

#include <algorithm>
#include <concepts>
#include <exception>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#ifdef __cpp_lib_source_location
#include <source_location>

#else
#include <boost/assert/source_location.hpp>
#endif  // defined(__cpp_lib_source_location)

#include <gsl/pointers>

#ifdef UNIT_TESTING

#define PRIVATE public

#else  // UNIT_TESTING not defined

#define PRIVATE private

#endif  // UNIT_TESTING

// NOLINTBEGIN(cppcoreguidelines-macro-usage) : Less typing when specifying
// container ranges
#define CBOUNDS(cont) std::cbegin(cont), std::cend(cont)
#define BOUNDS(cont) std::begin(cont), std::end(cont)
// NOLINTEND(cppcoreguidelines-macro-usage)

// Access to source location information(file, line, col, function)
#ifdef __cpp_lib_source_location
#define LOC_INFO std::source_location
#define HERE std::source_location::current()

#else
#define LOC_INFO boost::source_location
#define HERE BOOST_CURRENT_LOCATION
#endif  // defined(__cpp_lib_source_location)

/// Concept for pointer-like types: raw pointers, smart pointers, and nullptr_t
template <typename T>
concept PointerLike =
    std::same_as<T, std::nullptr_t> || std::is_pointer_v<T> || requires(T ptr) {
      { ptr == nullptr };
      { *ptr };
      { ptr.operator->() };
    };

/// Forwarding a checked (smart) pointer. Throwing provided exception with given
/// message if NULL
template <class Exc = std::invalid_argument>
  requires std::derived_from<Exc, std::exception>
inline decltype(auto) throwIfNull(PointerLike auto&& ptr,
                                  const std::string& msg = "NULL value!",
                                  const LOC_INFO& where = HERE) {
  if (ptr)
    return std::forward<decltype(ptr)>(ptr);

  throw Exc{std::format("{} - {}", where.function_name(), msg)};
}

using FmtCtxIt = std::format_context::iterator;

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
decltype(auto) makeNoexcept(Func&& f, const LOC_INFO& where = HERE) noexcept {
  // Default error message for the exception thrown by 'f', if any
  const char* errMsg{"unknown exception"};

  try {
    return std::invoke(std::forward<Func>(f));

  } catch (const std::exception& e) {
    if (e.what())
      errMsg = e.what();

  } catch (...) {  // NOLINT(bugprone-empty-catch) : errMsg is already set to
                   // "unknown exception"
  }

  MUTE_UNSAFE_BUFF_USE_WARN
  // NOLINTBEGIN(cppcoreguidelines-pro-type-vararg,modernize-use-std-print) :
  // Call unlikely to throw (cout/print could invoke allocators and fail)
  std::fprintf(stderr,
               "\nFatal Error: Noexcept-marked code threw!\n"
               "Exception: %s\n"
               "Function: %s\n"
               "File: %s:%u\n",
               errMsg, where.function_name(), where.file_name(), where.line());
  // NOLINTEND(cppcoreguidelines-pro-type-vararg,modernize-use-std-print)
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
                                const LOC_INFO& where = HERE) noexcept {
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
decltype(auto) makeMoveNoexcept(T&& val,
                                const LOC_INFO& where = HERE) noexcept {
  if constexpr (std::is_nothrow_move_constructible_v<T>)
    return std::forward<T>(val);
  else
    return makeNoexcept(
        [&val]() -> T {
          // NOLINTBEGIN(bugprone-move-forwarding-reference) : Forcing the move
          // here to catch and report an unlikely exception caused by it
          return std::move(val);
          // NOLINTEND(bugprone-move-forwarding-reference)
        },
        where);
}

/**
Takes the working directory and hopes
it is (a subfolder of) RiverCrossing directory.

@return RiverCrossing folder if found; otherwise an empty path
*/
[[nodiscard]] std::filesystem::path projectFolder() noexcept;

/// Helper for displaying a container
template <class FwCont, class Proj = std::identity>
  requires std::ranges::forward_range<FwCont>
class ContView {
 public:
  explicit constexpr ContView(
      const FwCont& cont,
      std::string_view sep_ = {},
      const Proj& proj_ =
          {}) noexcept(std::is_nothrow_copy_constructible_v<Proj> &&
                       std::is_nothrow_default_constructible_v<Proj>)
      : pCont{&cont},
        sep{sep_},
        proj{proj_} {}

  ContView(const ContView&) = default;
  ContView(ContView&&) noexcept = delete;
  ~ContView() noexcept = default;

  ContView& operator=(const ContView&) = delete;
  ContView& operator=(ContView&&) noexcept = delete;

  void formatTo(FmtCtxIt& outIt) const {
    using namespace std;

    // Using begin() and end() since FwCont.empty() and FwCont.size()
    // might not be available.
    if (const auto itFirst{begin(*pCont)}; itFirst != end(*pCont)) {
      outIt = format_to(outIt, "{}", proj(*itFirst));

      // The saved view prevents getting -Wdangling-reference in GCC
      for (auto rest{*pCont | views::drop(1)}; auto&& elem : rest)
        outIt = format_to(outIt, "{}{}", sep, proj(elem));
    }
  }

 private:
  gsl::not_null<const FwCont*> pCont;  // not_null is not movable
  std::string_view sep;
  Proj proj;
};

template <class FwCont, class Proj>
ContView(const FwCont&, std::string_view, const Proj&)
    -> ContView<FwCont, Proj>;

template <class FwCont>
ContView(const FwCont&) -> ContView<FwCont, std::identity>;

/// Concept for classes which can call 'formatTo(FmtCtxIt&)'
template <class T>
concept HasFormatTo = requires(const T& obj) {
  { obj.formatTo(std::declval<FmtCtxIt&>()) } -> std::same_as<void>;
};

/**
Class used within formatTo(FmtCtxIt) methods to display extra information
about the host object.

The template parameter represents a visitor displaying extended
information about a host object.

The extra information can be placed:
- in front of the previous details with ExtendedInfo::formatPrefixTo(outIt)
- at the end of the previous details with ExtendedInfo::formatSuffixTo(outIt)
*/
template <class ExtendedInfo>
  requires requires(ExtendedInfo& i) {
    { i.formatPrefixTo(std::declval<FmtCtxIt&>()) } -> std::same_as<void>;
    { i.formatSuffixTo(std::declval<FmtCtxIt&>()) } -> std::same_as<void>;
  }
class InfoWrapper {
 public:
  InfoWrapper(const ExtendedInfo& ext_, FmtCtxIt& it) : ext{&ext_}, pIt{&it} {
    ext_.formatPrefixTo(it);
  }

  ~InfoWrapper() noexcept try {
    ext->formatSuffixTo(*pIt);
  } catch (...) {  // NOLINT(bugprone-empty-catch) : Ensures a noexcept dtor
  }

  // Makes no sense to copy / move
  InfoWrapper(const InfoWrapper&) = delete;
  InfoWrapper(InfoWrapper&&) = delete;
  InfoWrapper& operator=(const InfoWrapper&) = delete;
  InfoWrapper& operator=(InfoWrapper&&) noexcept = delete;

 private:
  gsl::not_null<const ExtendedInfo*> ext;
  gsl::not_null<FmtCtxIt*> pIt;
};

template <class ExtendedInfo>
InfoWrapper(const ExtendedInfo&, FmtCtxIt&) -> InfoWrapper<ExtendedInfo>;

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

 private:
  /// Class only for friends (AbsDecorator - derived from DecoratorManager)
  constexpr DecoratorManager() noexcept = default;
  friend AbsDecorator;
};

}  // namespace rc

namespace std {

/// Formatter for classes able to call 'formatTo(FmtCtxIt&)'
template <rc::HasFormatTo T>
class formatter<T> {  // NOLINT(bugprone-std-namespace-modification) : Required
                      // this way
 public:
  constexpr auto parse(auto& ctx) { return ctx.begin(); }

  auto format(const T& obj, auto& ctx) const {
    auto outIt = ctx.out();
    obj.formatTo(outIt);  // the method updates outIt
    return outIt;
  }
};

}  // namespace std

#endif  // H_UTIL
