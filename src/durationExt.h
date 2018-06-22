/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 @2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_DURATION_EXT
#define H_DURATION_EXT

#include "absSolution.h"

namespace rc {
namespace sol {

/// Allows State to contain a time entry - the moment the state is reached
class TimeStateExt : public AbsStateExt {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  unsigned _time; ///< the moment this state is reached

  std::shared_ptr<const IStateExt>
      _clone(const std::shared_ptr<const IStateExt> &nextExt_) const override;

  /// Validates the parameter state based on the constraints of the extension
  bool _validate() const override;

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  bool _isNotBetterThan(const IState &s2) const override;

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  std::shared_ptr<const IStateExt>
      _extensionForNextState(const ent::MovingEntities &movedEnts,
                             const std::shared_ptr<const IStateExt> &fromNextExt)
              const override;

  /**
  The browser visualizer shows various state information.
  This extension adds elapsed time information.
  */
  std::string _detailsForDemo() const override;

  std::string _toString(bool suffixesInsteadOfPrefixes/* = true*/) const override;

public:
  TimeStateExt(unsigned time_, const ScenarioDetails &info_,
               const std::shared_ptr<const IStateExt> &nextExt_ = DefStateExt::INST());

  unsigned time() const;
};

} // namespace sol
} // namespace rc

#endif // H_DURATION_EXT not defined
