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
// This keeps precompiled.h first; Otherwise header sorting might move it

#include "mathRelated.h"
#include "transferredLoadExt.h"
#include "util.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <sstream>

#include <gsl/pointers>
#include <gsl/util>

using namespace std;

namespace rc {

namespace ent {

void TotalLoadExt::_newGroup(const set<unsigned>& ids) {
  load = accumulate(CBOUNDS(ids), 0., [this](double prevSum, unsigned id) {
    return prevSum + (*all)[id]->weight();
  });
}

void TotalLoadExt::_addEntity(unsigned id) {
  load += (*all)[id]->weight();
}

void TotalLoadExt::_removeEntity(unsigned id) {
  load -= (*all)[id]->weight();
  assert(load > -Eps);
}

void TotalLoadExt::_addMovePostProcessing(SymbolsTable& SymTb) const noexcept {
  // The initial fake move moves no entities, so the load is 0,
  // thus no need to update the Symbols Table
  if (load > Eps)
    SymTb["PreviousRaftLoad"] = load;
}

void TotalLoadExt::_removeMovePostProcessing(
    SymbolsTable& SymTb) const noexcept {
  if (load > Eps)
    SymTb["PreviousRaftLoad"] = load;
  else
    // The initial fake move moves no entities, so the load is 0,
    // thus `PreviousRaftLoad` must be removed from the Symbols Table
    SymTb.erase("PreviousRaftLoad");
}

unique_ptr<IMovingEntitiesExt> TotalLoadExt::_clone(
    unique_ptr<IMovingEntitiesExt> cloneOfNextExt) const noexcept {
  return make_unique<TotalLoadExt>(all, load, std::move(cloneOfNextExt));
}

string TotalLoadExt::_toString(
    bool suffixesInsteadOfPrefixes /* = true*/) const {
  // Suffix information
  if (!suffixesInsteadOfPrefixes)
    return {};

  ostringstream oss;
  oss << " weighing " << load;
  return oss.str();
}

TotalLoadExt::TotalLoadExt(const shared_ptr<const AllEntities>& all_,
                           double load_ /* = 0.*/,
                           unique_ptr<IMovingEntitiesExt> nextExt_
                           /* = make_unique<DefMovingEntitiesExt>()*/) noexcept
    : AbsMovingEntitiesExt{all_, std::move(nextExt_)}, load{load_} {}

}  // namespace ent

namespace cond {

void MaxLoadValidatorExt::checkTypesCfg(const TypesConstraint& cfg,
                                        const ent::AllEntities& allEnts) const {
  const map<string, set<unsigned>>& idsByTypes{allEnts.idsByTypes()};

  double minConfigWeight{};
  for (const auto& typeAndLimits : cfg.mandatoryTypeNames()) {
    const string& t{typeAndLimits.first};
    const set<unsigned>& matchingIds{idsByTypes.at(t)};
    const size_t minLim{(size_t)typeAndLimits.second.first};
    assert(minLim <= size(matchingIds));

    // Add the lightest minLim entities of this type
    multiset<double> weightsOfType;
    for (const unsigned id : matchingIds)
      weightsOfType.insert(allEnts[id]->weight());
    const auto weightsOfTypeBegin = cbegin(weightsOfType);
    minConfigWeight += accumulate(
        weightsOfTypeBegin, next(weightsOfTypeBegin, (ptrdiff_t)minLim), 0.);
  }

  if (minConfigWeight - Eps > _maxLoad)
    throw logic_error{HERE.function_name() + " - Constraint `"s +
                      cfg.toString() + "` produces a load >= "s +
                      to_string(minConfigWeight) +
                      ", which is more than the maximum allowed load ("s +
                      to_string(_maxLoad) + ")!"s};
}

/// @throw logic_error if the ids configuration does not respect current
/// extension
void MaxLoadValidatorExt::checkIdsCfg(const IdsConstraint&,
                                      const ent::AllEntities&) const {
  // Checking the maxLoad constraint is unpractical here,
  // because of the extra id-s (a count of mandatory unspecified entities)
}

MaxLoadValidatorExt::MaxLoadValidatorExt(
    double maxLoad_,
    unique_ptr<const IConfigConstraintValidatorExt> nextExt_
    /* = DefConfigConstraintValidatorExt::NEW_INST()*/) noexcept
    : AbsConfigConstraintValidatorExt{std::move(nextExt_)}, _maxLoad{maxLoad_} {
  assert(_maxLoad > 0.);
}

double MaxLoadValidatorExt::maxLoad() const noexcept {
  return _maxLoad;
}

unique_ptr<const IConfigConstraintValidatorExt>
MaxLoadTransferConstraintsExt::_configValidatorExt(
    unique_ptr<const IConfigConstraintValidatorExt> fromNextExt)
    const noexcept {
  return make_unique<const MaxLoadValidatorExt>(*_maxLoad,
                                                std::move(fromNextExt));
}

bool MaxLoadTransferConstraintsExt::_check(
    const ent::MovingEntities& cfg) const {
  const gsl::not_null<const ent::TotalLoadExt*> totalLoadExt{
      ent::AbsMovingEntitiesExt::selectExt<ent::TotalLoadExt>(
          cfg.getExtension())};

  const double entsWeight{totalLoadExt->totalLoad()};
  if (entsWeight - Eps > *_maxLoad) {
#ifndef NDEBUG
    const auto& entsIds{cfg.ids()};
    cout << "violates maxWeight constraint [ " << entsWeight << " > "
         << *_maxLoad << " ] : " << ContView{entsIds, {"", " ", "\n"}};
#endif  // NDEBUG
    return false;
  }

  return true;
}

MaxLoadTransferConstraintsExt::MaxLoadTransferConstraintsExt(
    const double& maxLoad_,
    unique_ptr<const ITransferConstraintsExt> nextExt_
    /* = DefTransferConstraintsExt::NEW_INST()*/)
    : AbsTransferConstraintsExt{std::move(nextExt_)}, _maxLoad{&maxLoad_} {
  assert(*_maxLoad > 0.);
}

double MaxLoadTransferConstraintsExt::maxLoad() const noexcept {
  return *_maxLoad;
}

InitiallyNoPrevRaftLoadExcHandler::InitiallyNoPrevRaftLoadExcHandler(
    const shared_ptr<const IValues<double>>& allowedLoads)
    : _allowedLoads{allowedLoads},
      dependsOnPreviousRaftLoad{
          _allowedLoads->dependsOnVariable("PreviousRaftLoad")} {
  if (_allowedLoads->empty())
    throw invalid_argument{HERE.function_name() +
                           " - doesn't accept empty allowedLoads parameter! "
                           "Some loads must be allowed!"s};
}

boost::logic::tribool InitiallyNoPrevRaftLoadExcHandler::assess(
    const ent::MovingEntities&,
    const SymbolsTable& st) const noexcept {
  // Ensure first that the allowed loads depend on `PreviousRaftLoad`
  if (!dependsOnPreviousRaftLoad)
    return boost::logic::indeterminate;

  const auto stEnd = cend(st), itCrossingIndex = st.find("CrossingIndex");
  const bool isInitialState{
      !st.contains("PreviousRaftLoad")            // missing PreviousRaftLoad
      && (itCrossingIndex != stEnd)               // existing CrossingIndex
      && (itCrossingIndex->second <= 1. + Eps)};  // CrossingIndex <= 1
  if (isInitialState)
    return true;

  return boost::logic::indeterminate;
}

bool AllowedLoadsValidator::doValidate(const ent::MovingEntities& ents,
                                       const SymbolsTable& st) const {
  const gsl::not_null<const ent::TotalLoadExt*> totalLoadExt{
      ent::AbsMovingEntitiesExt::selectExt<ent::TotalLoadExt>(
          ents.getExtension())};

  const double entsWeight{totalLoadExt->totalLoad()};
  const bool valid{_allowedLoads->contains(entsWeight, st)};
#ifndef NDEBUG
  if (!valid)
    cout << "Invalid load [" << entsWeight << " outside " << *_allowedLoads
         << "] : " << ContView{ents.ids(), {"", " ", "\n"}};
#endif  // NDEBUG
  return valid;
}

AllowedLoadsValidator::AllowedLoadsValidator(
    const shared_ptr<const IValues<double>>& allowedLoads,
    const shared_ptr<const IContextValidator>& nextValidator_
    /* = DefContextValidator::SHARED_INST()*/,
    const shared_ptr<const IValidatorExceptionHandler>& ownValidatorExcHandler_
    /* = {}*/) noexcept
    : AbsContextValidator{nextValidator_, ownValidatorExcHandler_},
      _allowedLoads{allowedLoads} {}

}  // namespace cond

namespace sol {

unique_ptr<const IStateExt> PrevLoadStateExt::_clone(
    const shared_ptr<const IStateExt>& nextExt_) const noexcept {
  return make_unique<const PrevLoadStateExt>(crossingIndex, previousRaftLoad,
                                             *info, nextExt_);
}

bool PrevLoadStateExt::_isNotBetterThan(const IState& s2) const {
  const gsl::not_null<shared_ptr<const IStateExt>> extensions2{
      s2.getExtension()};
  const gsl::not_null<shared_ptr<const PrevLoadStateExt>> prevLoadStateExt2{
      AbsStateExt::selectExt<PrevLoadStateExt>(extensions2)};

  const double otherPreviousRaftLoad{prevLoadStateExt2->previousRaftLoad};

  /*
  PreviousRaftLoad on NaN means the initial state.
  Whenever the algorithm reaches back to initial state
  (by advancing, not backtracking), it should backtrack,
  as the length of the solution would be longer (when continuing in this manner)
  than the length of the solution starting fresh with the move about to consider
  next from this revisited initial state.
  */
  if (isnan(otherPreviousRaftLoad))
    return true;  // deciding to disallow revisiting the initial state

  return abs(previousRaftLoad - otherPreviousRaftLoad) < Eps;
}

shared_ptr<const IStateExt> PrevLoadStateExt::_extensionForNextState(
    const ent::MovingEntities& movedEnts,
    const shared_ptr<const IStateExt>& fromNextExt) const {
  const gsl::not_null<const ent::TotalLoadExt*> totalLoadExt{
      ent::AbsMovingEntitiesExt::selectExt<ent::TotalLoadExt>(
          movedEnts.getExtension())};

  const double entsWeight{totalLoadExt->totalLoad()};
  return make_shared<const PrevLoadStateExt>(crossingIndex + 1U, entsWeight,
                                             *info, fromNextExt);
}

string PrevLoadStateExt::_detailsForDemo() const {
  if (isnan(previousRaftLoad))
    return {};

  ostringstream oss;
  oss << "; Previous transferred load: " << previousRaftLoad;
  return oss.str();
}

string PrevLoadStateExt::_toString(
    bool suffixesInsteadOfPrefixes /* = true*/) const {
  // This is displayed only as suffix information
  if (!suffixesInsteadOfPrefixes)
    return {};

  if (isnan(previousRaftLoad))
    return {};

  ostringstream oss;
  oss << " ; PrevRaftLoad: " << previousRaftLoad;
  return oss.str();
}

PrevLoadStateExt::PrevLoadStateExt(unsigned crossingIndex_,
                                   double previousRaftLoad_,
                                   const ScenarioDetails& info_,
                                   const shared_ptr<const IStateExt>& nextExt_
                                   /*= DefStateExt::SHARED_INST()*/) noexcept
    : AbsStateExt{info_, nextExt_},
      previousRaftLoad{previousRaftLoad_},
      crossingIndex{crossingIndex_} {}

PrevLoadStateExt::PrevLoadStateExt(const SymbolsTable& symbols,
                                   const ScenarioDetails& info_,
                                   const shared_ptr<const IStateExt>& nextExt_
                                   /*= DefStateExt::SHARED_INST()*/)
    : AbsStateExt{info_, nextExt_} {
  // PreviousRaftLoad should miss from Symbols Table when CrossingIndex <= 1
  const auto stEnd = cend(symbols);
  const auto itCrossingIndex = symbols.find("CrossingIndex"),
             itPreviousRaftLoad = symbols.find("PreviousRaftLoad");
  if (itCrossingIndex == stEnd)
    throw logic_error{HERE.function_name() +
                      " - needs to get `symbols` table containing an entry for "
                      "CrossingIndex!"s};

  crossingIndex = gsl::narrow_cast<unsigned>(
      floor(.5 + itCrossingIndex->second));  // rounded value
  if (itPreviousRaftLoad == stEnd) {
    if (crossingIndex >= 2U)
      throw logic_error{HERE.function_name() +
                        " - needs to get `symbols` table containing an entry "
                        "for PreviousRaftLoad "
                        "when the CrossingIndex entry is >= 2 !"s};

    previousRaftLoad = numeric_limits<double>::quiet_NaN();

  } else
    previousRaftLoad = itPreviousRaftLoad->second;
}

double PrevLoadStateExt::prevRaftLoad() const noexcept {
  return previousRaftLoad;
}

unsigned PrevLoadStateExt::crossingIdx() const noexcept {
  return crossingIndex;
}

}  // namespace sol
}  // namespace rc
