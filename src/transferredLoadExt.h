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

#ifndef H_TRANSFERRED_LOAD_EXT
#define H_TRANSFERRED_LOAD_EXT

#include "scenarioDetails.h"

namespace rc {

namespace ent {

/// Extension used when the total weight of the entities from the raft/bridge
/// matters
class TotalLoadExt : public AbsMovingEntitiesExt {
 public:
  explicit TotalLoadExt(const std::shared_ptr<const AllEntities>& all_,
                        double load_ = 0.,
                        std::unique_ptr<IMovingEntitiesExt> nextExt_ =
                            std::make_unique<DefMovingEntitiesExt>()) noexcept;
  ~TotalLoadExt() noexcept override = default;

  TotalLoadExt(const TotalLoadExt&) = delete;
  TotalLoadExt(TotalLoadExt&&) = delete;
  void operator=(const TotalLoadExt&) = delete;
  void operator=(TotalLoadExt&&) = delete;

  [[nodiscard]] double totalLoad() const noexcept { return load; }

  PROTECTED :

      /// Selecting a new group of entities for moving to the other bank
      void
      _newGroup(const std::set<unsigned>& ids) override;

  /// Adds a new entity to the group from the raft/bridge
  /// @throw out_of_range for invalid id
  void _addEntity(unsigned id) override;

  /// Removes an existing entity from the raft/bridge
  /// @throw out_of_range for invalid id
  void _removeEntity(unsigned id) override;

  /**
  Some extensions might want to change the content of the Symbols Table
  after a move is performed
  */
  void _addMovePostProcessing(SymbolsTable& SymTb) const noexcept override;

  /**
  Some extensions might want to change the content of the Symbols Table
  after a move is removed
  */
  void _removeMovePostProcessing(SymbolsTable& SymTb) const noexcept override;

  /// @return a clone of these extensions
  [[nodiscard]] std::unique_ptr<IMovingEntitiesExt> _clone(
      std::unique_ptr<IMovingEntitiesExt> cloneOfNextExt)
      const noexcept override;

  /**
  Display either only suffix (most of them), or only prefix extensions.
  It needs to be called before (with param false) and after (with param true)
  the information of the moving entities
  */
  [[nodiscard]] std::string _toString(
      bool suffixesInsteadOfPrefixes = true) const override;

  double load;  ///< total load of the raft/bridge
};

}  // namespace ent

namespace cond {

/// Extension used when the total weight of the entities from the raft/bridge
/// matters
class MaxLoadValidatorExt : public AbsConfigConstraintValidatorExt {
 public:
  explicit MaxLoadValidatorExt(
      double maxLoad_,
      std::unique_ptr<const IConfigConstraintValidatorExt> nextExt_ =
          DefConfigConstraintValidatorExt::NEW_INST()) noexcept;
  ~MaxLoadValidatorExt() noexcept override = default;

  MaxLoadValidatorExt(const MaxLoadValidatorExt&) = delete;
  MaxLoadValidatorExt(MaxLoadValidatorExt&&) = delete;
  void operator=(const MaxLoadValidatorExt&) = delete;
  void operator=(MaxLoadValidatorExt&&) = delete;

  [[nodiscard]] double maxLoad() const noexcept;

  PROTECTED :

      /// @throw logic_error if the types configuration does not respect current
      /// extension
      void
      checkTypesCfg(const TypesConstraint& cfg,
                    const ent::AllEntities& allEnts) const override;

  /// @throw logic_error if the ids configuration does not respect current
  /// extension
  void checkIdsCfg(const IdsConstraint&,
                   const ent::AllEntities&) const override;

  double _maxLoad;  ///< max allowed total raft/bridge load
};

/// Extension used when the total weight of the entities from the raft/bridge
/// matters
class MaxLoadTransferConstraintsExt : public AbsTransferConstraintsExt {
 public:
  explicit MaxLoadTransferConstraintsExt(
      const double& maxLoad_,
      std::unique_ptr<const ITransferConstraintsExt> nextExt_ =
          DefTransferConstraintsExt::NEW_INST());
  ~MaxLoadTransferConstraintsExt() noexcept override = default;

  MaxLoadTransferConstraintsExt(const MaxLoadTransferConstraintsExt&) = delete;
  MaxLoadTransferConstraintsExt(MaxLoadTransferConstraintsExt&&) = delete;
  void operator=(const MaxLoadTransferConstraintsExt&) = delete;
  void operator=(MaxLoadTransferConstraintsExt&&) = delete;

  [[nodiscard]] double maxLoad() const noexcept;

  PROTECTED :

