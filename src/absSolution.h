/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_ABS_SOLUTION
#define H_ABS_SOLUTION

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

  /// Ensures that toString() from above is called twice, as mentioned
  class ToStringManager {

      #ifdef UNIT_TESTING // leave fields public for Unit tests
  public:
      #else // keep fields protected otherwise
  protected:
      #endif

    const IStateExt &ext;
    std::ostringstream &oss;

  public:

    ToStringManager(const IStateExt &ext_, std::ostringstream &oss_) :
        ext(ext_), oss(oss_) {
      oss<<ext.toString(false);
    }
    ~ToStringManager() {
      oss<<ext.toString();
    }
  };
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

/// Allows performing canRow, allowedLoads and other checks on raft/bridge configurations
struct IContextValidator /*abstract*/ {
  virtual ~IContextValidator()/* = 0 */{}

  /// @return true if `ents` is a valid raft/bridge configuration within `st` context
  virtual bool validate(const ent::MovingEntities &ents,
                        const SymbolsTable &st) const = 0;
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
