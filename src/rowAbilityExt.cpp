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

#include "configConstraint.h"
#include "entitiesManager.h"
#include "rowAbilityExt.h"
#include "symbolsTable.h"

#ifndef NDEBUG
#include "util.h"

#include <cstdio>

#include <print>
#endif  // NDEBUG not defined

#include <memory>

using namespace std;

namespace rc::cond {

bool CanRowValidator::doValidate(const ent::MovingEntities& ents,
                                 const SymbolsTable& st) const {
  const bool valid{ents.anyRowCapableEnts(st)};
#ifndef NDEBUG
  if (!valid) {
    println("Nobody rows now : {}", ContView{ents.ids(), " "});
    fflush(stdout);
  }
#endif  // NDEBUG
  return valid;
}

CanRowValidator::CanRowValidator(
    const shared_ptr<const IContextValidator>& nextValidator_
    /* = DefContextValidator::SHARED_INST()*/,
    const shared_ptr<const IValidatorExceptionHandler>& ownValidatorExcHandler_
    /* = {}*/) noexcept
    : AbsContextValidator{nextValidator_, ownValidatorExcHandler_} {}

}  // namespace rc::cond