      /// @return validator extensions of a configuration
      [[nodiscard]] std::unique_ptr<const IConfigConstraintValidatorExt>
      _configValidatorExt(std::unique_ptr<const IConfigConstraintValidatorExt>
                              fromNextExt) const noexcept override;

  /// @return true only if cfg satisfies current extension
  /// @throw logic_error for invalid extension
  [[nodiscard]] bool _check(const ent::MovingEntities& cfg) const override;

  /// Max allowed total raft/bridge load
  gsl::not_null<const double*> _maxLoad;
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
 public:
  explicit InitiallyNoPrevRaftLoadExcHandler(
      const std::shared_ptr<const IValues<double>>& allowedLoads);
  ~InitiallyNoPrevRaftLoadExcHandler() noexcept override = default;

  InitiallyNoPrevRaftLoadExcHandler(const InitiallyNoPrevRaftLoadExcHandler&) =
      delete;
  InitiallyNoPrevRaftLoadExcHandler(InitiallyNoPrevRaftLoadExcHandler&&) =
      delete;
  void operator=(const InitiallyNoPrevRaftLoadExcHandler&) = delete;
  void operator=(InitiallyNoPrevRaftLoadExcHandler&&) = delete;

  /**
  Tries to detect if the algorithm is in / has back-tracked to the initial
  state. If it isn't, it returns `indeterminate`. If it is, it returns true, to
  validate any possible raft/bridge load.
  */
  [[nodiscard]] boost::logic::tribool assess(
      const ent::MovingEntities&,
      const SymbolsTable& st) const noexcept override;

  PROTECTED :

      /// The allowed loads
      gsl::not_null<std::shared_ptr<const IValues<double>>>
          _allowedLoads;
  bool dependsOnPreviousRaftLoad;
};

/// Allowed loads validator
class AllowedLoadsValidator : public AbsContextValidator {
 public:
  explicit AllowedLoadsValidator(
      const std::shared_ptr<const IValues<double>>& allowedLoads,
      const std::shared_ptr<const IContextValidator>& nextValidator_ =
          DefContextValidator::SHARED_INST(),
      const std::shared_ptr<const IValidatorExceptionHandler>&
          ownValidatorExcHandler_ = {}) noexcept;
  ~AllowedLoadsValidator() noexcept override = default;

  AllowedLoadsValidator(const AllowedLoadsValidator&) = delete;
  AllowedLoadsValidator(AllowedLoadsValidator&&) = delete;
  void operator=(const AllowedLoadsValidator&) = delete;
  void operator=(AllowedLoadsValidator&&) = delete;

  PROTECTED :

      /// @return true if `ents` is a valid raft/bridge configuration within
      /// `st` context
      /// @throw logic_error if ents misses some extension(s)
      [[nodiscard]] bool
      doValidate(const ent::MovingEntities& ents,
                 const SymbolsTable& st) const override;

  /// The allowed loads
  gsl::not_null<std::shared_ptr<const IValues<double>>> _allowedLoads;
};

}  // namespace cond

namespace sol {

/// A state decorator considering `PreviousRaftLoad` from the Symbols Table
class PrevLoadStateExt : public AbsStateExt {
 public:
  PrevLoadStateExt(unsigned crossingIndex_,
                   double previousRaftLoad_,
                   const ScenarioDetails& info_,
                   const std::shared_ptr<const IStateExt>& nextExt_ =
                       DefStateExt::SHARED_INST()) noexcept;

  PrevLoadStateExt(const SymbolsTable& symbols,
                   const ScenarioDetails& info_,
                   const std::shared_ptr<const IStateExt>& nextExt_ =
                       DefStateExt::SHARED_INST());
  ~PrevLoadStateExt() noexcept override = default;

  PrevLoadStateExt(const PrevLoadStateExt&) = delete;
  PrevLoadStateExt(PrevLoadStateExt&&) = delete;
  void operator=(const PrevLoadStateExt&) = delete;
  void operator=(PrevLoadStateExt&&) = delete;

  [[nodiscard]] double prevRaftLoad() const noexcept;
  [[nodiscard]] unsigned crossingIdx() const noexcept;

  PROTECTED :

      /// Clones the State extension
      [[nodiscard]] std::unique_ptr<const IStateExt>
      _clone(const std::shared_ptr<const IStateExt>& nextExt_)
          const noexcept override;

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
  This extensions add information about the previous raft load.
  */
  [[nodiscard]] std::string _detailsForDemo() const override;

  [[nodiscard]] std::string _toString(
      bool suffixesInsteadOfPrefixes /* = true*/) const override;

  double previousRaftLoad;
  unsigned crossingIndex;
};

}  // namespace sol
}  // namespace rc

#endif  // H_TRANSFERRED_LOAD_EXT
