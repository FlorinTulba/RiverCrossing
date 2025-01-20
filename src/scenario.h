/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_SCENARIO
#define H_SCENARIO

#include "scenarioDetails.h"

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
  class Results {
   public:
    /// Updates the fields based on a new unsuccessful attempt
    void update(size_t attemptLen,
                size_t crtDistToSol,
                const ent::BankEntities& currentLeftBank,
                size_t& bestMinDistToGoal) noexcept;

    /// The solution or an unsuccessful attempt
    std::shared_ptr<const sol::IAttempt> attempt{};

    /// All the configurations identified as closest to the target left bank
    std::vector<ent::BankEntities> closestToTargetLeftBank;

    /// Length of the longest investigated attempt
    size_t longestInvestigatedPath{};

    /// Count of total investigated states
    size_t investigatedStates{};
  };

 public:
  /**
  Builds a scenario based on the input from the provided stream.
  If solveNow is true, it attempts to also solve this scenario using
  Breadth-First search

  interactiveSol_ is used when intending to visualize an interactive solution

  @throw domain_error if there is a problem with the given scenario
  */
  explicit Scenario(std::istream& scenarioStream,
                    bool solveNow = false,
                    bool interactiveSol = false);
  explicit Scenario(std::istream&& scenarioStream,
                    bool solveNow = false,
                    bool interactiveSol = false);
  Scenario(Scenario&&) noexcept = default;
  Scenario& operator=(Scenario&&) noexcept = default;
  ~Scenario() noexcept = default;

  Scenario(const Scenario&) = delete;
  void operator=(const Scenario&) = delete;

  /// Provided description of the scenario
  [[nodiscard]] const std::string& description() const noexcept;

  /**
  Solves the scenario if possible.
  Subsequent calls use the obtained attempt / solution.

  @param usingBFS true when a Breadth-First solution is wanted (default); false
  for a Depth-First solution
  @param interactiveSol_ true when an interactive visualization of the solution
  is requested and possible; false by default

  @return the solution or an unsuccessful attempt
  */
  [[nodiscard]] const Results& solution(bool usingBFS = true,
                                        bool interactiveSol = false);

  [[nodiscard]] std::string toString()
      const;  ///< data apart from the description

  PROTECTED :

      /// Prepares visualizing the solution
      void
      outputResults(const Results& res, bool interactiveSol = false) const;

  // Read in ctor and reused by outputResults()
  boost::property_tree::ptree entTree;
  boost::property_tree::ptree descrTree;

  std::shared_ptr<const cond::LogicalExpr> nightMode;

  std::string descr;  ///< provided description of the scenario

  ScenarioDetails details;  ///< relevant details of the scenario

  Results resultsBFS;  ///< the results obtained with Breadth-First search
  Results resultsDFS;  ///< the results obtained with Depth-First search

  /// Ensures solving is performed only once with Breadth-First search
  bool investigatedByBFS{};

  /// Ensures solving is performed only once with Depth-First search
  bool investigatedByDFS{};

  /// Some scenarios use bridges instead of rafts
  bool bridgeInsteadOfRaft{};
};

}  // namespace rc

namespace std {

std::ostream& operator<<(std::ostream& os, const rc::Scenario& sc);
std::ostream& operator<<(std::ostream& os, const rc::Scenario::Results& o);

}  // namespace std

#endif  // H_SCENARIO not defined
