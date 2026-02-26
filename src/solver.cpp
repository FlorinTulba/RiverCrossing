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
#define CPP_SOLVER
#include "solver.hpp"
#undef CPP_SOLVER

#endif  // UNIT_TESTING

#include "solverDetail.hpp"

#include "durationExt.h"
#include "scenario.h"
#include "transferredLoadExt.h"
#include "warnings.h"

#include <exception>
#include <iomanip>
#include <queue>

using namespace std;

namespace rc {

namespace sol {

MUTE_EXPLICIT_NEW_DELETE_WARN
const shared_ptr<const DefStateExt>& DefStateExt::SHARED_INST() noexcept {
  MUTE_EXIT_TIME_DTOR_WARN
  // Using new instead of make_shared, since the ctor is private
  // and not a friend of make_shared
  static const shared_ptr<const DefStateExt> inst{makeNoexcept([] {
    return shared_ptr<const DefStateExt>{
        // NOLINTNEXTLINE(bugprone-unhandled-exception-at-new)
        new DefStateExt};
  })};
  UNMUTE_WARNING

  return inst;
}

unique_ptr<const IStateExt> DefStateExt::clone() const noexcept {
  // Using new instead of make_unique, since the ctor is private
  // and not a friend of make_unique
  return unique_ptr<const IStateExt>{
      makeNoexcept([]() -> gsl::owner<DefStateExt*> {
        // NOLINTNEXTLINE(bugprone-unhandled-exception-at-new)
        return new DefStateExt;
      })};
}
UNMUTE_WARNING

shared_ptr<const IStateExt> DefStateExt::extensionForNextState(
    const ent::MovingEntities&) const noexcept {
  return DefStateExt::SHARED_INST();
}

AbsStateExt::AbsStateExt(const rc::ScenarioDetails& info_,
                         const shared_ptr<const IStateExt>& nextExt_) noexcept
    : info{&info_},
      nextExt{nextExt_} {}

unique_ptr<const IStateExt> AbsStateExt::clone() const noexcept {
  return _clone(makeNoexcept(
      [this] { return shared_ptr<const IStateExt>{nextExt->clone()}; }));
}

bool AbsStateExt::validate() const {
  return nextExt->validate() && _validate();
}

bool AbsStateExt::isNotBetterThan(const IState& s2) const {
  return nextExt->isNotBetterThan(s2) && _isNotBetterThan(s2);
}

shared_ptr<const IStateExt> AbsStateExt::extensionForNextState(
    const ent::MovingEntities& me) const {
  const shared_ptr<const IStateExt> fromNextExt{
      nextExt->extensionForNextState(me)};
  return _extensionForNextState(me, fromNextExt);
}

string AbsStateExt::detailsForDemo() const {
  return _detailsForDemo() + nextExt->detailsForDemo();
}

string AbsStateExt::toString(bool suffixesInsteadOfPrefixes /* = true*/) const {
  // Only the matching extension categories will return non-empty strings
  // given suffixesInsteadOfPrefixes
  // (some display only as prefixes, the rest only as suffixes)
  return _toString(suffixesInsteadOfPrefixes) +
         nextExt->toString(suffixesInsteadOfPrefixes);
}

}  // namespace sol

const SymbolsTable& InitialSymbolsTable() noexcept {
  MUTE_EXIT_TIME_DTOR_WARN
  static const SymbolsTable st{
      makeNoexcept([] { return SymbolsTable{{"CrossingIndex", 0.}}; })};
  UNMUTE_WARNING

  return st;
}

unique_ptr<const sol::IState> ScenarioDetails::createInitialState(
    const SymbolsTable& SymTb) const {
  using namespace ent;
  using namespace sol;

  shared_ptr<const IStateExt> stateExt{DefStateExt::SHARED_INST()};

  if (allowedLoads && allowedLoads->dependsOnVariable("PreviousRaftLoad"))
    // stateExt is extended, not replaced
    stateExt = make_shared<const PrevLoadStateExt>(SymTb, *this, stateExt);

  if (maxDuration != UINT_MAX)
    // stateExt is extended, not replaced
    stateExt = make_shared<const TimeStateExt>(0U, *this, stateExt);

  return make_unique<const State>(
      BankEntities(entities, entities->idsStartingFromLeftBank()),
      BankEntities(entities, entities->idsStartingFromRightBank()),
      true,  // always start from left bank
      stateExt);
}

const Scenario::Results& Scenario::solution(bool usingBFS /* = true*/,
                                            bool interactiveSol /* = false*/) {
  const Results* results{};
  if (usingBFS) {
    if (!investigatedByBFS) {
      Solver solver{details, resultsBFS};
      solver.run(usingBFS);

      investigatedByBFS = true;
    }

    results = &resultsBFS;

  } else {
    if (!investigatedByDFS) {
      Solver solver{details, resultsDFS};
      solver.run(usingBFS);

      investigatedByDFS = true;
    }

    results = &resultsDFS;
  }

  try {
    const gsl::not_null<const Results*> safeResults{results};
    outputResults(*safeResults, interactiveSol);
  } catch (const exception&) {
    cerr << "Unable to prepare the solution animation!\n" << flush;
  }

  return *results;
}

}  // namespace rc
