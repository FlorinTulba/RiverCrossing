/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_ABS_SOLUTION
#define H_ABS_SOLUTION

#include "scenarioDetails.h"
#include "util.h"

#include <iostream>
#include <memory>

namespace rc {

namespace ent {

// Forward declarations
class BankEntities;
class MovingEntities;

} // namespace ent

namespace cond {

class ConfigConstraints; // forward declaration

} // namespace ent

namespace sol {

struct IState; // forward declaration

/// Allows State extensions
struct IStateExt /*abstract*/ {
  virtual ~IStateExt()/* = 0 */{}

  /// Clones the State extension
  virtual std::shared_ptr<const IStateExt> clone() const = 0;

  /// Validates the parameter state based on the constraints of the extension
  virtual bool validate() const = 0;

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  virtual bool isNotBetterThan(const IState&) const = 0;

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  virtual std::shared_ptr<const IStateExt>
    extensionForNextState(const ent::MovingEntities&) const = 0;

  /**
  Display either only suffix (most of them), or only prefix extensions.
  It needs to be called before (with param false) and after (with param true)
  the state information
  */
  virtual std::string toString(bool suffixesInsteadOfPrefixes = true) const = 0;
};

/// Neutral State extension, which does nothing
struct DefStateExt final : IStateExt {
  /// Allows sharing the default instance
  static const std::shared_ptr<const DefStateExt>& INST();

  /// Clones the State extension
  std::shared_ptr<const IStateExt> clone() const override final;

  /// Validates the parameter state based on the constraints of the extension
  bool validate() const override final {return true;}

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  bool isNotBetterThan(const IState&) const override final {return true;}

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  std::shared_ptr<const IStateExt>
      extensionForNextState(const ent::MovingEntities&) const override final;

  std::string toString(bool suffixesInsteadOfPrefixes/* = true*/) const override final {
    return "";
  }

    #ifndef UNIT_TESTING // leave ctor public only for Unit tests
protected:
    #endif

  DefStateExt() {}
};

/**
Base class for state extensions decorators.
Some of the new virtual methods are abstract and must be implemented
by every derived class.
*/
class AbsStateExt /*abstract*/ :
      public IStateExt,
      public DecoratorManager<AbsStateExt, IStateExt> { // provides `selectExt` static method

  friend struct DecoratorManager<AbsStateExt, IStateExt>; // for accessing nextExt

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  const ScenarioDetails &info;
  std::shared_ptr<const IStateExt> nextExt;

  AbsStateExt(const ScenarioDetails &info_,
              const std::shared_ptr<const IStateExt> &nextExt_);

  /// Clones the State extension
  virtual std::shared_ptr<const IStateExt>
    _clone(const std::shared_ptr<const IStateExt> &nextExt_) const = 0;

  /// Validates the parameter state based on the constraints of the extension
  virtual bool _validate() const {return true;}

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  virtual bool _isNotBetterThan(const IState&) const {return true;}

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  virtual std::shared_ptr<const IStateExt>
      _extensionForNextState(const ent::MovingEntities&,
                             const std::shared_ptr<const IStateExt> &fromNextExt)
                      const = 0;

  virtual std::string _toString(bool suffixesInsteadOfPrefixes = true) const {
    return "";
  }

public:
  /// Clones the State extension
  std::shared_ptr<const IStateExt> clone() const override final;

  /// Validates the parameter state based on the constraints of the extension
  bool validate() const override final ;

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  bool isNotBetterThan(const IState &s2) const override final;

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  std::shared_ptr<const IStateExt>
      extensionForNextState(const ent::MovingEntities &me) const override final;

  std::string toString(bool suffixesInsteadOfPrefixes/* = true*/) const override final;
};

/// A state during solving the scenario
struct IState /*abstract*/ {
  virtual ~IState() /*= 0*/ {}

  virtual const ent::BankEntities& leftBank() const = 0;
  virtual const ent::BankEntities& rightBank() const = 0;

  /// Is the direction of next move from left to right?
  virtual bool nextMoveFromLeft() const = 0;

  /// Provides access to the extensions of this IState
  virtual std::shared_ptr<const IStateExt> getExtension() const = 0;

  /// @return the next state of the algorithm when moving `movedEnts` to the opposite bank
  virtual std::unique_ptr<const IState>
    next(const ent::MovingEntities &movedEnts) const = 0;

  /// @return true if the `other` state is the same or a better version of this state
  virtual bool handledBy(const IState &other) const = 0;

  /// @return true if the provided examinedStates do not cover already this state
  virtual bool
    handledBy(const std::vector<std::unique_ptr<const IState>> &examinedStates)
        const = 0;

  /// @return true if this state conforms to all constraints that apply to it
  virtual bool
    valid(const std::unique_ptr<const cond::ConfigConstraints> &bankConstraints)
        const = 0;

  /// Clones this state
  virtual std::unique_ptr<const IState> clone() const = 0;

  virtual std::string toString() const = 0;
};

/// The moved entities and the resulted state
struct IMove /*abstract*/ {
  virtual ~IMove() /*= 0*/ {}

  virtual const ent::MovingEntities& movedEntities() const = 0;
  virtual std::shared_ptr<const IState> resultedState() const = 0;
  virtual unsigned index() const = 0; ///< 0-based index of the move

  virtual std::string toString() const = 0;
};

/**
The current states from the path of the backtracking algorithm.
If the algorithm finds a solution, the attempt will express
the path required to reach that solution.
*/
struct IAttempt /*abstract*/ {
  virtual ~IAttempt() /*= 0*/ {}

  virtual std::shared_ptr<const IState> initialState() const = 0;
  virtual size_t length() const = 0; ///< number of moves from the current path
  virtual const IMove& move(size_t idx) const = 0; ///< n-th move

  /**
  @return size of the symmetric difference between the entities from
    the target left bank and current left bank
  */
  virtual size_t distToSolution() const = 0;
  virtual bool isSolution() const = 0; ///< @return true for a solution path

  virtual std::string toString() const = 0;
};

} // namespace sol
} // namespace rc

namespace std {

std::ostream& operator<<(std::ostream &os, const rc::sol::IState &st);

std::ostream& operator<<(std::ostream &os, const rc::sol::IMove &m);

std::ostream& operator<<(std::ostream &os, const rc::sol::IAttempt &attempt);

} // namespace std

#endif // H_ABS_SOLUTION not defined
