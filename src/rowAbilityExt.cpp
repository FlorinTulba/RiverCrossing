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

#include "entitiesManager.h"
#include "rowAbilityExt.h"

using namespace std;

namespace rc::cond {

bool CanRowValidator::doValidate(const ent::MovingEntities& ents,
                                 const SymbolsTable& st) const {
  const bool valid{ents.anyRowCapableEnts(st)};
#ifndef NDEBUG
  if (!valid) {
    cout << "Nobody rows now : ";
    ranges::copy(ents.ids(), ostream_iterator<unsigned>{cout, " "});
    cout << endl;
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
