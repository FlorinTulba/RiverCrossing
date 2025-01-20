/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

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

namespace rc::cond {

/// Can row validator
class CanRowValidator : public AbsContextValidator {
 public:
  /// @throw invalid_argument if nextValidator_ is NULL
  CanRowValidator(const std::shared_ptr<const IContextValidator>&
                      nextValidator_ = DefContextValidator::SHARED_INST(),
                  const std::shared_ptr<const IValidatorExceptionHandler>&
                      ownValidatorExcHandler_ = {}) noexcept;
  ~CanRowValidator() noexcept override = default;

  CanRowValidator(const CanRowValidator&) = delete;
  CanRowValidator(CanRowValidator&&) = delete;
  void operator=(const CanRowValidator&) = delete;
  void operator=(CanRowValidator&&) = delete;

  PROTECTED :

      /// @return true if `ents` is a valid raft/bridge configuration within
      /// `st` context
      /// @throw logic_error if ents misses some extension(s)
      [[nodiscard]] bool
      doValidate(const ent::MovingEntities& ents,
                 const SymbolsTable& st) const override;
};

}  // namespace rc::cond

#endif  // H_ROW_ABILITY_EXT not defined
