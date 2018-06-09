/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_SCENARIO
#define H_SCENARIO

#include "entitiesManager.h"
#include "configConstraint.h"
#include "absSolution.h"

namespace rc {

/**
Data and the solution for a river crossing puzzle:
- entities to move to the opposite bank
- various constraints about the raft / bridge or
  about which entities can be on a bank
*/
class Scenario {
public:
  /// Allows offering all these details to chosen friends
  struct Details {
    // shared as several classes keep this information
    std::shared_ptr<const ent::AllEntities> entities; ///< all mentioned entities (at least 3)

    /// Raft/bridge conditions; Unique as it is always immediately used and not stored
    std::unique_ptr<const cond::TransferConstraints> _transferConstraints;

    /// Banks conditions; Unique as it is always immediately used and not stored
    std::unique_ptr<const cond::ConfigConstraints> _banksConstraints;

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
    double _maxLoad = DBL_MAX;

    /**
    How many entities are allowed on the raft/bridge at once?

    Value that needs to be updated to be between 2 and the number of entities - 1,
    in order to have a feasible and non-trivial scenario.

    Even when the scenario description includes this information explicitly,
    it can be set to a lower value (still >= 2) based on the other constraints.

    When it is not specified, it gets set to the number of entities - 1.
    */
    unsigned _capacity = UINT_MAX;

    /// Max duration for all the entities to reach the opposite bank
    unsigned _maxDuration = UINT_MAX;

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
    std::shared_ptr<const sol::IContextValidator> createTransferValidator() const;
  };

  /**
  Results from exploring the scenario.

  There is an associated operator<<. Since the fields are public, the operator<<
  does not appear here as friend.
  */
  struct Results {
    /// The solution or an unsuccessful attempt
    std::shared_ptr<const sol::IAttempt> attempt;

    /// All the configurations identified as closest to the target left bank
    std::vector<ent::BankEntities> closestToTargetLeftBank;

    /// Length of the longest investigated attempt
    size_t longestInvestigatedPath = 0ULL;

    /// Count of total investigated states
    size_t investigatedStates = 0ULL;
  };

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

	std::string descr;	///< provided description of the scenario

	Details details; ///< relevant details of the scenario

  Results results; ///< the results

	bool investigated = false; ///< ensures solving is performed only once

public:
	/**
  Builds a scenario based on the input from the provided stream.
  If solveNow is true, it attempts to also solve this scenario
  @throw domain_error if there is a problem with the given scenario
	*/
	Scenario(std::istream &&scenarioStream, bool solveNow = false);
	Scenario(const Scenario&) = delete;
	Scenario(Scenario&&) = default;
	void operator=(const Scenario&) = delete;
	Scenario& operator=(Scenario&&) = default;

	/// Provided description of the scenario
	const std::string& description() const;

	/**
  Solves the scenario if possible.
  Subsequent calls use the obtained attempt / solution.

  @return the solution or an unsuccessful attempt
	*/
	const Results& solution();

	std::string toString() const; ///< data apart from the description
};

} // namespace rc

namespace std {

std::ostream& operator<<(std::ostream &os, const rc::Scenario &sc);
std::ostream& operator<<(std::ostream &os, const rc::Scenario::Results &o);

} // namespace std

#endif // H_SCENARIO not defined
