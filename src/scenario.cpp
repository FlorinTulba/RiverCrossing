/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.83) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2026 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#include "precompiled.h"

#ifdef UNIT_TESTING

/*
This include allows recompiling only the Unit tests project when updating the
tests. It also keeps the count of total code units to recompile to a minimum
value.
*/
// NOLINTNEXTLINE(misc-include-cleaner) : Contains tests
#include "scenario.hpp"  // IWYU pragma: keep

#endif  // UNIT_TESTING defined

#include "absConfigConstraint.h"
#include "absEntity.h"
#include "absSolution.h"
#include "configConstraint.h"
#include "configParser.h"
#include "entitiesManager.h"
#include "scenario.h"
#include "symbolsTable.h"
#include "transferredLoadExt.h"
#include "util.h"

#include <cassert>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdio>

#include <algorithm>
#include <array>
#include <format>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include <gsl/pointers>
#include <gsl/span>
#include <gsl/util>

#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree_fwd.hpp>

using namespace std;
namespace fs = std::filesystem;
using namespace fs;
using namespace boost::property_tree;

namespace {

/// @throw domain_error mentioning the set of keys to choose only one from
[[noreturn]] void duplicateKeyExc(gsl::span<const string_view> keys) {
  throw domain_error{format("There must appear only one from the keys: {}",
                            rc::ContView{keys, ", "})};
}

/**
@param pt the tree node that should contain one of the keyNames
@param keyNames the set of keys
@param firstFoundKeyName the returned key from the set
@return true if there is exactly one such key; false when no such key
@throw domain_error when the tree node contains more keys from the set
*/
[[nodiscard]] bool onlyOneExpected(const ptree& pt,
                                   gsl::span<const string_view> keyNames,
                                   string& firstFoundKeyName) {
  firstFoundKeyName = "";
  size_t keysCount{};
  for (const string_view keyNameSv : keyNames) {
    const string keyName{keyNameSv};
    if (keysCount == 0ULL) {
      keysCount = pt.count(keyName);
      if (keysCount > 0ULL)
        firstFoundKeyName = keyName;
    } else {
      keysCount += pt.count(keyName);
    }

    if (keysCount >= 2ULL)
      duplicateKeyExc(keyNames);
  }

  return keysCount == 1ULL;
}

/// @return the semantic from nightModeExpr
/// @throw domain_error if nightModeExpr is incorrect
[[nodiscard]] std::shared_ptr<const rc::cond::LogicalExpr> nightModeSemantic(
    const string& nightModeExpr) {
  std::shared_ptr<const rc::cond::LogicalExpr> semantic{
      rc::grammar::parseNightModeExpr(nightModeExpr)};
  return throwIfNull<domain_error>(
      semantic, "NightMode parsing error! See the cause above.");
}

using namespace rc;
using namespace rc::ent;
using namespace rc::cond;

/**
Some scenarios mention how many entities can be simultaneously on the raft /
bridge. For the other scenarios is helpful to deduce an upper bound for this
transfer capacity.
When the capacity needs to be determined, it must be between
        2 and the count of all entities - 1,
that is the raft / bridge must hold >= 2 entities and there must remain
at least 1 entity on the other bank, to have a scenario that is not trivial.
*/
class TransferCapacityManager {
 public:
  TransferCapacityManager(const std::shared_ptr<const AllEntities>& entities_,
                          unsigned& capacity_)
      :  // capacity_ is &ScenarioDetails::capacity
        entities{entities_},
        capacity{&capacity_} {
    if (entities->count() < 3ULL)
      throw domain_error{format("{} - there have to be at least 3 entities!",
                                HERE.function_name())};

    // Not all entities are allowed to cross the river simultaneously
    // (just to keep the problem interesting)
    *capacity = gsl::narrow_cast<unsigned>(entities->count()) - 1U;
  }
  ~TransferCapacityManager() noexcept = default;

  TransferCapacityManager(const TransferCapacityManager&) = delete;
  TransferCapacityManager(TransferCapacityManager&&) noexcept = delete;
  TransferCapacityManager& operator=(const TransferCapacityManager&) = delete;
  TransferCapacityManager& operator=(TransferCapacityManager&&) noexcept =
      delete;

  /// The scenario specifies the transfer capacity
  // NOLINTNEXTLINE(readability-make-member-function-const) : Setter method
  void providedCapacity(unsigned capacity_) {
    if (static_cast<size_t>(capacity_) >= entities->count() || capacity_ < 2U)
      throw domain_error{
          format("{} - RaftCapacity / BridgeCapacity should be "
                 "at least 2 and less than the number of entities!",
                 HERE.function_name())};
    *capacity = std::min(*capacity, capacity_);
  }

