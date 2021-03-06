/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_SCENARIO
#define H_SCENARIO

#include "scenarioDetails.h"
#include "entitiesManager.h"
#include "configConstraint.h"
#include "absSolution.h"

#include <boost/property_tree/ptree.hpp>

namespace rc {

/**
Data and the solution for a river crossing puzzle:
- entities to move to the opposite bank
- various constraints about the raft / bridge or
  about which entities can be on a bank
*/
class Scenario {
public:

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

    /// Updates the fields based on a new unsuccessful attempt
    void update(size_t attemptLen, size_t crtDistToSol,
                const ent::BankEntities &currentLeftBank,
                size_t &bestMinDistToGoal);
  };

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  /// Path to the file needed to prepare visualizations; Empty when no visualization is desired
  std::string demoPrepFile;

  /// Shell command to visualize a found solution; Empty when no visualization is desired
  std::string visualizer;

  // Read in ctor and reused by prepareDemo()
  boost::property_tree::ptree entTree, descrTree;

  std::shared_ptr<const cond::LogicalExpr> nightMode;

	std::string descr;	///< provided description of the scenario

	ScenarioDetails details; ///< relevant details of the scenario

  Results resultsBFS; ///< the results obtained with Breadth-First search
  Results resultsDFS; ///< the results obtained with Depth-First search

  /// Ensures solving is performed only once with Breadth-First search
	bool investigatedByBFS = false;

  /// Ensures solving is performed only once with Depth-First search
	bool investigatedByDFS = false;

	/// Some scenarios use bridges instead of rafts
	bool bridgeInsteadOfRaft = false;

	/// Prepares html/js/solution.js for visualizing the solution
	void prepareDemo(const Results& res) const;

public:
	/**
  Builds a scenario based on the input from the provided stream.
  If solveNow is true, it attempts to also solve this scenario using Breadth-First search

  demoPrepFile_ and visualizer_ are used when intending to visualize the solution
  within a browser.

  @throw domain_error if there is a problem with the given scenario
	*/
	Scenario(std::istream &&scenarioStream, bool solveNow = false,
          const std::string &demoPrepFile_ = "",
          const std::string &visualizer_ = "");
	Scenario(const Scenario&) = delete;
	Scenario(Scenario&&) = default;
	void operator=(const Scenario&) = delete;
	Scenario& operator=(Scenario&&) = default;

	/// Provided description of the scenario
	const std::string& description() const;

	/**
  Solves the scenario if possible.
  Subsequent calls use the obtained attempt / solution.

  @param usingBFS true when a Breadth-First solution is wanted (default)
    false for a Depth-First solution
  @param visualize true when a visualization of the solution is requested and possible
    false by default

  @return the solution or an unsuccessful attempt
	*/
	const Results& solution(bool usingBFS = true, bool visualize = false);

	std::string toString() const; ///< data apart from the description
};

} // namespace rc

namespace std {

std::ostream& operator<<(std::ostream &os, const rc::Scenario &sc);
std::ostream& operator<<(std::ostream &os, const rc::Scenario::Results &o);

} // namespace std

#endif // H_SCENARIO not defined
