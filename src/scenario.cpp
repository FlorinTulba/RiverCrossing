/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#include "precompiled.h"

#ifdef UNIT_TESTING

/*
  This include allows recompiling only the Unit tests project when updating the
  tests. It also keeps the count of total code units to recompile to a minimum
  value.
*/
#define CPP_SCENARIO
#include "scenario.hpp"
#undef CPP_SCENARIO

#endif  // UNIT_TESTING defined

#include "durationExt.h"
#include "scenario.h"
#include "transferredLoadExt.h"
#include "util.h"

#include <climits>
#include <cmath>
#include <ranges>
#include <tuple>

#include <boost/property_tree/json_parser.hpp>

using namespace std;
namespace fs = std::filesystem;
using namespace fs;
using namespace boost::property_tree;

namespace {

/// @throw domain_error mentioning the set of keys to choose only one from
void duplicateKeyExc(const vector<string>& keys) {
  ostringstream oss;
  if (!keys.empty()) {
    oss << keys.front();
    for (const auto& key : keys | views::drop(1))
      oss << ", " << key;
  }
  throw domain_error{"There must appear only one from the keys: "s + oss.str()};
}

/**
@param pt the tree node that should contain one of the keyNames
@param keyNames the set of keys
@param firstFoundKeyName the returned key from the set
@return true if there is exactly one such key; false when no such key
@throw domain_error when the tree node contains more keys from the set
*/
[[nodiscard]] bool onlyOneExpected(const ptree& pt,
                                   const vector<string>& keyNames,
                                   string& firstFoundKeyName) {
  firstFoundKeyName = "";
  size_t keysCount{};
  for (const string& keyName : keyNames) {
    if (!keysCount) {
      keysCount = pt.count(keyName);
      if (keysCount > 0ULL)
        firstFoundKeyName = keyName;
    } else
      keysCount += pt.count(keyName);

    if (keysCount >= 2ULL)
      duplicateKeyExc(keyNames);
  }

  return keysCount == 1ULL;
}

/**
  @return the semantic from nightModeExpr
  @throw domain_error if nightModeExpr is incorrect
*/
[[nodiscard]] shared_ptr<const rc::cond::LogicalExpr> nightModeSemantic(
    const string& nightModeExpr) {
  shared_ptr<const rc::cond::LogicalExpr> semantic{
      rc::grammar::parseNightModeExpr(nightModeExpr)};
  return CP_EX_MSG(semantic, domain_error,
                   "NightMode parsing error! See the cause above.");
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
  TransferCapacityManager(const shared_ptr<const AllEntities>& entities_,
                          unsigned& capacity_)
      :  // capacity_ is &ScenarioDetails::capacity
        entities{entities_},
        capacity{&capacity_} {
    if (entities->count() < 3ULL)
      throw domain_error{HERE.function_name() +
                         " - there have to be at least 3 entities!"s};

    // Not all entities are allowed to cross the river simultaneously
    // (just to keep the problem interesting)
    *capacity = (unsigned)entities->count() - 1U;
  }
  ~TransferCapacityManager() noexcept = default;

  TransferCapacityManager(const TransferCapacityManager&) = delete;
  void operator=(const TransferCapacityManager&) = delete;
  void operator=(TransferCapacityManager&&) = delete;

  /// The scenario specifies the transfer capacity
  void providedCapacity(unsigned capacity_) {
    if (capacity_ >= (unsigned)entities->count() || capacity_ < 2U)
      throw domain_error{HERE.function_name() +
                         " - RaftCapacity / BridgeCapacity should be "
                         "at least 2 and less than the number of entities!"s};
    if (capacity_ < *capacity)
      *capacity = capacity_;
  }

  /**
  The scenario specifies the raft / bridge max load, which might help
  for deducing the capacity.
  */
  void setMaxLoad(double maxLoad) {
    // Count lightest entities among which 1 might row whose total weight <=
    // maxLoad
    const map<double, set<unsigned>>& idsByWeights{entities->idsByWeights()};
    unsigned lightestEntWhoMightRow{UINT_MAX};
    double totalWeight{};
    for (const auto& idsOfWeight : idsByWeights) {
      for (const unsigned id : idsOfWeight.second)
        if (!!(*entities)[id]
                  ->canRow()) {  // !!(tribool) => true or indeterminate
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
          min(availableCount, (size_t)floor((maxLoad - totalWeight) / weight))};
      if (newCount) {
        cap += (unsigned)newCount;
        totalWeight += (double)newCount * weight;
      }
      if (newCount < availableCount)
        break;
    }

    if (cap < 2U)
      throw domain_error{HERE.function_name() +
                         " - Based on the entities' weights and the "
                         "RaftMaxLoad/BridgeMaxLoad, "
                         "the raft can hold at most one of them at a time! "
                         "Please ensure the raft holds at least 2 entities!"s};

    if (cap >= (unsigned)entities->count())
      throw domain_error{
          HERE.function_name() +
          " - Based on the entities' weights and the "
          "RaftMaxLoad/BridgeMaxLoad, "
          "the raft can hold all of them at a time! "
          "This constraint cannot be counted as a valid scenario condition!"s};

    if (cap < *capacity)
      *capacity = cap;
  }

  /**
  The scenario specifies several constraints about the (dis)allowed raft/bridge
  configurations, which might help for deducing the capacity.
  */
  void setTransferConstraints(const TransferConstraints& transferConstraints) {
    const unsigned cap{transferConstraints.minRequiredCapacity()};

    if (cap < 2U)
      throw domain_error{
          HERE.function_name() +
          " - Based on the "
          "[Dis]AllowedRaftConfigurations / [Dis]AllowedBridgeConfigurations, "
          "the raft can hold at most one entity at a time! "
          "Please ensure the raft holds at least 2 entities!"s};

    if (cap < *capacity)
      *capacity = cap;
  }

  [[nodiscard]] const unsigned& getCapacity() const noexcept {
    return *capacity;
  }  ///< provided / deduced capacity

  PROTECTED :

      /// All entities of the scenario
      gsl::not_null<shared_ptr<const AllEntities>>
          entities;

  /// Provided / deduced transfer capacity (&ScenarioDetails::capacity)
  gsl::not_null<unsigned*> capacity;
};

}  // anonymous namespace

