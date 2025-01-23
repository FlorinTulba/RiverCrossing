/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 @2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_SCENARIO_DETAILS
#define H_SCENARIO_DETAILS

#include "absSolution.h"

#include <cfloat>
#include <climits>

namespace rc {

/// Allows offering all these scenario information for inspection
class ScenarioDetails {
 public:
  ScenarioDetails() noexcept = default;
  ScenarioDetails(ScenarioDetails&&) noexcept = default;
  ScenarioDetails& operator=(ScenarioDetails&&) noexcept = default;
  ~ScenarioDetails() noexcept = default;

  ScenarioDetails(const ScenarioDetails&) = delete;
  void operator=(const ScenarioDetails&) = delete;

  /**
  The specific type of the initial state depends on the values above and on
  the Symbols Table.
  The algorithm will then preserve the specific state type for all the states
  created later.
  @return the appropriately decorated initial state
  */
  [[nodiscard]] std::unique_ptr<const sol::IState> createInitialState(
      const SymbolsTable& SymTb) const;

  /**
  Generates a decorated validator for raft / bridge configurations.
  It assumes the extra decoration for checking if there is someone able to row
  or not is added separately only when needed.
  Otherwise, it must add any necessary 'dynamic' validators, like allowedLoads
  */
  [[nodiscard]] std::shared_ptr<const cond::IContextValidator>
  createTransferValidator() const;

  /**
  Creates extensions for 'static' checks about each additional transfer
  constraint apart from the raft/bridge capacity. Example - the max raft/bridge
  load.

  Allows establishing whether a certain raft/bridge configuration satisfies
  the extension constraints of transfer.
  */
  void createTransferConstraintsExt();

  /**
  The raft/bridge configurations might have several additional details,
  like the max total load.

  This is unique_ptr because each raft/bridge configuration might have
  unique features and that state is not the same for all, thus not a shared_ptr
  */
  [[nodiscard]] std::unique_ptr<ent::IMovingEntitiesExt>
  createMovingEntitiesExt() const;

  [[nodiscard]] std::string toString() const;  ///< displays the content

  /// All mentioned entities (at least 3).
  /// shared as several classes keep this information
  std::shared_ptr<const ent::AllEntities> entities;

  /**
  Extensions for 'static' checks about each additional transfer constraint apart
  from the raft/bridge capacity. Example - the max raft/bridge load.

  Allows establishing whether a certain raft/bridge configuration satisfies
  the extension constraints of transfer.
  */
  std::unique_ptr<const cond::ITransferConstraintsExt> transferConstraintsExt;

  /**
  Raft/bridge conditions; Unique as it is always immediately used and not
  stored. When not explicitly specified, it gets a default instantiation that
  checks the capacity and the max load of the raft/bridge.
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
  /// shared as several classes keep this information
  std::shared_ptr<const cond::IValues<double>> allowedLoads;

  /**
  Overall max load of the raft/bridge.
  If provided, it needs to allow at least 2 entities (but not all of them)
  using the raft/bridge at the same time in order to have a feasible and
  non-trivial scenario.
  */
  double maxLoad{DBL_MAX};

  /**
  How many entities are allowed on the raft/bridge at once?

  Value that needs to be updated to be between 2 and the number of entities - 1,
  in order to have a feasible and non-trivial scenario.

  Even when the scenario description includes this information explicitly,
  it can be set to a lower value (still >= 2) based on the other constraints.

  When it is not specified, it gets set to the number of entities - 1.
  */
  unsigned capacity{UINT_MAX};

  /// Max duration for all the entities to reach the opposite bank
  unsigned maxDuration{UINT_MAX};
};

}  // namespace rc

namespace std {

inline auto& operator<<(auto& os, const rc::ScenarioDetails& sc) {
  os << sc.toString();
  return os;
}

}  // namespace std

#endif  // H_SCENARIO_DETAILS not defined
