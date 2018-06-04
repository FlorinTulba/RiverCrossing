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

class ConfigurationsTransferDuration;

} // namespace cond

namespace sol {

/// A state during solving the scenario
struct IState /*abstract*/ {
  virtual ~IState() /*= 0*/ {}

  virtual const ent::BankEntities& leftBank() const = 0;
  virtual const ent::BankEntities& rightBank() const = 0;
  virtual unsigned time() const = 0; ///< the moment this state is reached

  /// Is the direction of next move from left to right?
  virtual bool nextMoveFromLeft() const = 0;

  /// @return the next state of the algorithm when moving `movedEnts` to the opposite bank
  virtual std::unique_ptr<const IState>
    next(const ent::MovingEntities &movedEnts,
         const std::vector<cond::ConfigurationsTransferDuration> &ctdItems = {})
      const = 0;

  /// @return true if the `other` state is the same or a better version of this state
  virtual bool handledBy(const IState &other) const = 0;

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