namespace rc {

Scenario::Scenario(istream&& scenarioStream,
                   bool solveNow /* = false*/,
                   bool interactiveSol /* = false*/)
    : Scenario{scenarioStream, solveNow, interactiveSol} {}

Scenario::Scenario(istream& scenarioStream,
                   bool solveNow /* = false*/,
                   bool interactiveSol /* = false*/) {
  ptree pt, crossingConstraintsTree, banksConstraintsTree, otherConstraintsTree;
  const ptree empty;
  try {
    read_json(scenarioStream, pt);
  } catch (const json_parser_error& ex) {
    throw domain_error{
        HERE.function_name() +
        " - Couldn't parse puzzle data (json format expected)!\nReason:\n"s +
        ex.what()};
  }

  // Extract mandatory sections
  try {
    descrTree = pt.get_child("ScenarioDescription");
    entTree = pt.get_child("Entities");
    crossingConstraintsTree = pt.get_child("CrossingConstraints");
  } catch (const ptree_bad_path& ex) {
    throw domain_error{HERE.function_name() +
                       " - Missing mandatory section! "s + ex.what()};
  }

  // Store ScenarioDescription
  if (!descrTree.count(""))
    throw domain_error{
        HERE.function_name() +
        " - The scenario description should be an array of 1 or more strings!"s};
  ostringstream oss;
  for (const auto& descrLine : descrTree)
    if (descrLine.first.empty())
      oss << descrLine.second.data() << endl;
    else
      throw domain_error{HERE.function_name() +
                         " - The scenario description should be an array of 1 "
                         "or more strings!"s};
  descr = oss.str();

  unsigned& capacity{details.capacity};
  shared_ptr<const AllEntities>& entities{details.entities};
  entities = make_shared<const AllEntities>(entTree);
  assert(entities && entities->count() > 0ULL);
  const shared_ptr<const IEntity> firstEntity{
      (*entities)[*cbegin(entities->ids())]};
  TransferCapacityManager capManager{entities, capacity};

  banksConstraintsTree = pt.get_child("BanksConstraints", empty);
  otherConstraintsTree = pt.get_child("OtherConstraints", empty);

  unsigned uniqueConstraints{};
  string key;
  static const vector<string> capacitySynonyms{"RaftCapacity",
                                               "BridgeCapacity"};
  if (onlyOneExpected(crossingConstraintsTree, capacitySynonyms, key)) {
    try {
      // get<unsigned> fails to signal negative values
      const int readCapacity{crossingConstraintsTree.get<int>(key)};
      if (readCapacity < 0)
        throw domain_error{
            HERE.function_name() +
            " - RaftCapacity / BridgeCapacity should be non-negative!"s};

      capManager.providedCapacity((unsigned)readCapacity);
    } catch (const ptree_bad_data& ex) {
      throw domain_error{HERE.function_name() +
                         " - Bad type for the raft capacity! "s + ex.what()};
    }

    if (!bridgeInsteadOfRaft && key[0ULL] == 'B')
      bridgeInsteadOfRaft = true;

    ++uniqueConstraints;
  }

  double& maxLoad{details.maxLoad};
  static const vector<string> maxLoadSynonyms{"RaftMaxLoad", "BridgeMaxLoad"};
  if (onlyOneExpected(crossingConstraintsTree, maxLoadSynonyms, key)) {
    try {
      maxLoad = crossingConstraintsTree.get<double>(key);
    } catch (const ptree_bad_data& ex) {
      throw domain_error{HERE.function_name() +
                         " - Bad type for the raft max load! "s + ex.what()};
    }

    if (maxLoad <= 0.)
      throw domain_error{HERE.function_name() +
                         " - The raft max load cannot be negative or zero! "s};

    // If 1st entity has no specified weight, then none have.
    // However max load requires those properties
    if (!firstEntity->weight())
      throw domain_error{
          HERE.function_name() +
          " - Please specify strictly positive weights for all entities "
          "when using the `"s +
          key + "` constraint!"s};

    capManager.setMaxLoad(maxLoad);

    if (!bridgeInsteadOfRaft && key[0ULL] == 'B')
      bridgeInsteadOfRaft = true;

    ++uniqueConstraints;
  }

  shared_ptr<const IValues<double>>& allowedLoads{details.allowedLoads};
  static const vector<string> allowedLoadsSynonyms{"AllowedRaftLoads",
                                                   "AllowedBridgeLoads"};
  if (onlyOneExpected(crossingConstraintsTree, allowedLoadsSynonyms, key)) {
    allowedLoads = grammar::parseAllowedLoadsExpr(
        crossingConstraintsTree.get<string>(key));
    if (!allowedLoads)
      throw domain_error{
          HERE.function_name() +
          " - AllowedRaftLoads parsing error! See the cause above."s};

    // If 1st entity has no specified weight, then none have.
    // However max load requires these properties
    if (!firstEntity->weight())
      throw domain_error{
          HERE.function_name() +
          " - Please specify strictly positive weights for all entities "
          "when using the `"s +
          key + "` constraint!"s};

    if (!bridgeInsteadOfRaft && key[7ULL] == 'B')
      bridgeInsteadOfRaft = true;

    ++uniqueConstraints;
  }

  // Keep this after capacitySynonyms, maxLoadSynonyms and allowedLoadsSynonyms
  details.createTransferConstraintsExt();

  static const vector<string> trConfigSpecifiers{
      "AllowedRaftConfigurations", "AllowedBridgeConfigurations",
      "DisallowedRaftConfigurations", "DisallowedBridgeConfigurations"};
  if (onlyOneExpected(crossingConstraintsTree, trConfigSpecifiers, key)) {
    std::optional<grammar::ConstraintsVec> readConstraints =
        grammar::parseConfigurationsExpr(
            crossingConstraintsTree.get<string>(key));
    if (!readConstraints)
      throw domain_error{HERE.function_name() +
                         " - Constraints parsing error! See the cause above."s};

    // key starts either with 'A' or with 'D':
    // AllowedRaftConfigurations or AllowedBridgeConfigurations
    // DisallowedRaftConfigurations or DisallowedBridgeConfigurations
    const bool allowed{key[0ULL] == 'A'};
    if (!allowed)  // ensure that DisallowedRaftConfigurations contains the
                   // empty set
      (*readConstraints).push_back(make_shared<const IdsConstraint>());

    try {
      details.transferConstraints = make_unique<const TransferConstraints>(
          std::move(*readConstraints), *entities, capManager.getCapacity(),
          allowed, *details.transferConstraintsExt);
    } catch (const logic_error& e) {
      throw domain_error{e.what()};
    }

    capManager.setTransferConstraints(*details.transferConstraints);

    if (!bridgeInsteadOfRaft &&
        ((allowed && key[7ULL] == 'B') || (!allowed && key[10ULL] == 'B')))
      bridgeInsteadOfRaft = true;

    ++uniqueConstraints;

  } else {  // no [dis]allowed raft/bridge configurations constraint
    // Create a constraint checking only the capacity & maxLoad for the
    // raft/bridge
    details.transferConstraints = make_unique<const TransferConstraints>(
        grammar::ConstraintsVec{}, *entities, capManager.getCapacity(), false,
        *details.transferConstraintsExt);
  }

  // Keep this after capacitySynonyms, maxLoadSynonyms and allowedLoadsSynonyms
  vector<ConfigurationsTransferDuration>& ctdItems{details.ctdItems};
  if (crossingConstraintsTree.count("CrossingDurationsOfConfigurations") >
      0ULL) {
    const ptree cdcTree{
        crossingConstraintsTree.get_child("CrossingDurationsOfConfigurations")};
    if (!cdcTree.count(""))
      throw domain_error{
          HERE.function_name() +
          " - The CrossingDurationsOfConfigurations section should be an array "
          "of 1 or more such items!"s};

    unordered_set<unsigned> durations;
    for (const auto& cdcPair : cdcTree) {
      if (!cdcPair.first.empty())
        throw domain_error{HERE.function_name() +
                           " - The CrossingDurationsOfConfigurations section "
                           "should be an array "
                           "of 1 or more such items!"s};
      const string cdcStr{cdcPair.second.data()};
      std::optional<grammar::ConfigurationsTransferDurationInitType> readCdc{
          grammar::parseCrossingDurationForConfigurationsExpr(cdcStr)};
      if (!readCdc)
        throw domain_error{
            HERE.function_name() +
            " - CrossingDurationsOfConfigurations parsing error! "
            "See the cause above."s};
      ctdItems.emplace_back(std::move(*readCdc), *entities,
                            capManager.getCapacity(),
                            *details.transferConstraintsExt);
      const ConfigurationsTransferDuration& ctd{ctdItems.back()};
      if (!durations.insert(ctd.duration()).second)
        throw domain_error{
            HERE.function_name() + " - Before configuration `"s +
            ctd.toString() +
            "` there was another one with the same transfer time. "
            "Please group them together instead!"s};
    }

    ++uniqueConstraints;
  }

  if (!uniqueConstraints)
    throw domain_error{
        HERE.function_name() +
        " - There must be at least one valid crossing constraint!"s};

  if (!banksConstraintsTree.empty()) {
    static const vector<string> bankConfigSpecifiers{
        "AllowedBankConfigurations", "DisallowedBankConfigurations"};
    if (onlyOneExpected(banksConstraintsTree, bankConfigSpecifiers, key)) {
      std::optional<grammar::ConstraintsVec> readConstraints{
          grammar::parseConfigurationsExpr(
              banksConstraintsTree.get<string>(key))};
      if (!readConstraints)
        throw domain_error{
            HERE.function_name() +
            " - Constraints parsing error! See the cause above."s};

      // key starts either with 'A' or with 'D':
      // AllowedBankConfigurations  OR  DisallowedBankConfigurations
      const bool allowed{key[0ULL] == 'A'};

      // For AllowedBankConfigurations allow also start & final configurations
      if (allowed) {
        const vector<unsigned>&idsStartingFromLeftBank{
            entities->idsStartingFromLeftBank()},
            &idsStartingFromRightBank{entities->idsStartingFromRightBank()};

        // Building constraints
        IdsConstraint* pInitiallyOnLeftBank{new IdsConstraint};
        IdsConstraint* pInitiallyOnRightBank{new IdsConstraint};

        for (const unsigned id : idsStartingFromLeftBank)
          pInitiallyOnLeftBank->addMandatoryId(id);
        for (const unsigned id : idsStartingFromRightBank)
          pInitiallyOnRightBank->addMandatoryId(id);

        // Non-mutable final constraints
        shared_ptr<const IdsConstraint> initiallyOnLeftBank{
            pInitiallyOnLeftBank};
        shared_ptr<const IdsConstraint> initiallyOnRightBank{
            pInitiallyOnRightBank};

        (*readConstraints).push_back(initiallyOnLeftBank);
        (*readConstraints).push_back(initiallyOnRightBank);
      }

      try {
        details.banksConstraints = make_unique<const ConfigConstraints>(
            std::move(*readConstraints), *entities, allowed);
      } catch (const logic_error& e) {
        throw domain_error{e.what()};
      }
    }
  }

  unsigned& maxDuration{details.maxDuration};

  string nightModeExpr{"false"};
  if (!otherConstraintsTree.empty()) {
    static const vector<string> timeLimitSpecifiers{"TimeLimit"};
    if (onlyOneExpected(otherConstraintsTree, timeLimitSpecifiers, key)) {
      try {
        // get<unsigned> fails to signal negative values
        const int readMaxDuration{otherConstraintsTree.get<int>(key)};
        if (readMaxDuration <= 0)
          throw domain_error{HERE.function_name() +
                             " - TimeLimit should be > 0!"s};

        maxDuration = (unsigned)readMaxDuration;
      } catch (const ptree_bad_data& ex) {
        throw domain_error{HERE.function_name() +
                           " - Bad type for the time limit! "s + ex.what()};
      }

      // TimeLimit requires CrossingDurationsOfConfigurations constraints
      if (ctdItems.empty())
        throw domain_error{
            HERE.function_name() +
            " - Please specify a CrossingDurationsOfConfigurations section "
            "when using the `TimeLimit` constraint!"s};
    }

    static const vector<string> nightModeSpecifiers{"NightMode"};
    if (onlyOneExpected(otherConstraintsTree, nightModeSpecifiers, key))
      nightModeExpr = otherConstraintsTree.get<string>(key);
  }
  nightMode = nightModeSemantic(nightModeExpr);

  if (firstEntity->weight() > 0.  // all entities have then specified weights
      && maxLoad == DBL_MAX && !allowedLoads)
    throw domain_error{
        HERE.function_name() +
        " - Unnecessary weights of entities when not using none of the "
        "following constraints: "
        "{RaftMaxLoad/BridgeMaxLoad or AllowedRaftLoads/AllowedBridgeLoads}!"s};

  if (!ctdItems.empty() && maxDuration == UINT_MAX)
    throw domain_error{HERE.function_name() +
                       " - Unnecessary CrossingDurationsOfConfigurations "
                       "when not using the TimeLimit constraint!"s};

  if (solveNow)
    ignore = solution(true, interactiveSol);
}

void Scenario::outputResults(const Results& res,
                             bool interactiveSol /* = false*/) const {
  if (!res.attempt)
    throw logic_error{
        HERE.function_name() +
        " - cannot be called when the scenario wasn't / couldn't be solved "
        "based on the requested algorithm!"s};

  if (!interactiveSol || !res.attempt->isSolution()) {
    cout << "Considered scenario:\n" << *this << "\n\n";
    cout << res;
    return;
  }

  const unsigned solLen{(unsigned)res.attempt->length()};
  const shared_ptr<const sol::IState> initialState{res.attempt->initialState()};
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
                           shared_ptr<const sol::IState> state,
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
    const shared_ptr<const sol::IState> resultedState{aMove.resultedState()};

    addMove(aMove.resultedState(), &aMove.movedEntities());
  }

