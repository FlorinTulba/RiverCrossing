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

#ifndef H_DURATION_EXT
#define H_DURATION_EXT

#include "scenarioDetails.h"

namespace rc::sol {

/// Allows State to contain a time entry - the moment the state is reached
class TimeStateExt : public AbsStateExt {
 public:
  TimeStateExt(unsigned time_,
               const rc::ScenarioDetails& info_,
               const std::shared_ptr<const IStateExt>& nextExt_ =
                   DefStateExt::SHARED_INST()) noexcept;
  TimeStateExt(const TimeStateExt&) = delete;
  void operator=(const TimeStateExt&) = delete;
  void operator=(TimeStateExt&&) = delete;

  [[nodiscard]] unsigned time() const noexcept;

  PROTECTED :

      [[nodiscard]] std::unique_ptr<const IStateExt>
      _clone(const std::shared_ptr<const IStateExt>& nextExt_)
          const noexcept override;

  /// Validates the parameter state based on the constraints of the extension
  [[nodiscard]] bool _validate() const override;

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  @throw logic_error for invalid extension
  */
  [[nodiscard]] bool _isNotBetterThan(const IState& s2) const override;

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  [[nodiscard]] std::shared_ptr<const IStateExt> _extensionForNextState(
      const ent::MovingEntities& movedEnts,
      const std::shared_ptr<const IStateExt>& fromNextExt) const override;

  /**
  The browser visualizer shows various state information.
  This extension adds elapsed time information.
  */
  [[nodiscard]] std::string _detailsForDemo() const override;

  [[nodiscard]] std::string _toString(
      bool suffixesInsteadOfPrefixes /* = true*/) const override;

  unsigned _time;  ///< the moment this state is reached
};

}  // namespace rc::sol

#endif  // H_DURATION_EXT not defined