  /// The scenario specifies the raft / bridge max load, which might help
  /// for deducing the capacity.
  // NOLINTNEXTLINE(readability-make-member-function-const) : Setter method
  void setMaxLoad(double maxLoad) {
    // Count lightest entities among which 1 might row whose total weight <=
    // maxLoad
    const map<double, set<unsigned>>& idsByWeights{entities->idsByWeights()};
    unsigned lightestEntWhoMightRow{UINT_MAX};
    double totalWeight{};
    for (const auto& idsOfWeight : idsByWeights) {
      for (const unsigned id : idsOfWeight.second)
        if (static_cast<bool>(
                (*entities)
                    [id]  // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                          // : Checked [] - uses at()
                        ->canRow())) {
          lightestEntWhoMightRow = id;
          totalWeight = idsOfWeight.first;
          break;
        }
      if (lightestEntWhoMightRow != UINT_MAX)
        break;
    }
    assert(lightestEntWhoMightRow != UINT_MAX);
    assert(totalWeight);

    unsigned cap{1U};
    for (const auto& idsOfWeight : idsByWeights) {
      const auto& [weight, sameWeightIds] = idsOfWeight;
      size_t availableCount{size(sameWeightIds)};
      if (sameWeightIds.contains(lightestEntWhoMightRow))
        --availableCount;
      const size_t newCount{
          min(availableCount,
              static_cast<size_t>(floor((maxLoad - totalWeight) / weight)))};
      if (newCount != 0ULL) {
        cap += gsl::narrow_cast<unsigned>(newCount);
        totalWeight += static_cast<double>(newCount) * weight;
      }
      if (newCount < availableCount)
        break;
    }

    if (cap < 2U)
      throw domain_error{
          format("{} - Based on the entities' weights and the "
                 "RaftMaxLoad/BridgeMaxLoad, "
                 "the raft can hold at most one of them at a time! "
                 "Please ensure the raft holds at least 2 entities!",
                 HERE.function_name())};

    if (static_cast<size_t>(cap) >= entities->count())
      throw domain_error{format(
          "{} - Based on the entities' weights and the "
          "RaftMaxLoad/BridgeMaxLoad, "
          "the raft can hold all of them at a time! "
          "This constraint cannot be counted as a valid scenario condition!",
          HERE.function_name())};

    *capacity = std::min(*capacity, cap);
  }

  /// The scenario specifies several constraints about the (dis)allowed
  /// raft/bridge configurations, which might help for deducing the capacity.
  // NOLINTNEXTLINE(readability-make-member-function-const) : Setter method
  void setTransferConstraints(const TransferConstraints& transferConstraints) {
    const unsigned cap{transferConstraints.minRequiredCapacity()};

    if (cap < 2U)
      throw domain_error{format(
          "{} - Based on the "
          "[Dis]AllowedRaftConfigurations / [Dis]AllowedBridgeConfigurations, "
          "the raft can hold at most one entity at a time! "
          "Please ensure the raft holds at least 2 entities!",
          HERE.function_name())};

    *capacity = std::min(*capacity, cap);
  }

  [[nodiscard]] const unsigned& getCapacity() const noexcept {
    return *capacity;
  }  ///< provided / deduced capacity

 protected:
  // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes) :
  // Easier to use in derived classes when protected

  /// All entities of the scenario
  gsl::not_null<std::shared_ptr<const AllEntities>> entities;

