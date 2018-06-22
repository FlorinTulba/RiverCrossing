/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 @2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_TRANSFERRED_LOAD_EXT
#define H_TRANSFERRED_LOAD_EXT

#include "entitiesManager.h"
#include "configConstraint.h"
#include "absSolution.h"

namespace rc {

namespace ent {

/// Extension used when the total weight of the entities from the raft/bridge matters
class TotalLoadExt : public AbsMovingEntitiesExt {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  double load; ///< total load of the raft/bridge

  /// Selecting a new group of entities for moving to the other bank
  void _newGroup(const std::set<unsigned> &ids) override;

  /// Adds a new entity to the group from the raft/bridge
  void _addEntity(unsigned id) override;

  /// Removes an existing entity from the raft/bridge
  void _removeEntity(unsigned id) override;

  /**
  Some extensions might want to change the content of the Symbols Table
  after a move is performed
  */
  void _addMovePostProcessing(SymbolsTable &SymTb) const override;

  /**
  Some extensions might want to change the content of the Symbols Table
  after a move is removed
  */
  void _removeMovePostProcessing(SymbolsTable &SymTb) const override;

  /// @return a clone of these extensions
  std::unique_ptr<IMovingEntitiesExt> _clone(
          std::unique_ptr<IMovingEntitiesExt> &&cloneOfNextExt) const override;

  /**
  Display either only suffix (most of them), or only prefix extensions.
  It needs to be called before (with param false) and after (with param true)
  the information of the moving entities
  */
  std::string _toString(bool suffixesInsteadOfPrefixes = true) const override;

public:
  TotalLoadExt(const std::shared_ptr<const AllEntities> &all_,
               double load_ = 0.,
               std::unique_ptr<IMovingEntitiesExt> &&nextExt_
                  = std::make_unique<DefMovingEntitiesExt>());

  double totalLoad() const {return load;}
};

} // namespace ent

namespace cond {

/// Extension used when the total weight of the entities from the raft/bridge matters
class MaxLoadValidatorExt : public AbsConfigConstraintValidatorExt {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  double _maxLoad; ///< max allowed total raft/bridge load

  /// @throw logic_error if the types configuration does not respect current extension
  void checkTypesCfg(const TypesConstraint &cfg,
             const std::shared_ptr<const ent::AllEntities> &allEnts) const override;

  /// @throw logic_error if the ids configuration does not respect current extension
  void checkIdsCfg(const IdsConstraint&,
             const std::shared_ptr<const ent::AllEntities>&) const override;

public:
  MaxLoadValidatorExt(double maxLoad_,
      const std::shared_ptr<const IConfigConstraintValidatorExt> &nextExt_
          = DefConfigConstraintValidatorExt::INST());

  double maxLoad() const;
};

/// Extension used when the total weight of the entities from the raft/bridge matters
class MaxLoadTransferConstraintsExt : public AbsTransferConstraintsExt {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  const double &_maxLoad; ///< max allowed total raft/bridge load

  /// @return validator extensions of a configuration
  std::shared_ptr<const IConfigConstraintValidatorExt>
    _configValidatorExt(
        const std::shared_ptr<const IConfigConstraintValidatorExt> &fromNextExt)
                              const override;

  /// @return true only if cfg satisfies current extension
  bool _check(const ent::MovingEntities &cfg) const override;

public:
  MaxLoadTransferConstraintsExt(const double &maxLoad_,
      const std::shared_ptr<const ITransferConstraintsExt> &nextExt_
          = DefTransferConstraintsExt::INST());

  double maxLoad() const;
};

/**
The validation of allowed loads generates an out_of_range exception
when looking for `PreviousRaftLoad` entry in the Symbols Table
for the initial state of the scenario to be solved.

Initially and whenever the algorithm backtracks to the initial state,
there is no previous raft load, so in those contexts
`PreviousRaftLoad` can't appear in the Symbols Table.
*/
class InitiallyNoPrevRaftLoadExcHandler : public IValidatorExceptionHandler {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  std::shared_ptr<const IValues<double>> _allowedLoads; ///< the allowed loads
  bool dependsOnPreviousRaftLoad;

public:
  InitiallyNoPrevRaftLoadExcHandler(
        const std::shared_ptr<const IValues<double>> &allowedLoads);

  /**
  Tries to detect if the algorithm is in / has back-tracked to the initial state.
  If it isn't, it returns `indeterminate`.
  If it is, it returns true, to validate any possible raft/bridge load.
  */
  boost::logic::tribool assess(const ent::MovingEntities&,
                               const SymbolsTable &st) const override;
};

/// Allowed loads validator
class AllowedLoadsValidator : public AbsContextValidator {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  std::shared_ptr<const IValues<double>> _allowedLoads; ///< the allowed loads

  /// @return true if `ents` is a valid raft/bridge configuration within `st` context
  bool doValidate(const ent::MovingEntities &ents,
                  const SymbolsTable &st) const override;

public:
  AllowedLoadsValidator(
    const std::shared_ptr<const IValues<double>> &allowedLoads,
    const std::shared_ptr<const IContextValidator> &nextValidator_
      = DefContextValidator::INST(),
    const std::shared_ptr<const IValidatorExceptionHandler> &ownValidatorExcHandler_
      = nullptr);
};

} // namespace cond

namespace sol {

/// A state decorator considering `PreviousRaftLoad` from the Symbols Table
class PrevLoadStateExt : public AbsStateExt {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  double previousRaftLoad;
  unsigned crossingIndex;

  /// Clones the State extension
  std::shared_ptr<const IStateExt>
      _clone(const std::shared_ptr<const IStateExt> &nextExt_) const override;

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
  This extensions add information about the previous raft load.
  */
  std::string _detailsForDemo() const override;

  std::string _toString(bool suffixesInsteadOfPrefixes/* = true*/) const override;

public:
  PrevLoadStateExt(unsigned crossingIndex_, double previousRaftLoad_,
                   const ScenarioDetails &info_,
                   const std::shared_ptr<const IStateExt> &nextExt_
                      = DefStateExt::INST());

  PrevLoadStateExt(const SymbolsTable &symbols,
                   const ScenarioDetails &info_,
                   const std::shared_ptr<const IStateExt> &nextExt_
                      = DefStateExt::INST());

  double prevRaftLoad() const;
  unsigned crossingIdx() const;
};

} // namespace sol
} // namespace rc

#endif // H_TRANSFERRED_LOAD_EXT
