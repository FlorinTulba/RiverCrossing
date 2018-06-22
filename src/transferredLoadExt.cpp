/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#include "transferredLoadExt.h"
#include "mathRelated.h"

#include <numeric>
#include <iostream>
#include <sstream>
#include <cmath>
#include <cassert>

using namespace std;

namespace rc {

namespace ent {

void TotalLoadExt::_newGroup(const set<unsigned> &ids) {
  load = accumulate(CBOUNDS(ids), 0.,
    [this](double prevSum, unsigned id) {
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

void TotalLoadExt::_addMovePostProcessing(SymbolsTable &SymTb) const {
  // The initial fake move moves no entities, so the load is 0,
  // thus no need to update the Symbols Table
  if(load > Eps)
    SymTb["PreviousRaftLoad"] = load;
}

void TotalLoadExt::_removeMovePostProcessing(SymbolsTable &SymTb) const {
  if(load > Eps)
    SymTb["PreviousRaftLoad"] = load;
  else
    // The initial fake move moves no entities, so the load is 0,
    // thus `PreviousRaftLoad` must be removed from the Symbols Table
    SymTb.erase("PreviousRaftLoad");
}

unique_ptr<IMovingEntitiesExt> TotalLoadExt::_clone(
          unique_ptr<IMovingEntitiesExt> &&cloneOfNextExt) const {
  return make_unique<TotalLoadExt>(all, load, std::move(cloneOfNextExt));
}

string TotalLoadExt::_toString(bool suffixesInsteadOfPrefixes/* = true*/) const {
  // Suffix information
  if( ! suffixesInsteadOfPrefixes)
    return "";

  ostringstream oss;
  oss<<" weighing "<<load;
  return oss.str();
}

TotalLoadExt::TotalLoadExt(const shared_ptr<const AllEntities> &all_,
             double load_/* = 0.*/,
             unique_ptr<IMovingEntitiesExt> &&nextExt_
                /* = make_unique<DefMovingEntitiesExt>()*/) :
    AbsMovingEntitiesExt(all_, std::move(nextExt_)), load(load_) {}

} // namespace ent

namespace cond {

void MaxLoadValidatorExt::checkTypesCfg(const TypesConstraint &cfg,
           const shared_ptr<const ent::AllEntities> &allEnts) const {
  const map<string, set<unsigned>> &idsByTypes = allEnts->idsByTypes();

  double minConfigWeight = 0.;
  for(const auto &typeAndLimits : cfg.mandatoryTypeNames()) {
    const string &t = typeAndLimits.first;
    const set<unsigned> &matchingIds = idsByTypes.at(t);
    const size_t minLim = (size_t)typeAndLimits.second.first;
    assert(minLim <= matchingIds.size());

    // Add the lightest minLim entities of this type
    multiset<double> weightsOfType;
    for(unsigned id : matchingIds)
      weightsOfType.insert((*allEnts)[id]->weight());
    const auto weightsOfTypeBegin = cbegin(weightsOfType);
    minConfigWeight +=
      accumulate(weightsOfTypeBegin, next(weightsOfTypeBegin, minLim), 0.);
  }

  if(minConfigWeight - Eps > _maxLoad)
    throw logic_error(string(__func__) + " - Constraint `"s + cfg.toString() +
      "` produces a load >= "s + to_string(minConfigWeight) +
      ", which is more than the maximum allowed load ("s + to_string(_maxLoad) + ")!"s);
}

/// @throw logic_error if the ids configuration does not respect current extension
void MaxLoadValidatorExt::checkIdsCfg(const IdsConstraint&,
           const shared_ptr<const ent::AllEntities>&) const {
  // Checking the maxLoad constraint is unpractical here,
  // because of the extra id-s (a count of mandatory unspecified entities)
}

MaxLoadValidatorExt::MaxLoadValidatorExt(double maxLoad_,
    const shared_ptr<const IConfigConstraintValidatorExt> &nextExt_
        /* = DefConfigConstraintValidatorExt::INST()*/) :
    AbsConfigConstraintValidatorExt(nextExt_), _maxLoad(maxLoad_) {
  assert(_maxLoad > 0.);
}

double MaxLoadValidatorExt::maxLoad() const {return _maxLoad;}

shared_ptr<const IConfigConstraintValidatorExt>
    MaxLoadTransferConstraintsExt::_configValidatorExt(
        const shared_ptr<const IConfigConstraintValidatorExt> &fromNextExt)
                              const {
  return make_shared<const MaxLoadValidatorExt>(_maxLoad, fromNextExt);
}

bool MaxLoadTransferConstraintsExt::_check(const ent::MovingEntities &cfg) const {
  const ent::TotalLoadExt *totalLoadExt =
    ent::AbsMovingEntitiesExt::selectExt<ent::TotalLoadExt>(cfg.getExtension());

  const double entsWeight =
    CP_EX_MSG(totalLoadExt, logic_error,
            "expects a MovingEntities parameter extended with TotalLoadExt!")
          ->totalLoad();
  if(entsWeight - Eps > _maxLoad) {
#ifndef NDEBUG
    const auto &entsIds = cfg.ids();
    cout<<"violates maxWeight constraint [ "
      <<entsWeight<<" > "<<_maxLoad<<" ] : ";
    copy(CBOUNDS(entsIds), ostream_iterator<unsigned>(cout, " "));
    cout<<endl;
#endif // NDEBUG
    return false;
  }

  return true;
}

MaxLoadTransferConstraintsExt::MaxLoadTransferConstraintsExt(const double &maxLoad_,
      const shared_ptr<const ITransferConstraintsExt> &nextExt_
          /* = DefTransferConstraintsExt::INST()*/) :
    AbsTransferConstraintsExt(nextExt_), _maxLoad(maxLoad_) {
  assert(_maxLoad > 0.);
}

double MaxLoadTransferConstraintsExt::maxLoad() const {return _maxLoad;}

InitiallyNoPrevRaftLoadExcHandler::InitiallyNoPrevRaftLoadExcHandler(
      const shared_ptr<const IValues<double>> &allowedLoads) :
    _allowedLoads(CP(allowedLoads)) {
  if(_allowedLoads->empty())
    throw invalid_argument(string(__func__) +
      " - doesn't accept empty allowedLoads parameter! "
      "Some loads must be allowed!");

  dependsOnPreviousRaftLoad =
      _allowedLoads->dependsOnVariable("PreviousRaftLoad");
}

boost::logic::tribool InitiallyNoPrevRaftLoadExcHandler::assess(
                            const ent::MovingEntities&,
                            const SymbolsTable &st) const {
  // Ensure first that the allowed loads depend on `PreviousRaftLoad`
  if( ! dependsOnPreviousRaftLoad)
    return boost::logic::indeterminate;

  const auto stEnd = cend(st),
    itCrossingIndex = st.find("CrossingIndex");
  const bool
    isInitialState =
      (st.find("PreviousRaftLoad") == stEnd)      // missing PreviousRaftLoad
      && (itCrossingIndex != stEnd)               // existing CrossingIndex
      && (itCrossingIndex->second <= 1. + Eps);   // CrossingIndex <= 1
  if(isInitialState)
    return true;

  return boost::logic::indeterminate;
}

bool AllowedLoadsValidator::doValidate(const ent::MovingEntities &ents,
                const SymbolsTable &st) const {
  const ent::TotalLoadExt *totalLoadExt =
    ent::AbsMovingEntitiesExt::selectExt<ent::TotalLoadExt>(ents.getExtension());

  const double entsWeight =
    CP_EX_MSG(totalLoadExt, logic_error,
            "expects a MovingEntities parameter extended with TotalLoadExt!")
          ->totalLoad();
  const bool valid = _allowedLoads->contains(entsWeight, st);
#ifndef NDEBUG
  if( ! valid) {
    cout<<"Invalid load ["<<entsWeight<<" outside "<<*_allowedLoads<<"] : ";
    copy(CBOUNDS(ents.ids()), ostream_iterator<unsigned>(cout, " "));
    cout<<endl;
  }
#endif // NDEBUG
  return valid;
}

AllowedLoadsValidator::AllowedLoadsValidator(
  const shared_ptr<const IValues<double>> &allowedLoads,
  const shared_ptr<const IContextValidator> &nextValidator_
    /* = DefContextValidator::INST()*/,
  const shared_ptr<const IValidatorExceptionHandler> &ownValidatorExcHandler_
    /* = nullptr*/) :
    AbsContextValidator(nextValidator_, ownValidatorExcHandler_),
    _allowedLoads(CP(allowedLoads)) {}

} // namespace cond

namespace sol {

shared_ptr<const IStateExt>
    PrevLoadStateExt::_clone(const shared_ptr<const IStateExt> &nextExt_) const {
  return make_shared<const PrevLoadStateExt>(crossingIndex, previousRaftLoad,
                                             info, nextExt_);
}

bool PrevLoadStateExt::_isNotBetterThan(const IState &s2) const {
  const shared_ptr<const IStateExt> extensions2 = s2.getExtension();
  assert(nullptr != extensions2);

  shared_ptr<const PrevLoadStateExt> prevLoadStateExt2 =
    AbsStateExt::selectExt<PrevLoadStateExt>(extensions2);

  const double otherPreviousRaftLoad =
    CP_EX_MSG(prevLoadStateExt2,
              logic_error,
              "The parameter must be a state "
              "with a PrevLoadStateExt extension!")->previousRaftLoad;

  /*
  PreviousRaftLoad on NaN means the initial state.
  Whenever the algorithm reaches back to initial state
  (by advancing, not backtracking), it should backtrack,
  as the length of the solution would be longer (when continuing in this manner)
  than the length of the solution starting fresh with the move about to consider
  next from this revisited initial state.
  */
  if(isNaN(otherPreviousRaftLoad))
    return true; // deciding to disallow revisiting the initial state

  return abs(previousRaftLoad - otherPreviousRaftLoad) < Eps;
}

shared_ptr<const IStateExt>
    PrevLoadStateExt::_extensionForNextState(const ent::MovingEntities &movedEnts,
                           const shared_ptr<const IStateExt> &fromNextExt)
                    const {
  const ent::TotalLoadExt *totalLoadExt =
    ent::AbsMovingEntitiesExt::selectExt<ent::TotalLoadExt>(movedEnts.getExtension());

  const double entsWeight =
    CP_EX_MSG(totalLoadExt, logic_error,
            "expects a MovingEntities parameter extended with TotalLoadExt!")
          ->totalLoad();
  return make_shared<const PrevLoadStateExt>(crossingIndex + 1U,
                                             entsWeight,
                                             info, fromNextExt);
}

string PrevLoadStateExt::_detailsForDemo() const {
  if(isNaN(previousRaftLoad))
    return "";

  ostringstream oss;
  oss<<"; Previous transferred load: "<<previousRaftLoad;
  return oss.str();
}

string PrevLoadStateExt::_toString(bool suffixesInsteadOfPrefixes/* = true*/) const {
  // This is displayed only as suffix information
  if( ! suffixesInsteadOfPrefixes)
    return "";

  ostringstream oss;
  oss<<" ; PrevRaftLoad: "<<previousRaftLoad;
  return oss.str();
}

PrevLoadStateExt::PrevLoadStateExt(unsigned crossingIndex_,
                                   double previousRaftLoad_,
                                   const ScenarioDetails &info_,
                                   const shared_ptr<const IStateExt> &nextExt_
                                      /*= DefStateExt::INST()*/) :
    AbsStateExt(info_, nextExt_),
    crossingIndex(crossingIndex_), previousRaftLoad(previousRaftLoad_) {}

PrevLoadStateExt::PrevLoadStateExt(const SymbolsTable &symbols,
                                   const ScenarioDetails &info_,
                                   const shared_ptr<const IStateExt> &nextExt_
                                      /*= DefStateExt::INST()*/) :
    AbsStateExt(info_, nextExt_) {
  // PreviousRaftLoad should miss from Symbols Table when CrossingIndex <= 1
  const auto stEnd = cend(symbols);
  const auto itCrossingIndex = symbols.find("CrossingIndex"),
    itPreviousRaftLoad = symbols.find("PreviousRaftLoad");
  if(itCrossingIndex == stEnd)
    throw logic_error(string(__func__) +
      " - needs to get `symbols` table containing an entry for CrossingIndex!");

  crossingIndex = (unsigned)floor(.5 + itCrossingIndex->second); // rounded value
  if(itPreviousRaftLoad == stEnd) {
    if(crossingIndex >= 2U)
      throw logic_error(string(__func__) +
        " - needs to get `symbols` table containing an entry for PreviousRaftLoad "
        "when the CrossingIndex entry is >= 2 !");

    previousRaftLoad = numeric_limits<double>::quiet_NaN();
  } else previousRaftLoad = itPreviousRaftLoad->second;
}

double PrevLoadStateExt::prevRaftLoad() const {return previousRaftLoad;}

unsigned PrevLoadStateExt::crossingIdx() const {return crossingIndex;}

} // namespace sol
} // namespace rc
