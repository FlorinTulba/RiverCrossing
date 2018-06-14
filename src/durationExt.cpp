/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#include "durationExt.h"
#include "configConstraint.h"
#include "entitiesManager.h"

#include <iomanip>
#include <memory>
#include <cassert>

using namespace std;

namespace rc {
namespace sol {

shared_ptr<const IStateExt>
    TimeStateExt::_clone(const shared_ptr<const IStateExt> &nextExt_) const {
  return make_shared<const TimeStateExt>(_time, info, nextExt_);
}

bool TimeStateExt::_validate() const {
  if(_time > info.maxDuration) {
#ifndef NDEBUG
    cout<<"violates duration constraint ["
      <<_time<<" > "<<info.maxDuration<<']'<<endl;
#endif // NDEBUG
    return false;
  }
  return true;
}

bool TimeStateExt::_isNotBetterThan(const IState &s2) const {
  const shared_ptr<const IStateExt> extensions2 = s2.getExtension();
  assert(nullptr != extensions2);

  shared_ptr<const TimeStateExt> timeExt2 =
    AbsStateExt::selectExt<TimeStateExt>(extensions2);

  // if the other state was reached earlier, it is better
  return _time >=
    CP_EX_MSG(timeExt2,
              logic_error,
              "The parameter must be a state "
              "with a TimeStateExt extension!")->_time;
}

shared_ptr<const IStateExt>
    TimeStateExt::_extensionForNextState(const ent::MovingEntities &movedEnts,
                           const shared_ptr<const IStateExt> &fromNextExt) const {
  unsigned timeOfNextState = _time;
  bool foundMatch = false;
  for(const cond::ConfigurationsTransferDuration &ctdItem : info.ctdItems) {
    const cond::TransferConstraints& config = ctdItem.configConstraints();
    if( ! config.check(movedEnts))
      continue;

    foundMatch = true;
    timeOfNextState += ctdItem.duration();
    break;
  }

  if( ! foundMatch)
    throw domain_error(string(__func__) +
      " - Provided CrossingDurationsOfConfigurations items don't cover "
      "raft configuration: "s + movedEnts.toString());

  return make_shared<const TimeStateExt>(timeOfNextState, info, fromNextExt);
}

string TimeStateExt::_toString(bool suffixesInsteadOfPrefixes/* = true*/) const {
  // This is displayed only as prefix information
  if(suffixesInsteadOfPrefixes)
    return "";

  ostringstream oss;
  oss<<"[Time "<<setw(4)<<_time<<"] ";
  return oss.str();
}

TimeStateExt::TimeStateExt(unsigned time_, const ScenarioDetails &info_,
                           const shared_ptr<const IStateExt> &nextExt_
                              /*= DefStateExt::INST()*/) :
      AbsStateExt(info_, nextExt_), _time(time_) {}

unsigned TimeStateExt::time() const {return _time;}

} // namespace sol
} // namespace rc
