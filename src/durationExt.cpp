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

#include "configConstraint.h"
#include "durationExt.h"
#include "entitiesManager.h"
#include "util.h"

#include <cassert>
#include <iomanip>
#include <memory>

#include <gsl/pointers>

using namespace std;

namespace rc::sol {

unique_ptr<const IStateExt> TimeStateExt::_clone(
    const shared_ptr<const IStateExt>& nextExt_) const noexcept {
  return make_unique<const TimeStateExt>(_time, *info, nextExt_);
}

bool TimeStateExt::_validate() const {
  if (_time > info->maxDuration) {
#ifndef NDEBUG
    cout << "violates duration constraint [" << _time << " > "
         << info->maxDuration << ']' << endl;
#endif  // NDEBUG
    return false;
  }
  return true;
}

bool TimeStateExt::_isNotBetterThan(const IState& s2) const {
  const shared_ptr<const IStateExt> extensions2{s2.getExtension()};
  const gsl::not_null<shared_ptr<const TimeStateExt>> timeExt2{
      AbsStateExt::selectExt<TimeStateExt>(extensions2)};

  // if the other state was reached earlier, it is better
  return _time >= timeExt2->_time;
}

shared_ptr<const IStateExt> TimeStateExt::_extensionForNextState(
    const ent::MovingEntities& movedEnts,
    const shared_ptr<const IStateExt>& fromNextExt) const {
  unsigned timeOfNextState{_time};
  bool foundMatch{};
  for (const cond::ConfigurationsTransferDuration& ctdItem : info->ctdItems) {
    const cond::TransferConstraints& config{ctdItem.configConstraints()};
    if (!config.check(movedEnts))
      continue;

    foundMatch = true;
    timeOfNextState += ctdItem.duration();
    break;
  }

  if (!foundMatch)
    throw domain_error{
        HERE.function_name() +
        " - Provided CrossingDurationsOfConfigurations items don't cover "
        "raft configuration: "s +
        movedEnts.toString()};

  return make_shared<const TimeStateExt>(timeOfNextState, *info, fromNextExt);
}

string TimeStateExt::_detailsForDemo() const {
  ostringstream oss;
  oss << "; Elapsed time units: " << _time;
  return oss.str();
}

string TimeStateExt::_toString(
    bool suffixesInsteadOfPrefixes /* = true*/) const {
  // This is displayed only as prefix information
  if (suffixesInsteadOfPrefixes)
    return {};

  ostringstream oss;
  oss << "[Elapsed time units: " << setw(4) << _time << "] ";
  return oss.str();
}

TimeStateExt::TimeStateExt(unsigned time_,
                           const rc::ScenarioDetails& info_,
                           const shared_ptr<const IStateExt>& nextExt_
                           /*= DefStateExt::SHARED_INST()*/) noexcept
    : AbsStateExt{info_, nextExt_}, _time{time_} {}

unsigned TimeStateExt::time() const noexcept {
  return _time;
}

}  // namespace rc::sol