  /// Provided / deduced transfer capacity (&ScenarioDetails::capacity)
  gsl::not_null<unsigned*> capacity;
  // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};

/// Extract scenario description string
string parseScenarioDescription(const ptree& descrTree) {
  if (descrTree.count("") == 0ULL)
    throw domain_error{
        format("{} - The scenario description should be an array of 1 or more "
               "strings!",
               HERE.function_name())};

  string out;
  out.reserve(512ULL);

  for (auto outIt{back_inserter(out)}; const auto& descrLine : descrTree) {
    if (descrLine.first.empty())
      outIt = format_to(outIt, "{}\n", descrLine.second.data());

    else
      throw domain_error{
          format("{} - The scenario description should be an array of 1 "
                 "or more strings!",
                 HERE.function_name())};
  }

  return out;
}

/// Parse crossing-related constraint: Capacity
void parseCapacityConstraint(const ptree& crossingConstraintsTree,
                             TransferCapacityManager& capManager,
                             unsigned& uniqueConstraints,
                             bool& bridgeInsteadOfRaft) {
  string key;
  static constexpr array<string_view, 2> capacitySynonyms{
      {"RaftCapacity", "BridgeCapacity"}};
  if (onlyOneExpected(crossingConstraintsTree, capacitySynonyms, key)) {
    try {
      // get<unsigned> fails to signal negative values
      const int readCapacity{crossingConstraintsTree.get<int>(key)};
      if (readCapacity < 0)
        throw domain_error{
            format("{} - RaftCapacity / BridgeCapacity should be non-negative!",
                   HERE.function_name())};

      capManager.providedCapacity(gsl::narrow_cast<unsigned>(readCapacity));
    } catch (const ptree_bad_data& ex) {
      throw domain_error{format("{} - Bad type for the raft capacity! {}",
                                HERE.function_name(), ex.what())};
    }

    if (!bridgeInsteadOfRaft && key.at(0ULL) == 'B')
      bridgeInsteadOfRaft = true;

    ++uniqueConstraints;
  }
}

/// Parse crossing-related constraint: Max Load
void parseMaxLoadConstraint(const ptree& crossingConstraintsTree,
                            ScenarioDetails& details,
                            const IEntity& firstEntity,
                            TransferCapacityManager& capManager,
                            unsigned& uniqueConstraints,
                            bool& bridgeInsteadOfRaft) {
  string key;
  double& maxLoad{details.maxLoad};
  static constexpr array<string_view, 2> maxLoadSynonyms{
      {"RaftMaxLoad", "BridgeMaxLoad"}};
  if (onlyOneExpected(crossingConstraintsTree, maxLoadSynonyms, key)) {
    try {
      maxLoad = crossingConstraintsTree.get<double>(key);
    } catch (const ptree_bad_data& ex) {
      throw domain_error{format("{} - Bad type for the raft max load! {}",
                                HERE.function_name(), ex.what())};
    }

    if (maxLoad <= 0.)
      throw domain_error{
          format("{} - The raft max load cannot be negative or zero! ",
                 HERE.function_name())};

    // If 1st entity has no specified weight, then none have.
    // However max load requires those properties
    if (firstEntity.weight() <= 0.)
      throw domain_error{format(
          "{} - Please specify strictly positive weights for all entities "
          "when using the `{}` constraint!",
          HERE.function_name(), key)};

    capManager.setMaxLoad(maxLoad);

    if (!bridgeInsteadOfRaft && key.at(0ULL) == 'B')
      bridgeInsteadOfRaft = true;

    ++uniqueConstraints;
  }
}

/// Parse crossing-related constraint: AllowedLoads
void parseAllowedLoadsConstraint(const ptree& crossingConstraintsTree,
                                 ScenarioDetails& details,
                                 const IEntity& firstEntity,
                                 unsigned& uniqueConstraints,
                                 bool& bridgeInsteadOfRaft) {
  string key;
  std::shared_ptr<const IValues<double>>& allowedLoads{details.allowedLoads};
  static constexpr array<string_view, 2> allowedLoadsSynonyms{
      {"AllowedRaftLoads", "AllowedBridgeLoads"}};
  if (onlyOneExpected(crossingConstraintsTree, allowedLoadsSynonyms, key)) {
    allowedLoads = grammar::parseAllowedLoadsExpr(
        crossingConstraintsTree.get<string>(key));
    if (!allowedLoads)
      throw domain_error{
          format("{} - AllowedRaftLoads parsing error! See the cause above.",
                 HERE.function_name())};

    // If 1st entity has no specified weight, then none have.
    // However max load requires these properties
    if (firstEntity.weight() <= 0.)
      throw domain_error{format(
          "{} - Please specify strictly positive weights for all entities "
          "when using the `{}` constraint!",
          HERE.function_name(), key)};

    if (!bridgeInsteadOfRaft && key.at(7ULL) == 'B')
      bridgeInsteadOfRaft = true;

    ++uniqueConstraints;
  }
}

/// Parse crossing-related constraint: [Dis]Allowed{Raft,Bridge}Configurations
void parseRaftCfgsConstraint(const ptree& crossingConstraintsTree,
                             ScenarioDetails& details,
                             const AllEntities& entities,
                             TransferCapacityManager& capManager,
                             unsigned& uniqueConstraints,
                             bool& bridgeInsteadOfRaft) {
  string key;
  static constexpr array<string_view, 4> trConfigSpecifiers{
      {"AllowedRaftConfigurations", "AllowedBridgeConfigurations",
       "DisallowedRaftConfigurations", "DisallowedBridgeConfigurations"}};
  if (onlyOneExpected(crossingConstraintsTree, trConfigSpecifiers, key)) {
    std::optional<grammar::ConstraintsVec> readConstraints =
        grammar::parseConfigurationsExpr(
            crossingConstraintsTree.get<string>(key));
    if (!readConstraints)
      throw domain_error{
          format("{} - Constraints parsing error! See the cause above.",
                 HERE.function_name())};

    // key starts either with 'A' or with 'D':
    // AllowedRaftConfigurations or AllowedBridgeConfigurations
    // DisallowedRaftConfigurations or DisallowedBridgeConfigurations
    const bool allowed{key.at(0ULL) == 'A'};
    // ensure that DisallowedRaftConfigurations contains the empty set
    if (!allowed)
      (*readConstraints).push_back(make_shared<const IdsConstraint>());

    try {
      details.transferConstraints = make_unique<const TransferConstraints>(
          std::move(*readConstraints), entities, capManager.getCapacity(),
          allowed, *details.transferConstraintsExt);
    } catch (const logic_error& e) {
      throw domain_error{e.what()};
    }

    capManager.setTransferConstraints(*details.transferConstraints);

    if (!bridgeInsteadOfRaft && ((allowed && key.at(7ULL) == 'B') ||
                                 (!allowed && key.at(10ULL) == 'B')))
      bridgeInsteadOfRaft = true;

    ++uniqueConstraints;

  } else {  // no [dis]allowed raft/bridge configurations constraint
    // Create a constraint checking only the capacity & maxLoad for the
    // raft/bridge
    details.transferConstraints = make_unique<const TransferConstraints>(
        grammar::ConstraintsVec{}, entities, capManager.getCapacity(), false,
        *details.transferConstraintsExt);
  }
}

/// Parse crossing-related constraint: CrossingDurationsOfConfigurations
void parseCrossTimesConstraint(const ptree& crossingConstraintsTree,
                               ScenarioDetails& details,
                               const AllEntities& entities,
                               const TransferCapacityManager& capManager,
                               unsigned& uniqueConstraints) {
  vector<ConfigurationsTransferDuration>& ctdItems{details.ctdItems};
  if (crossingConstraintsTree.count("CrossingDurationsOfConfigurations") >
      0ULL) {
    const ptree cdcTree{
        crossingConstraintsTree.get_child("CrossingDurationsOfConfigurations")};
    if (cdcTree.count("") == 0ULL)
      throw domain_error{
          format("{} - The CrossingDurationsOfConfigurations section should be "
                 "an array "
                 "of 1 or more such items!",
                 HERE.function_name())};

    unordered_set<unsigned> durations;
    for (const auto& cdcPair : cdcTree) {
      if (!cdcPair.first.empty())
        throw domain_error{
            format("{} - The CrossingDurationsOfConfigurations section "
                   "should be an array "
                   "of 1 or more such items!",
                   HERE.function_name())};
      const string cdcStr{cdcPair.second.data()};
      std::optional<grammar::ConfigurationsTransferDurationInitType> readCdc{
          grammar::parseCrossingDurationForConfigurationsExpr(cdcStr)};
      if (!readCdc)
        throw domain_error{
            format("{} - CrossingDurationsOfConfigurations parsing error! "
                   "See the cause above.",
                   HERE.function_name())};
      ctdItems.emplace_back(std::move(*readCdc), entities,
                            capManager.getCapacity(),
                            *details.transferConstraintsExt);
      const ConfigurationsTransferDuration& ctd{ctdItems.back()};
      if (!durations.insert(ctd.duration()).second)
        throw domain_error{
            format("{} - Before configuration `{}` there was another one with "
                   "the same transfer time. "
                   "Please group them together instead!",
                   HERE.function_name(), ctd)};
    }

    ++uniqueConstraints;
  }
}

/// Parse crossing-related constraints: Capacity, MaxLoad, AllowedLoads,
/// [Dis]Allowed{Raft,Bridge}Configurations and
/// CrossingDurationsOfConfigurations.
void parseCrossingConstraints(const ptree& crossingConstraintsTree,
                              ScenarioDetails& details,
                              const AllEntities& entities,
                              const IEntity& firstEntity,
                              TransferCapacityManager& capManager,
                              unsigned& uniqueConstraints,
                              bool& bridgeInsteadOfRaft) {
  parseCapacityConstraint(crossingConstraintsTree, capManager,
                          uniqueConstraints, bridgeInsteadOfRaft);
  parseMaxLoadConstraint(crossingConstraintsTree, details, firstEntity,
                         capManager, uniqueConstraints, bridgeInsteadOfRaft);
  parseAllowedLoadsConstraint(crossingConstraintsTree, details, firstEntity,
                              uniqueConstraints, bridgeInsteadOfRaft);

  // Setup transfer constraints extension before parsing configs/ctd.
  // Keep this after Capacity, Max Load and Allowed Loads.
  details.createTransferConstraintsExt();

  parseRaftCfgsConstraint(crossingConstraintsTree, details, entities,
                          capManager, uniqueConstraints, bridgeInsteadOfRaft);

  // Keep this after Capacity, Max Load and Allowed Loads
  parseCrossTimesConstraint(crossingConstraintsTree, details, entities,
                            capManager, uniqueConstraints);
}

/// Parse banks constraints block if present
void parseBanksConstraints(const ptree& banksConstraintsTree,
                           ScenarioDetails& details,
                           const AllEntities& entities) {
  if (banksConstraintsTree.empty())
    return;

  static constexpr array<string_view, 2> bankConfigSpecifiers{
      {"AllowedBankConfigurations", "DisallowedBankConfigurations"}};
  string key;
  if (!onlyOneExpected(banksConstraintsTree, bankConfigSpecifiers, key))
    return;

  std::optional<grammar::ConstraintsVec> readConstraints{
      grammar::parseConfigurationsExpr(banksConstraintsTree.get<string>(key))};
  if (!readConstraints)
    throw domain_error{
        format("{} - Constraints parsing error! See the cause above.",
               HERE.function_name())};

  // key starts either with 'A' or with 'D':
  // AllowedBankConfigurations  OR  DisallowedBankConfigurations
  const bool allowed{key.at(0ULL) == 'A'};

  // For AllowedBankConfigurations allow also start & final configurations
  if (allowed) {
    const vector<unsigned>& idsStartingFromLeftBank{
        entities.idsStartingFromLeftBank()};
    const vector<unsigned>& idsStartingFromRightBank{
        entities.idsStartingFromRightBank()};

    // Building constraints
    auto pInitiallyOnLeftBank{make_unique<IdsConstraint>()};
    auto pInitiallyOnRightBank{make_unique<IdsConstraint>()};

    for (const unsigned id : idsStartingFromLeftBank)
      pInitiallyOnLeftBank->addMandatoryId(id);
    for (const unsigned id : idsStartingFromRightBank)
      pInitiallyOnRightBank->addMandatoryId(id);

    // Non-mutable final constraints
    std::shared_ptr<const IdsConstraint> initiallyOnLeftBank{
        pInitiallyOnLeftBank.release()};
    std::shared_ptr<const IdsConstraint> initiallyOnRightBank{
        pInitiallyOnRightBank.release()};

    (*readConstraints).push_back(initiallyOnLeftBank);
    (*readConstraints).push_back(initiallyOnRightBank);
  }

  try {
    details.banksConstraints = make_unique<const ConfigConstraints>(
        std::move(*readConstraints), entities, allowed);
  } catch (const logic_error& e) {
    throw domain_error{e.what()};
  }
}

/// Parse OtherConstraints (TimeLimit, NightMode).
/// Returns nightMode expression string (default "false").
string parseOtherConstraints(const ptree& otherConstraintsTree,
                             ScenarioDetails& details) {
  string nightModeExpr{"false"};
  if (otherConstraintsTree.empty())
    return nightModeExpr;

  static constexpr array<string_view, 1> timeLimitSpecifiers{{"TimeLimit"}};
  string key;
  if (onlyOneExpected(otherConstraintsTree, timeLimitSpecifiers, key)) {
    try {
      // get<unsigned> fails to signal negative values
      const int readMaxDuration{otherConstraintsTree.get<int>(key)};
      if (readMaxDuration <= 0)
        throw domain_error{
            format("{} - TimeLimit should be > 0!", HERE.function_name())};

      details.maxDuration = gsl::narrow_cast<unsigned>(readMaxDuration);
    } catch (const ptree_bad_data& ex) {
      throw domain_error{format("{} - Bad type for the time limit! {}",
                                HERE.function_name(), ex.what())};
    }

    // TimeLimit requires CrossingDurationsOfConfigurations constraints
    if (details.ctdItems.empty())
      throw domain_error{format(
          "{} - Please specify a CrossingDurationsOfConfigurations section "
          "when using the `TimeLimit` constraint!",
          HERE.function_name())};
  }

  static constexpr array<string_view, 1> nightModeSpecifiers{{"NightMode"}};
  if (onlyOneExpected(otherConstraintsTree, nightModeSpecifiers, key))
    nightModeExpr = otherConstraintsTree.get<string>(key);

  return nightModeExpr;
}

}  // anonymous namespace

namespace rc {

Scenario::Scenario(istream& scenarioStream,
                   bool solveNow /* = false*/,
                   bool interactiveSol /* = false*/) {
  ptree pt, crossingConstraintsTree, banksConstraintsTree, otherConstraintsTree;
  const ptree empty;
  try {
    read_json(scenarioStream, pt);
  } catch (const json_parser_error& ex) {
    throw domain_error{
        format("{} - Couldn't parse puzzle data (json format "
               "expected)!\nReason:\n{}",
               HERE.function_name(), ex.what())};
  }

  // Extract mandatory sections
  try {
    descrTree = pt.get_child("ScenarioDescription");
    entTree = pt.get_child("Entities");
    crossingConstraintsTree = pt.get_child("CrossingConstraints");
  } catch (const ptree_bad_path& ex) {
    throw domain_error{format("{} - Missing mandatory section! ",
                              HERE.function_name(), ex.what())};
  }

  // Store ScenarioDescription (extracted into helper)
  descr = parseScenarioDescription(descrTree);

  // Entities and capacity manager
  unsigned& capacity{details.capacity};
  std::shared_ptr<const AllEntities>& entities{details.entities};
  entities = make_shared<const AllEntities>(entTree);
  assert(entities && entities->count() > 0ULL);

  const auto firstId{*cbegin(entities->ids())};
  const std::shared_ptr<const IEntity> firstEntity{
      (*entities)
          [firstId]};  // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                       // : Checked [] - uses at()
  TransferCapacityManager capManager{entities, capacity};

  // Optional trees
  banksConstraintsTree = pt.get_child("BanksConstraints", empty);
  otherConstraintsTree = pt.get_child("OtherConstraints", empty);

  unsigned uniqueConstraints{};
  bool bridgeInsteadOfRaftLocal{bridgeInsteadOfRaft};

  // Move large crossing constraints parsing into helper
  parseCrossingConstraints(crossingConstraintsTree, details, *entities,
                           *firstEntity, capManager, uniqueConstraints,
                           bridgeInsteadOfRaftLocal);

  // Apply possible bridge flag change from helper
  bridgeInsteadOfRaft = bridgeInsteadOfRaftLocal;

  // Banks constraints parsing
  parseBanksConstraints(banksConstraintsTree, details, *entities);

  // Other constraints (TimeLimit and NightMode)
  string nightModeExpr = parseOtherConstraints(otherConstraintsTree, details);
  nightMode = nightModeSemantic(nightModeExpr);

  // Post-conditions / validations
  if (firstEntity->weight() > 0.  // all entities have then specified weights
      && isgreaterequal(details.maxLoad, DBL_MAX) && !details.allowedLoads)
    throw domain_error{format(
        "{} - Unnecessary weights of entities when not using none of the "
        "following constraints: "
        "{{RaftMaxLoad/BridgeMaxLoad or AllowedRaftLoads/AllowedBridgeLoads}}!",
        HERE.function_name())};

  if (!details.ctdItems.empty() && details.maxDuration == UINT_MAX)
    throw domain_error{
        format("{} - Unnecessary CrossingDurationsOfConfigurations "
               "when not using the TimeLimit constraint!",
               HERE.function_name())};

  if (0U == uniqueConstraints)
    throw domain_error{
        format("{} - There must be at least one valid crossing constraint!",
               HERE.function_name())};

  if (solveNow)
    ignore = solution(true, interactiveSol);
}

void Scenario::outputResults(const Results& res,
                             bool interactiveSol /* = false*/) const {
  if (!res.attempt)
    throw logic_error{format(
        "{} - cannot be called when the scenario wasn't / couldn't be solved "
        "based on the requested algorithm!",
        HERE.function_name())};

  if (!interactiveSol || !res.attempt->isSolution()) {
    print("Considered scenario:\n{}\n\n{}", *this, res);
    fflush(stdout);
    return;
  }

  const unsigned solLen{gsl::narrow_cast<unsigned>(res.attempt->length())};
  const std::shared_ptr<const sol::IState> initialState{
      res.attempt->initialState()};
  SymbolsTable st{InitialSymbolsTable()};

  ptree root, movesTree;

  root.put_child("ScenarioDescription", descrTree);

  if (bridgeInsteadOfRaft)
    root.put("Bridge", "true");

  root.put_child("Entities", entTree);

  const auto addEntsSet = [](ptree& pt, const IsolatedEntities& ents,
                             const string& propName) {
    ptree idsTree;
    for (const unsigned id : ents.ids()) {
      ptree idTree;
      idTree.put_value(id);
      idsTree.push_back(make_pair("", idTree));
    }
    pt.put_child(propName, idsTree);
  };

  const auto addMove = [this, &addEntsSet, &movesTree, &st](
                           const std::shared_ptr<const sol::IState>& state,
                           const IsolatedEntities* moved = {}) {
    const string otherDetails{state->getExtension()->detailsForDemo()};

    ptree moveTree;
    moveTree.put("Idx", st["CrossingIndex"]++);
    if (nightMode->eval(st))
      moveTree.put("NightMode", true);
    if (moved)
      addEntsSet(moveTree, *moved, "Transferred");
    addEntsSet(moveTree, state->leftBank(), "LeftBank");
    addEntsSet(moveTree, state->rightBank(), "RightBank");
    if (!otherDetails.empty())
      moveTree.put("OtherDetails", otherDetails);
    movesTree.push_back(make_pair("", moveTree));
  };

  addMove(initialState);

  for (unsigned step{}; step < solLen; ++step) {
    const sol::IMove& aMove{res.attempt->move(step)};
    const std::shared_ptr<const sol::IState> resultedState{
        aMove.resultedState()};

    addMove(aMove.resultedState(), &aMove.movedEntities());
  }

  root.put_child("Moves", movesTree);

  write_json(cout, root);
}

std::shared_ptr<const cond::IContextValidator>
ScenarioDetails::createTransferValidator() const {
  const std::shared_ptr<const cond::IContextValidator>& res{
      cond::DefContextValidator::SHARED_INST()};
  if (!allowedLoads)
    return res;

  return make_shared<const AllowedLoadsValidator>(
      allowedLoads, res,
      make_shared<const InitiallyNoPrevRaftLoadExcHandler>(allowedLoads));
}

void ScenarioDetails::createTransferConstraintsExt() {
  auto res{cond::DefTransferConstraintsExt::NEW_INST()};
  if (isgreaterequal(maxLoad, DBL_MAX)) {
    transferConstraintsExt = std::move(res);
    return;
  }

  transferConstraintsExt =
      make_unique<const MaxLoadTransferConstraintsExt>(maxLoad, std::move(res));
}

std::unique_ptr<ent::IMovingEntitiesExt>
ScenarioDetails::createMovingEntitiesExt() const {
  std::unique_ptr<ent::IMovingEntitiesExt> res{
      make_unique<DefMovingEntitiesExt>()};

  if (!allowedLoads && isgreaterequal(maxLoad, DBL_MAX))
    return res;

  return make_unique<TotalLoadExt>(entities, 0., std::move(res));
}

void Scenario::Results::update(size_t attemptLen,
                               size_t crtDistToSol,
                               const ent::BankEntities& currentLeftBank,
                               size_t& bestMinDistToGoal) noexcept {
  longestInvestigatedPath = std::max(longestInvestigatedPath, attemptLen);

  if (bestMinDistToGoal > crtDistToSol) {
    bestMinDistToGoal = crtDistToSol;
    makeNoexcept([this, &currentLeftBank] {
      closestToTargetLeftBank = {currentLeftBank};
    });

  } else if (bestMinDistToGoal == crtDistToSol) {
    makeNoexcept([this, &currentLeftBank] {
      closestToTargetLeftBank.push_back(currentLeftBank);
    });
  }
}

void Scenario::Results::formatTo(FmtCtxIt& outIt) const {
  if (attempt->isSolution()) {
    outIt = format_to(outIt, "Found solution using {} steps:\n\n{}",
                      attempt->length(), *attempt);

  } else {
    outIt =
        format_to(outIt,
                  "Found no solution. Longest investigated path: {}. "
                  "Investigated states: {}. Nearest states to the solution:\n",
                  longestInvestigatedPath, investigatedStates);

    for (const rc::ent::BankEntities& leftBank : closestToTargetLeftBank)
      outIt = format_to(outIt, "{} - {}\n", leftBank, ~leftBank);
  }
}

const string& Scenario::description() const noexcept {
  return descr;
}

void Scenario::formatTo(FmtCtxIt& outIt) const {
  details.formatTo(outIt);
}

void ScenarioDetails::formatTo(FmtCtxIt& outIt) const {
  if (entities)
    outIt = format_to(outIt, "{}\n", *entities);

  outIt = format_to(outIt, "CrossingConstraints: {{ ");

  // The first constraint is shown as is, the next ones are separated by "; "
  // for better readability
  bool hasPrevConstraint{};

  // Helper for separating constraints in the output string
  const auto appendCategSeparator{[&hasPrevConstraint, &outIt] {
    if (hasPrevConstraint)
      outIt = format_to(outIt, "; ");
    else
      hasPrevConstraint = true;
  }};

  if (capacity != UINT_MAX) {
    outIt = format_to(outIt, "Capacity = {}", capacity);
    hasPrevConstraint = true;
  }

  if (isless(maxLoad, DBL_MAX)) {
    appendCategSeparator();
    outIt = format_to(outIt, "MaxLoad = {}", maxLoad);
  }

  if (transferConstraints && !transferConstraints->empty()) {
    appendCategSeparator();
    outIt = format_to(outIt, "TransferConstraints = {}", *transferConstraints);
  }

  if (allowedLoads) {
    appendCategSeparator();
    outIt = format_to(outIt, "AllowedLoads = `{}`", *allowedLoads);
  }

  if (!ctdItems.empty()) {
    appendCategSeparator();
    outIt = format_to(outIt, "CrossingDurations: {{ {} }}",
                      ContView{ctdItems, " ; "});
  }

  assert(hasPrevConstraint);
  outIt = format_to(outIt, " }}");

  if (banksConstraints && !banksConstraints->empty())
    outIt = format_to(outIt, "\nBanksConstraints = {}", *banksConstraints);

  if (maxDuration != UINT_MAX)
    outIt =
        format_to(outIt, "\nOtherConstraints: {{ TimeLimit = {} time units }}",
                  maxDuration);
}

}  // namespace rc

#ifndef UNIT_TESTING

#include <exception>
#include <filesystem>

#include <gsl/assert>

// GSL before 4.1.0 doesn't have <gsl/zstring>
#if __has_include(<gsl/zstring>)
#include <gsl/zstring>

#else
#include <gsl/span>
#include <gsl/string_span>

#endif  // <zstring> available or not

using namespace gsl;

namespace {

/// Settings for the project
class Config {
 public:
  Config() : projDir{rc::projectFolder()}, scenariosDir{projDir / "Scenarios"} {
    if (projDir.empty())
      throw runtime_error{
          format("{} - Couldn't locate the base folder of the project or "
                 "the Scenarios folder! "
                 "Please launch the program within (a subfolder of) "
                 "RiverCrossing directory!",
                 HERE.function_name())};

    if (!exists(scenariosDir))
      throw runtime_error{format("{} - Couldn't locate the Scenarios folder!",
                                 HERE.function_name())};

    const directory_iterator itEnd;
    for (directory_iterator it{scenariosDir}; it != itEnd; ++it) {
      fs::path file{it->path()};
      if (file.extension().string() != ".json")
        continue;

      _scenarios.emplace_back(std::move(file));
    }
  }

 private:
  fs::path projDir;
  fs::path scenariosDir;

  vector<fs::path> _scenarios;
};

}  // anonymous namespace

// NOLINTNEXTLINE(bugprone-exception-escape) : Any exception is caught
int main(int argc, zstring* argv) try {
  Expects(argv && argc >= 1);

  const gsl::span<zstring> args{argv, static_cast<size_t>(argc)};

  const bool interactive{
      (size(args) >= 2ULL) &&
      ("interactive" ==
       string_view{
           args[1ULL]  // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                       // : Check performed above
       })};

#ifndef NDEBUG
  println("Interactive:{}", interactive);
  fflush(stdout);
#endif  // NDEBUG

  Config cfg;
  Scenario scenario{cin, /*solveNow = */ false};
  const Scenario::Results& sol{
      scenario.solution(/*usingBFS = */ true, interactive)};
  if (!sol.attempt->isSolution())
    return -1;

  return 0;

} catch (const exception& e) {
  println(stderr, "{}", e.what());
  fflush(stderr);
  return -1;

} catch (...) {
  println(stderr, "An unknown error occurred!");
  fflush(stderr);
  return -1;
}

#endif  // UNIT_TESTING not defined
