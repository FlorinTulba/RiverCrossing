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
// This keeps precompiled.h first; Otherwise header sorting might move it

#include "absSolution.h"
#include "configConstraint.h"
#include "durationExt.h"
#include "entitiesManager.h"
#include "util.h"

#include <cassert>

#ifndef NDEBUG
#include <cstdio>
#endif  // NDEBUG not defined

#include <format>
#include <memory>
#include <string>

#ifndef NDEBUG
#include <print>
#endif  // NDEBUG not defined

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
    println("violates duration constraint [{} > {}]", _time, info->maxDuration);
    fflush(stdout);
#endif             // NDEBUG
    return false;  // NOLINT(readability-simplify-boolean-expr)
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
    throw domain_error{format(
        "{} - Provided CrossingDurationsOfConfigurations items don't cover "
        "raft configuration: {}",
        HERE.function_name(), movedEnts)};

  return make_shared<const TimeStateExt>(timeOfNextState, *info, fromNextExt);
}

string TimeStateExt::_detailsForDemo() const {
  return format("; Elapsed time units: {}", _time);
}

void TimeStateExt::_formatPrefixTo(FmtCtxIt& outIt) const {
  outIt = format_to(outIt, "[Elapsed time units: {:4}] ", _time);
}

TimeStateExt::TimeStateExt(unsigned time_,
                           const rc::ScenarioDetails& info_,
                           const shared_ptr<const IStateExt>& nextExt_
                           /*= DefStateExt::SHARED_INST()*/) noexcept
    : AbsStateExt{info_, nextExt_},
      _time{time_} {}

unsigned TimeStateExt::time() const noexcept {
  return _time;
}

}  // namespace rc::sol
