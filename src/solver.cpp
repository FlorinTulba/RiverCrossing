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
#include "solver.hpp"  // IWYU pragma: keep

#endif  // UNIT_TESTING

#include "absSolution.h"
#include "durationExt.h"
#include "entitiesManager.h"
#include "scenario.h"
#include "solverDetail.hpp"
#include "symbolsTable.h"
#include "transferredLoadExt.h"
#include "util.h"
#include "warnings.h"

#include <climits>
#include <cstdio>

#include <exception>
#include <memory>
#include <print>
#include <string>

#include <gsl/pointers>

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
        new DefStateExt  // NOLINT(bugprone-unhandled-exception-at-new)
                         // : Handled by makeNoexcept
    };
  })};
  UNMUTE_WARNING

  return inst;
}

unique_ptr<const IStateExt> DefStateExt::clone() const noexcept {
  // Using new instead of make_unique, since the ctor is private
  // and not a friend of make_unique
  return unique_ptr<const IStateExt>{
      makeNoexcept([]() -> gsl::owner<DefStateExt*> {
        return new DefStateExt;  // NOLINT(bugprone-unhandled-exception-at-new)
                                 // : Handled by makeNoexcept
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

void AbsStateExt::formatPrefixTo(FmtCtxIt& outIt) const {
  _formatPrefixTo(outIt);
  nextExt->formatPrefixTo(outIt);
}

void AbsStateExt::formatSuffixTo(FmtCtxIt& outIt) const {
  _formatSuffixTo(outIt);
  nextExt->formatSuffixTo(outIt);
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
    println(stderr, "Unable to prepare the solution animation!");
    fflush(stderr);
  }

  return *results;
}

}  // namespace rc