  root.put_child("Moves", movesTree);

  write_json(cout, root);
}

shared_ptr<const cond::IContextValidator>
ScenarioDetails::createTransferValidator() const {
  const shared_ptr<const cond::IContextValidator>& res{
      cond::DefContextValidator::SHARED_INST()};
  if (!allowedLoads)
    return res;

  return make_shared<const AllowedLoadsValidator>(
      allowedLoads, res,
      make_shared<const InitiallyNoPrevRaftLoadExcHandler>(allowedLoads));
}

void ScenarioDetails::createTransferConstraintsExt() {
  auto res{cond::DefTransferConstraintsExt::NEW_INST()};
  if (maxLoad == DBL_MAX) {
    transferConstraintsExt = std::move(res);
    return;
  }

  transferConstraintsExt =
      make_unique<const MaxLoadTransferConstraintsExt>(maxLoad, std::move(res));
}

unique_ptr<ent::IMovingEntitiesExt> ScenarioDetails::createMovingEntitiesExt()
    const {
  unique_ptr<ent::IMovingEntitiesExt> res{make_unique<DefMovingEntitiesExt>()};

  if (!allowedLoads && maxLoad == DBL_MAX)
    return res;

  return make_unique<TotalLoadExt>(entities, 0., std::move(res));
}

