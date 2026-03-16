/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.83) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2026 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_ABS_SOLUTION
#define H_ABS_SOLUTION

#include "configConstraint.h"
#include "util.h"

#include <cstddef>

#include <concepts>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <gsl/pointers>

#include <boost/core/demangle.hpp>

namespace rc {

// Forward declaration of a type used only as pointer within this header
// Avoids circular include dependency, by not including 'scenarioDetails.h'
class ScenarioDetails;

namespace ent {
// Forward declarations
class MovingEntities;
class BankEntities;
}  // namespace ent

namespace sol {

class IState;  // forward declaration

/// Allows State extensions
class IStateExt {
 public:
  virtual ~IStateExt() noexcept = default;

  IStateExt(const IStateExt&) = delete;
  IStateExt(IStateExt&&) = delete;
  IStateExt& operator=(const IStateExt&) = delete;
  IStateExt& operator=(IStateExt&&) noexcept = delete;

  /// Clones the State extension
  [[nodiscard]] virtual std::unique_ptr<const IStateExt> clone()
      const noexcept = 0;

  /// Validates the parameter state based on the constraints of the extension
  [[nodiscard]] virtual bool validate() const = 0;

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  [[nodiscard]] virtual bool isNotBetterThan(const IState&) const = 0;

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  [[nodiscard]] virtual std::shared_ptr<const IStateExt> extensionForNextState(
      const ent::MovingEntities&) const = 0;

  /**
  The browser visualizer shows various state information.
  The extensions may add some more.
  */
  [[nodiscard]] virtual std::string detailsForDemo() const { return {}; }

  // Display prefix extensions
  virtual void formatPrefixTo(FmtCtxIt&) const = 0;

  // Display suffix extensions
  virtual void formatSuffixTo(FmtCtxIt&) const = 0;

 protected:
  IStateExt() noexcept = default;
};

/// Neutral State extension, which does nothing
class DefStateExt final : public IStateExt {
 public:
  ~DefStateExt() noexcept override = default;

  DefStateExt(const DefStateExt&) = delete;
  DefStateExt(DefStateExt&&) = delete;
  DefStateExt& operator=(const DefStateExt&) = delete;
  DefStateExt& operator=(DefStateExt&&) noexcept = delete;

  /// Allows sharing the default instance
  [[nodiscard]] static const std::shared_ptr<const DefStateExt>&
  SHARED_INST() noexcept;

  /// Clones the State extension
  [[nodiscard]] std::unique_ptr<const IStateExt> clone() const noexcept final;

  /// Validates the parameter state based on the constraints of the extension
  [[nodiscard]] bool validate() const noexcept final { return true; }

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  [[nodiscard]] bool isNotBetterThan(const IState&) const noexcept final {
    return true;
  }

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  [[nodiscard]] std::shared_ptr<const IStateExt> extensionForNextState(
      const ent::MovingEntities&) const noexcept final;

  void formatPrefixTo(FmtCtxIt&) const noexcept final {}

  void formatSuffixTo(FmtCtxIt&) const noexcept final {}

  PRIVATE:
  DefStateExt() noexcept = default;
};

/// Base class for state extensions decorators.
class AbsStateExt : public IStateExt,

