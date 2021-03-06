/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 @2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_SCENARIO_DETAILS
#define H_SCENARIO_DETAILS

#include "symbolsTable.h"

#include <memory>
#include <vector>
#include <cfloat>
#include <climits>

namespace rc {

namespace ent {

// Forward declarations
class AllEntities;
struct IMovingEntitiesExt;

} // namespace ent

namespace sol {

// Forward declarations
struct IState;

} // namespace sol

namespace cond {

// Forward declarations
class ConfigConstraints;
class TransferConstraints;
class ConfigurationsTransferDuration;
struct IContextValidator;
struct ITransferConstraintsExt;

template<typename>
class IValues;

} // namespace cond

/// Allows offering all these scenario information for inspection
struct ScenarioDetails {
  // shared as several classes keep this information
  std::shared_ptr<const ent::AllEntities> entities; ///< all mentioned entities (at least 3)

  /**
  Raft/bridge conditions; Unique as it is always immediately used and not stored.
  When not explicitly specified, it gets a default instantiation
  that checks the capacity and the max load of the raft/bridge.
  */
  std::unique_ptr<const cond::TransferConstraints> transferConstraints;

  /// Banks conditions; Unique as it is always immediately used and not stored
  std::unique_ptr<const cond::ConfigConstraints> banksConstraints;

  /**
  Crossing durations for all possible raft/bridge configurations.
  Pairs like (TransferConstraints, duration).
  All valid configurations should be mentioned and have an associated duration.
  */
  std::vector<cond::ConfigurationsTransferDuration> ctdItems;

  /// Expression for limiting the load of the raft/bridge at each step
  std::shared_ptr<const cond::IValues<double>> allowedLoads; // shared as several classes keep this information

  /**
  Overall max load of the raft/bridge.
  If provided, it needs to allow at least 2 entities (but not all of them)
  using the raft/bridge at the same time in order to have a feasible and
  non-trivial scenario.
  */
  double maxLoad = DBL_MAX;

  /**
  How many entities are allowed on the raft/bridge at once?

  Value that needs to be updated to be between 2 and the number of entities - 1,
  in order to have a feasible and non-trivial scenario.

  Even when the scenario description includes this information explicitly,
  it can be set to a lower value (still >= 2) based on the other constraints.

  When it is not specified, it gets set to the number of entities - 1.
  */
  unsigned capacity = UINT_MAX;

  /// Max duration for all the entities to reach the opposite bank
  unsigned maxDuration = UINT_MAX;

  /**
  The specific type of the initial state depends on the values above and on
  the Symbols Table.
  The algorithm will then preserve the specific state type for all the states
  created later.
  @return the appropriately decorated initial state
  */
  std::unique_ptr<const sol::IState> createInitialState(const SymbolsTable &SymTb) const;

  /**
  Generates a decorated validator for raft / bridge configurations.
  It assumes the extra decoration for checking if there is someone able to row
  or not is added separately only when needed.
  Otherwise, it must add any necessary 'dynamic' validators, like allowedLoads
  */
  std::shared_ptr<const cond::IContextValidator> createTransferValidator() const;

  /**
  Creates extensions for 'static' checks about each additional transfer constraint
  apart from the raft/bridge capacity. Example - the max raft/bridge load.

  Allows establishing whether a certain raft/bridge configuration satisfies
  the extension constraints of transfer.
  */
  std::shared_ptr<const cond::ITransferConstraintsExt>
        createTransferConstraintsExt() const;

  /**
  The raft/bridge configurations might have several additional details,
  like the max total load.

  This is unique_ptr because each raft/bridge configuration might have
  unique features and that state is not the same for all, thus not a shared_ptr
  */
  std::unique_ptr<ent::IMovingEntitiesExt> createMovingEntitiesExt() const;

	std::string toString() const; ///< displays the content
};

} // namespace rc

namespace std {

std::ostream& operator<<(std::ostream &os, const rc::ScenarioDetails &sc);

} // namespace std

#endif // H_SCENARIO_DETAILS not defined