void Scenario::Results::update(size_t attemptLen,
                               size_t crtDistToSol,
                               const ent::BankEntities& currentLeftBank,
                               size_t& bestMinDistToGoal) noexcept {
  if (attemptLen > longestInvestigatedPath)
    longestInvestigatedPath = attemptLen;

  if (bestMinDistToGoal > crtDistToSol) {
    bestMinDistToGoal = crtDistToSol;
    closestToTargetLeftBank = {currentLeftBank};

  } else if (bestMinDistToGoal == crtDistToSol) {
    closestToTargetLeftBank.push_back(currentLeftBank);
  }
}

const string& Scenario::description() const noexcept {
  return descr;
}

string Scenario::toString() const {
  ostringstream oss;
  oss << details.toString();
  return oss.str();
}

string ScenarioDetails::toString() const {
  ostringstream oss;
  if (entities)
    oss << *entities << endl;

  oss << "CrossingConstraints: { ";
  if (capacity != UINT_MAX)
    oss << "Capacity = " << capacity << "; ";
  if (maxLoad != DBL_MAX)
    oss << "MaxLoad = " << maxLoad << "; ";
  if (transferConstraints && !transferConstraints->empty())
    oss << "TransferConstraints = " << *transferConstraints << "; ";
  if (allowedLoads)
    oss << "AllowedLoads = `" << *allowedLoads << "`; ";
  if (!ctdItems.empty()) {
    oss << "CrossingDurations: { " << *cbegin(ctdItems);
    for (const auto& cdtItem : ctdItems | views::drop(1))
      oss << " ; " << cdtItem;
    oss << " }; ";
  }

  // Replace last '; ' with ' }'
  oss.seekp(-2, ios_base::cur);
  oss << " }";

  if (banksConstraints && !banksConstraints->empty())
    oss << "\nBanksConstraints = " << *banksConstraints;

  if (maxDuration != UINT_MAX) {
    oss << "\nOtherConstraints: { ";
    oss << "TimeLimit = " << maxDuration << " time units }";
  }

  return oss.str();
}

}  // namespace rc