                    // provides the `selectExt` static method
                    public DecoratorManager<AbsStateExt, IStateExt> {
  // for accessing nextExt
  friend class DecoratorManager<AbsStateExt, IStateExt>;

 public:
  ~AbsStateExt() noexcept override = default;

  AbsStateExt(const AbsStateExt&) = delete;
  AbsStateExt(AbsStateExt&&) = delete;
  AbsStateExt& operator=(const AbsStateExt&) = delete;
  AbsStateExt& operator=(AbsStateExt&&) noexcept = delete;

  /// Clones the State extension
  [[nodiscard]] std::unique_ptr<const IStateExt> clone() const noexcept final;

  /// Validates the parameter state based on the constraints of the extension
  [[nodiscard]] bool validate() const final;

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  [[nodiscard]] bool isNotBetterThan(const IState& s2) const final;

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  [[nodiscard]] std::shared_ptr<const IStateExt> extensionForNextState(
      const ent::MovingEntities& me) const final;

  /**
  The browser visualizer shows various state information.
  The extensions may add some more.
  */
  [[nodiscard]] std::string detailsForDemo() const final;

  void formatPrefixTo(FmtCtxIt&) const final;

  void formatSuffixTo(FmtCtxIt&) const final;

 protected:
  AbsStateExt(const rc::ScenarioDetails& info_,
              const std::shared_ptr<const IStateExt>& nextExt_) noexcept;

  /// Clones the State extension
  [[nodiscard]] virtual std::unique_ptr<const IStateExt> _clone(
      const std::shared_ptr<const IStateExt>& nextExt_) const noexcept = 0;

  /// Validates the parameter state based on the constraints of the extension
  [[nodiscard]] virtual bool _validate() const { return true; }

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension.
  @throw logic_error for invalid extension
  */
  [[nodiscard]] virtual bool _isNotBetterThan(const IState&) const {
    return true;
  }

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  [[nodiscard]] virtual std::shared_ptr<const IStateExt> _extensionForNextState(
      const ent::MovingEntities&,
      const std::shared_ptr<const IStateExt>& fromNextExt) const = 0;

  /**
  The browser visualizer shows various state information.
  The extensions may add some more.
  */
  [[nodiscard]] virtual std::string _detailsForDemo() const { return {}; }

  virtual void _formatPrefixTo(FmtCtxIt&) const {}

  virtual void _formatSuffixTo(FmtCtxIt&) const {}

  // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes) :
  // Easier to use in subclasses when protected
  gsl::not_null<const rc::ScenarioDetails*> info;
  gsl::not_null<std::shared_ptr<const IStateExt>> nextExt;
  // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};

/// A state during solving the scenario
class IState {
 public:
  virtual ~IState() noexcept = default;

  IState& operator=(const IState&) = delete;
  IState& operator=(IState&&) noexcept = delete;

  [[nodiscard]] virtual const ent::BankEntities& leftBank() const noexcept = 0;
  [[nodiscard]] virtual const ent::BankEntities& rightBank() const noexcept = 0;

  /// Is the direction of next move from left to right?
  [[nodiscard]] virtual bool nextMoveFromLeft() const noexcept = 0;

  /// Provides access to the extensions of this IState
  [[nodiscard]] virtual gsl::not_null<std::shared_ptr<const IStateExt>>
  getExtension() const noexcept = 0;

  /// @return the next state of the algorithm when moving `movedEnts` to the
  /// opposite bank
  [[nodiscard]] virtual std::unique_ptr<const IState> next(
      const ent::MovingEntities& movedEnts) const = 0;

  /// @return true if the `other` state is the same or a better version of this
  /// state
  [[nodiscard]] virtual bool handledBy(const IState& other) const = 0;

  /// @return true if the provided examinedStates do not cover already this
  /// state
  [[nodiscard]] virtual bool handledBy(
      const std::vector<std::unique_ptr<const IState>>& examinedStates)
      const = 0;

  /// @return true if this state conforms to all constraints that apply to it
  [[nodiscard]] virtual bool valid(
      const cond::ConfigConstraints* bankConstraints) const = 0;

  /// Clones this state
  [[nodiscard]] virtual std::unique_ptr<const IState> clone()
      const noexcept = 0;

  /// Formats the state. If the bool param is true, it shows the direction of
  /// the next move
  virtual void formatTo(FmtCtxIt&, bool) const = 0;

 protected:
  IState() noexcept = default;

  IState(const IState&) = default;
  IState(IState&&) noexcept = default;
};

/// The moved entities and the resulted state
class IMove {
 public:
  virtual ~IMove() noexcept = default;

  [[nodiscard]] virtual const ent::MovingEntities& movedEntities()
      const noexcept = 0;
  [[nodiscard]] virtual std::shared_ptr<const IState> resultedState()
      const noexcept = 0;

  /**
  0-based index of the move
  or UINT_MAX for the fake empty move producing the initial state
  */
  [[nodiscard]] virtual unsigned index() const noexcept = 0;

  /// Formats the move. When the bool is true, it shows next move's direction
  virtual void formatTo(FmtCtxIt&, bool) const = 0;

 protected:
  IMove() noexcept = default;

  IMove(const IMove&) = default;
  IMove(IMove&&) noexcept = default;
  IMove& operator=(const IMove&) = default;
  IMove& operator=(IMove&&) noexcept = default;
};

/**
The current states from the path of the backtracking algorithm.
If the algorithm finds a solution, the attempt will express
the path required to reach that solution.
*/
class IAttempt {
 public:
  virtual ~IAttempt() noexcept = default;

  IAttempt(const IAttempt&) = delete;
  IAttempt(IAttempt&&) = delete;
  IAttempt& operator=(const IAttempt&) = delete;
  IAttempt& operator=(IAttempt&&) noexcept = delete;

  /// First call sets the initial state. Next calls are the actually moves.
  virtual void append(const IMove& move) = 0;

  /// Removes last move, if any left
  virtual void pop() noexcept = 0;

  /// Ensures the attempt won't show corrupt data after a difficult to trace
  /// exception
  virtual void clear() noexcept = 0;

  [[nodiscard]] virtual std::shared_ptr<const IState> initialState()
      const noexcept = 0;

  /// Number of moves from the current path
  [[nodiscard]] virtual size_t length() const noexcept = 0;

  /**
  @return n-th valid move
  @throw out_of_range for an invalid move index
  */
  [[nodiscard]] virtual const IMove& move(size_t idx) const = 0;

  /**
  @return last performed move or at least the initial fake empty move
     that set the initial state
  @throw out_of_range when there is not even the fake initial one
  */
  [[nodiscard]] virtual const IMove& lastMove() const = 0;

  /// @return true for a solution path
  [[nodiscard]] virtual bool isSolution() const noexcept = 0;

  virtual void formatTo(FmtCtxIt&) const = 0;

 protected:
  IAttempt() noexcept = default;
};

}  // namespace sol
}  // namespace rc

namespace std {

// Formatter for classes derived from IState / IMove
template <class T>
  requires derived_from<T, rc::sol::IState> || derived_from<T, rc::sol::IMove>
class formatter<T> {  // NOLINT(bugprone-std-namespace-modification) : Required
                      // this way
 public:
  /// Parses the format specifier for the class. It can contain only the
  /// option "showNextMoveDir" for showing the direction of the next move.
  constexpr auto parse(auto& ctx) {
    constexpr string_view directionOption{"showNextMoveDir"};

    auto it{ctx.begin()};
    const auto itEnd{ctx.end()};
    string_view s(it, itEnd);
    if (s.starts_with(directionOption)) {
      showNextMoveDir = true;
      it += size(directionOption);
    } else {
      showNextMoveDir = {};
    }

    if (it != itEnd && *it != '}') {
      throw format_error{
          std::format("Invalid format option for class `{}`: `{}`!",
                      boost::core::demangle(typeid(T).name()), s)};
    }

    return it;
  }

  /// Formats the class according to the provided format specifier. If the
  /// option "showNextMoveDir" is provided, it shows next move's direction.
  auto format(const T& obj, auto& ctx) const {
    auto outIt{ctx.out()};
    obj.formatTo(outIt, showNextMoveDir);  // changes outIt
    return outIt;
  }

 private:
  bool showNextMoveDir{};
};

}  // namespace std

#endif  // !H_ABS_SOLUTION
