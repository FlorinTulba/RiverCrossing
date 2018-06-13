/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 @2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_ROW_ABILITY_EXT
#define H_ROW_ABILITY_EXT

#include "configConstraint.h"

namespace rc {
namespace cond {

/// Can row validator
class CanRowValidator : public AbsContextValidator {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  /// @return true if `ents` is a valid raft/bridge configuration within `st` context
  bool doValidate(const ent::MovingEntities &ents,
                  const SymbolsTable &st) const override;

public:
  CanRowValidator(
    const std::shared_ptr<const IContextValidator> &nextValidator_
      = DefContextValidator::INST(),
    const std::shared_ptr<const IValidatorExceptionHandler> &ownValidatorExcHandler_
      = nullptr);
};

} // namespace cond
} // namespace rc

#endif // H_ROW_ABILITY_EXT not defined