namespace std {

ostream& operator<<(ostream& os, const rc::Scenario& sc) {
  os << sc.toString();
  return os;
}

ostream& operator<<(ostream& os, const rc::ScenarioDetails& sc) {
  os << sc.toString();
  return os;
}

ostream& operator<<(ostream& os, const rc::Scenario::Results& o) {
  const gsl::not_null<std::shared_ptr<const rc::sol::IAttempt>> attempt{
      o.attempt};
  if (attempt->isSolution()) {
    os << "Found solution using " << attempt->length() << " steps:\n\n"
       << *attempt;
  } else {
    os << "Found no solution. "
          "Longest investigated path: "
       << o.longestInvestigatedPath
       << ". Investigated states: " << o.investigatedStates
       << ". Nearest states to the solution:\n";
    for (const rc::ent::BankEntities& leftBank : o.closestToTargetLeftBank)
      os << leftBank << " - " << ~leftBank << '\n';
  }
  return os;
}

}  // namespace std

#ifndef UNIT_TESTING

#include <fstream>

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
      throw runtime_error{HERE.function_name() +
                          "Couldn't locate the base folder of the project or "
                          "the Scenarios folder! "
                          "Please launch the program within (a subfolder of) "
                          "RiverCrossing directory!"s};

    if (!exists(scenariosDir))
      throw runtime_error{HERE.function_name() +
                          "Couldn't locate the Scenarios folder!"s};

    const directory_iterator itEnd;
    for (directory_iterator it{scenariosDir}; it != itEnd; ++it) {
      fs::path file{it->path()};
      if (file.extension().string() != ".json")
        continue;

      _scenarios.emplace_back(std::move(file));
    }
  }

  [[nodiscard]] const vector<fs::path>& scenarios() const noexcept {
    return _scenarios;
  }

 private:
  fs::path projDir;
  fs::path scenariosDir;

  vector<fs::path> _scenarios;
};

}  // anonymous namespace

int main(int argc, zstring* argv) try {
  Expects(argv && argc >= 1);

  std::span<zstring> args{argv, (size_t)argc};

  const bool interactive{(size(args) >= 2ULL) &&
                         (!strcmp("interactive", args[1ULL]))};

#ifndef NDEBUG
  cout << "Interactive:" << boolalpha << interactive << endl;
#endif  // NDEBUG

  Config cfg;
  Scenario scenario{cin, /*solveNow = */ false};
  const Scenario::Results& sol{
      scenario.solution(/*usingBFS = */ true, interactive)};
  if (!sol.attempt->isSolution())
    return -1;

  return 0;

} catch (const exception& e) {
  cerr << e.what() << endl;
  return -1;
}

#endif  // UNIT_TESTING not defined
