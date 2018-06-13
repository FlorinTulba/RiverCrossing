/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#include "rowAbilityExt.h"
#include "entitiesManager.h"

using namespace std;

namespace rc {
namespace cond {

bool CanRowValidator::doValidate(const ent::MovingEntities &ents,
                                 const SymbolsTable &st) const {
  const bool valid = ents.anyRowCapableEnts(st);
#ifndef NDEBUG
  if( ! valid) {
    cout<<"Nobody rows now : ";
    copy(CBOUNDS(ents.ids()), ostream_iterator<unsigned>(cout, " "));
    cout<<endl;
  }
#endif // NDEBUG
  return valid;
}

CanRowValidator::CanRowValidator(
  const shared_ptr<const IContextValidator> &nextValidator_
    /* = DefContextValidator::INST()*/,
  const shared_ptr<const IValidatorExceptionHandler> &ownValidatorExcHandler_
    /* = nullptr*/) :
  AbsContextValidator(nextValidator_, ownValidatorExcHandler_) {}

} // namespace cond
} // namespace rc
