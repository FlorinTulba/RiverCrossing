/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#include "scenario.h"
#include "rowAbilityExt.h"
#include "durationExt.h"
#include "transferredLoadExt.h"

#include <queue>
#include <iomanip>

using namespace std;

namespace {

/**
Generates all k-combinations of the elements within first .. end.
If the iterators come from `std::unordered_...`,
function `std::prev()` used below raises a signal.

If n = end - first, their count is: n!/(k!*(n-k)!)

@param first the first possible element
@param end the end iterator after the last possible element
@param k the group size
@param results vector of the resulted combinations
@param prefixComb the prefix for a k-combination (initially empty)

@throw logic_error if k is negative or more than end-first
*/
template<class BidirIt, class CombVec>
void generateCombinations(BidirIt first, BidirIt end,
                          ptrdiff_t k,
                          vector<CombVec> &results,
                          const CombVec &prefixComb = {}) {

  static_assert(is_same<
      typename iterator_traits<BidirIt>::value_type,
      typename CombVec::value_type>::value,
    "The iterator and the combinations vectors "
    "must point to values of the same type!");

  const ptrdiff_t n = distance(first, end);
  if(n < k || k < 0LL)
    throw logic_error(string(__func__) +
      " - Provided k must be non-negative and at most end-first!");

  if(k == 0LL) {
    results.push_back(prefixComb);
    return;
  }

  CombVec newPrefixComb(prefixComb);
  BidirIt it=first, itEnd = prev(end, --k);
  while(it != itEnd) {
    newPrefixComb.push_back(*it);
    generateCombinations(++it, end, k, results, newPrefixComb);
    newPrefixComb.pop_back();
  }
}

using namespace rc;
using namespace rc::cond;
using namespace rc::ent;
using namespace rc::sol;

/// Raft/bridge configuration plus the associated validator
class MovingConfigOption {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  const MovingEntities cfg; ///< raft/bridge configuration
  shared_ptr<const IContextValidator> validator; ///< the associated validator

public:
  MovingConfigOption(const MovingEntities &cfg_,
                     const shared_ptr<const IContextValidator> &validator_
                        = DefContextValidator::INST()) :
      cfg(cfg_), validator(CP(validator_)) {}

  /**
  Method for checking which raft/bridge configuration from all possible ones
  is valid for a particular bank configuration and considering also
  the context dictated by the algorithm.
  It doesn't check the validity of the resulted bank configurations.
  It sticks to the validity of this raft/bridge configuration.

  @return can the contained raft/bridge configuration result within
    the SymTb context and from the provided configuration of a bank?
  */
  bool validFor(const BankEntities &bank, const SymbolsTable &SymTb) const {
    const set<unsigned> &raftIds = cfg.ids(), &bankIds = bank.ids();
    const auto bankEnd = cend(bankIds);
    for(unsigned id : raftIds)
      if(bankIds.find(id) == bankEnd) {
#ifndef NDEBUG
        cout<<"Invalid id ["<<id<<"] : ";
        copy(CBOUNDS(raftIds), ostream_iterator<unsigned>(cout, " "));
        cout<<endl;
#endif // NDEBUG
        return false; // cfg should not contain id-s outside bank
      }
    return validator->validate(cfg, SymTb);
  }

  /// @return the contained raft/bridge configuration
  const MovingEntities& get() const {return cfg;}
};

/**
Generates all possible raft/bridge configurations considering all entities
are on the same bank. Adds all necessary context validators.

Determines which raft/bridge configurations can be generated for
a particular bank configuration and within a given context.
It doesn't check the validity of the resulted bank configurations.
*/
class MovingConfigsManager {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  const ScenarioDetails &scenarioDetails; ///< the details of the scenario
  const SymbolsTable &SymTb; ///< the Symbols Table

  /// All possible raft/bridge configurations considering all entities are on the same bank
  vector<MovingConfigOption> allConfigs;

  /**
  Performs the `static` validation of the `cfg` raft/bridge configuration,
  associates it with its necessary `dynamic` validators and appends it
  to the set of all possible configurations.
  */
  void tackleConfig(const vector<unsigned> &cfg,
                    const shared_ptr<const IContextValidator> &validator) {
    const shared_ptr<const AllEntities> &entities = scenarioDetails.entities;
    const unique_ptr<const TransferConstraints> &transferConstraints =
      scenarioDetails.transferConstraints;

    MovingEntities me(entities, cfg, scenarioDetails.createMovingEntitiesExt());

    assert(nullptr != transferConstraints);
    if(transferConstraints->check(me)) {
      allConfigs.emplace_back(me, validator);
#ifndef NDEBUG
      copy(CBOUNDS(cfg), ostream_iterator<unsigned>(cout, " "));
      cout<<endl;
#endif // NDEBUG
    }
  }

public:
  /**
  Generates all possible raft/bridge configurations considering all entities
  are on the same bank. Adds all necessary context validators.
  */
  MovingConfigsManager(const ScenarioDetails &scenarioDetails_,
                       const SymbolsTable &SymTb_) :
      scenarioDetails(scenarioDetails_), SymTb(SymTb_) {
    VP_EX_MSG(scenarioDetails.transferConstraints, logic_error,
              "At this point ScenarioDetails::transferConstraints should be not NULL!");

    const shared_ptr<const AllEntities> &entities = scenarioDetails.entities;
    const size_t entsCount = entities->count();

    // separating entities by row-ability
    const set<unsigned> &allIds = entities->ids();
    unordered_set<unsigned> alwaysRowIds, rowSometimesIds;
    size_t neverRowIdsCount = 0ULL;
    for(unsigned id : allIds) {
      const shared_ptr<const IEntity> ent = (*entities)[id];
      if(ent->canRow())         alwaysRowIds.insert(id);
      else if( ! ent->canRow()) ++neverRowIdsCount;
      else                      rowSometimesIds.insert(id);
    }

    if(neverRowIdsCount == entsCount)
      throw domain_error(string(__func__) +
        " - There are no entities that can or might row!");

    const unsigned capacity = scenarioDetails.capacity;
    if(capacity >= (unsigned)entsCount)
      throw logic_error(string(__func__) +
        " - expecting scenario details with a raft/bridge capacity "
        "less than the number of mentioned entities!");

    shared_ptr<const IContextValidator>
      validatorWithoutCanRow = scenarioDetails_.createTransferValidator(),
      validatorWithCanRow =
        make_shared<const CanRowValidator>(validatorWithoutCanRow);

#ifndef NDEBUG
    cout<<"All possible raft configs: "<<endl;
#endif // NDEBUG

    // Storing allConfigs in increasing order of their capacity
    for(unsigned cap = 1U; cap <= capacity; ++cap) {
      const unsigned restCap = cap - 1U;
      vector<vector<unsigned>> alwaysCanCrossConfigs, sometimesCanCrossConfigs;

      // using here `unordered_set` instead of `set`
      // raises sometimes a signal in `generateCombinations`
      set<unsigned> ids(CBOUNDS(allIds));

      for(unsigned alwaysRowsId : alwaysRowIds) {
        ids.erase(alwaysRowsId);
        if((unsigned)ids.size() >= restCap)
          generateCombinations(CBOUNDS(ids), ptrdiff_t(restCap),
                               alwaysCanCrossConfigs,
                               vector<unsigned>{alwaysRowsId});
      }
      for(const auto &cfg : alwaysCanCrossConfigs)
        tackleConfig(cfg, validatorWithoutCanRow);

      for(unsigned rowsSometimesId : rowSometimesIds) {
        ids.erase(rowsSometimesId);
        if((unsigned)ids.size() >= restCap)
          generateCombinations(CBOUNDS(ids), ptrdiff_t(restCap),
                               sometimesCanCrossConfigs,
                               vector<unsigned>{rowsSometimesId});
      }
      for(const auto &cfg : sometimesCanCrossConfigs)
        tackleConfig(cfg, validatorWithCanRow);
    }

#ifndef NDEBUG
    cout<<endl;
#endif // NDEBUG
  }

  /**
  Determines which raft/bridge configurations can be generated for
  a particular bank configuration and within a given context.
  It doesn't check the validity of the resulted bank configurations.

  The order of the size of the results matters, since
  transports from the left bank should favor more candidates, while
  transports from the right bank should favor less candidates.
  This heuristic should let the algorithm find a solution quicker.

  @param bank the bank configuration from which to select the raft candidates
  @param result the possible raft configurations
  @param largerConfigsFirst how to order the results. Larger to smaller or the other way around
  */
  void configsForBank(const BankEntities &bank,
                      vector<const MovingEntities*> &result,
                      bool largerConfigsFirst) const {
#ifndef NDEBUG
    cout<<endl<<"Invalid raft configs:"<<endl;
#endif // NDEBUG
    result.clear();
    if(largerConfigsFirst) {
      for(auto it = crbegin(allConfigs), itEnd = crend(allConfigs);
          it != itEnd; ++it) {
        const MovingConfigOption &cfgOption = *it;
        if(cfgOption.validFor(bank, SymTb))
          result.push_back(&cfgOption.get());
      }
    } else {
      for(const MovingConfigOption &cfgOption : allConfigs)
        if(cfgOption.validFor(bank, SymTb))
          result.push_back(&cfgOption.get());
    }
#ifndef NDEBUG
    cout<<endl<<"Valid raft configs:"<<endl;
    for(const MovingEntities *me : result)
        cout<<*me<<endl;
    cout<<endl;
#endif // NDEBUG
  }
};

/// A state during solving the scenario
class State : public IState {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  BankEntities _leftBank;
  BankEntities _rightBank;
  bool _nextMoveFromLeft; ///< is the direction of next move from left to right?

  shared_ptr<const IStateExt> extension;

public:
  State(const BankEntities &leftBank, const BankEntities &rightBank,
        bool nextMoveFromLeft,
        const shared_ptr<const IStateExt> &extension_ = DefStateExt::INST()) :
      _leftBank(leftBank), _rightBank(rightBank),
      _nextMoveFromLeft(nextMoveFromLeft), extension(CP(extension_)) {
    if(_leftBank != ~_rightBank)
      throw invalid_argument(string(__func__) +
        " - needs complementary bank configurations!");
  }

  shared_ptr<const IStateExt> getExtension() const override final {
    return extension;
  }

  /// @return true if this state conforms to all constraints that apply to it
  bool valid(const unique_ptr<const ConfigConstraints> &banksConstraints) const override{
    assert(nullptr != extension);
    if( ! extension->validate())
      return false;

    if(banksConstraints) {
      if( ! banksConstraints->check(_leftBank)) {
#ifndef NDEBUG
      cout<<"violates bank constraint ["
        <<*banksConstraints<<"] : "
        <<_leftBank<<endl;
#endif // NDEBUG
        return false;
      }
      if( ! banksConstraints->check(_rightBank)) {
#ifndef NDEBUG
      cout<<"violates bank constraint ["
        <<*banksConstraints<<"] : "
        <<_rightBank<<endl;
#endif // NDEBUG
        return false;
      }
    }

    return true;
  }

  /// @return true if the provided examinedStates do not cover already this state
  bool handledBy(const vector<unique_ptr<const IState>> &examinedStates) const override {
    for(const auto &prevSt : examinedStates)
      if(handledBy(*prevSt)) {
#ifndef NDEBUG
        cout<<"previously considered state"<<endl;
#endif // NDEBUG
        return true;
      }

    return false;
  }

  /// @return true if the `other` state is the same or a better version of this state
  bool handledBy(const IState &other) const override {
    assert(nullptr != extension);
    return
      (extension->isNotBetterThan(other)) &&
      (_nextMoveFromLeft == other.nextMoveFromLeft()) &&
      ((_leftBank.count() <= _rightBank.count()) ? // compare the less crowded bank
        (_leftBank == other.leftBank()) :
        (_rightBank == other.rightBank()));
  }

  const BankEntities& leftBank() const override {return _leftBank;}
  const BankEntities& rightBank() const override {return _rightBank;}
  bool nextMoveFromLeft() const override {return _nextMoveFromLeft;}

  /// @return the next state of the algorithm when moving `movedEnts` to the opposite bank
  unique_ptr<const IState> next(const MovingEntities &movedEnts)
      const override {
    BankEntities left = _leftBank, right = _rightBank;
    if(_nextMoveFromLeft) {
      left -= movedEnts; right += movedEnts;
    } else {
      left += movedEnts; right -= movedEnts;
    }
    assert(nullptr != extension);
    return make_unique<const State>(left, right, ! _nextMoveFromLeft,
                                    extension->extensionForNextState(movedEnts));
  }

  /// Clones this state
  unique_ptr<const IState> clone() const override {
    assert(nullptr != extension);
    return make_unique<const State>(_leftBank, _rightBank, _nextMoveFromLeft,
                                    extension->clone());
  }

  string toString() const override {
    assert(nullptr != extension);
    ostringstream oss;
    { ToStringManager<IStateExt> tsm(*extension, oss); // extension wrapper

      oss<<"Left bank: "<<_leftBank<<" ; "<<"Right bank: "<<_rightBank
        <<" ; Next move direction: ";
      if(_nextMoveFromLeft)
        oss<<" --> ";
      else
        oss<<" <-- ";
    } // ensures tsm's destructor flushes to oss before the return
    return oss.str();
  }
};

/// The moved entities and the resulted state
class Move : public IMove {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  MovingEntities movedEnts; ///< the moved entities
  shared_ptr<const IState> resultedSt; ///< the resulted state
  unsigned idx; ///< 0-based index of the move

public:
  Move(const MovingEntities &movedEnts_,
       unique_ptr<const IState> resultedSt_,
       unsigned idx_) :
      movedEnts(movedEnts_), resultedSt(resultedSt_.release()), idx(idx_) {
    CP(resultedSt);

    const BankEntities &receiverBank = (resultedSt->nextMoveFromLeft()) ?
      resultedSt->leftBank() : resultedSt->rightBank();
    const set<unsigned>
      &movedIds = movedEnts.ids(),
      &receiverBankIds = receiverBank.ids();

    for(unsigned movedId : movedIds)
      if(cend(receiverBankIds) == receiverBankIds.find(movedId))
        throw logic_error(string(__func__) +
          " - Not all moved entities were found on the receiver bank!");
  }
  Move(const IMove &other) : ///< copy-ctor from the base IMove, not from Move
    Move(other.movedEntities(), other.resultedState()->clone(), other.index()) {}
  Move(const Move&) = default;
  Move(Move&&) = default;

  ///< copy assignment operator from the base IMove, not from Move
  Move& operator=(const IMove &other) {
    if(this != &other) {
      movedEnts = other.movedEntities();
      resultedSt = other.resultedState()->clone();
      idx = other.index();
    }
    return *this;
  }
  Move& operator=(const Move&) = default;
  Move& operator=(Move&&) = default;

  const MovingEntities& movedEntities() const override {return movedEnts;}
  shared_ptr<const IState> resultedState() const override {return resultedSt;}
  unsigned index() const override {return idx;}

  string toString() const override {
    ostringstream oss;
    if( ! movedEnts.empty()) {
      assert(idx != UINT_MAX);
      const char dirSign = (resultedSt->nextMoveFromLeft() ? '<' : '>');
      const string dirStr(4ULL, dirSign);
      oss<<endl<<endl<<'\t'
        <<"move "<<setw(3)<<(1U + idx)<<":\t"
        <<dirStr<<' '<<movedEnts<<' '<<dirStr<<endl<<endl;
    }
    oss<<*resultedSt;
    return oss.str();
  }
};

/// A move plus a link to the previous move
class ChainedMove : public Move {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  shared_ptr<const ChainedMove> prev; ///< the link to the previous move or NULL if first move

public:
  ChainedMove(const MovingEntities &movedEnts_,
              unique_ptr<const IState> resultedSt_,
              unsigned idx_,
              const shared_ptr<const ChainedMove> &prevMove = nullptr) :
    Move(movedEnts_, std::move(resultedSt_), idx_), prev(prevMove) {}
  ChainedMove(const ChainedMove&) = default;
  ChainedMove(ChainedMove&&) = default;
  ChainedMove& operator=(const ChainedMove&) = default;
  ChainedMove& operator=(ChainedMove&&) = default;

  shared_ptr<const ChainedMove> prevMove() const {return prev;}
};

/**
The current states from the path of the search algorithm.
If the algorithm finds a solution, the attempt will express
the path required to reach that solution.
*/
class Attempt : public IAttempt {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:

  const IMove& move(size_t idx) const override {return moves.at(idx);}

    #else // keep fields protected otherwise
protected:
    #endif

  /// Initial fake empty move setting the initial state
  unique_ptr<const Move> initFakeMove;

  /// The configuration of the left bank for a solution
  unique_ptr<const BankEntities> targetLeftBank;

  vector<Move> moves; ///< the moves to be extended by the algorithm

public:
  Attempt() {} ///< used by Depth-First search

  /// Builds the chronological Breadth-First solution based on the last move
  Attempt(const ChainedMove &chainedMove) {
    // Calling below Attempt instead of _Attempt would just create temporary
    // objects and won't create the desired effects of the recursion.

    // Function that must be called only within this ctor, so kept as a lambda:
    function<void(const ChainedMove&)>
      _Attempt = [&_Attempt, // cannot refer to itself below without capturing its own name
                  this](const ChainedMove &chMove) {
        if(nullptr != chMove.prevMove())
          _Attempt(*chMove.prevMove()); // this required the mentioned capture
        append(chMove);
      };
    _Attempt(chainedMove);
  }

  /// First call sets the initial state. Next calls are the actually moves.
  void append(const IMove &move) override {
    if(nullptr == initFakeMove) {
      if( ! move.movedEntities().empty())
        throw logic_error(string(__func__) +
          " should be called the first time with a `move` parameter "
          "using an empty `movedEntities`!");
      initFakeMove = make_unique<const Move>(move);
      targetLeftBank =
        make_unique<const BankEntities>(initFakeMove->resultedState()->rightBank());
      return;
    }
    if(move.index() != (unsigned)moves.size())
      throw logic_error(string(__func__) +
        " - Expecting move index "s + to_string(moves.size()) +
        ", but the provided move has index "s + to_string(move.index()));
    moves.emplace_back(move);
  }

  /// Removes last move, if any left
  void pop() override {
    if(moves.empty())
      return;
    moves.pop_back();
  }

  /// Ensures the attempt won't show corrupt data after a difficult to trace exception
  void clear() override {
    moves.clear();
  }

  shared_ptr<const IState> initialState() const override {
    if(nullptr != initFakeMove)
      return initFakeMove->resultedState();

    return nullptr;
  }

  size_t length() const override {return moves.size();} ///< number of moves from the current path

  /**
  @return last performed move or at least the initial fake empty move
     that set the initial state
  @throw out_of_range when there is not even the fake initial one
  */
  const IMove& lastMove() const override {
    if( ! moves.empty())
      return moves.back();

    if(nullptr != initFakeMove)
      return *initFakeMove;

    throw out_of_range(string(__func__) +
      " - Called when there are no moves yet and not even the initial state!");
  }

  bool isSolution() const override { ///< @return true for a solution path
    // Timing, bank and raft/bridge (capacity & load) constraints all conform here
    // Now it matters only if everyone reached the opposite bank
    return ( ! moves.empty())
      && (*targetLeftBank == moves.back().resultedState()->leftBank());
  }

  string toString() const override {
    if(nullptr == initFakeMove)
      return "";
    ostringstream oss;
    oss<<*initFakeMove->resultedState();
    for(const Move &m : moves)
      oss<<m;
    return oss.str();
  }
};

/// Performs the required backtracking
class Solver {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  const ScenarioDetails &scenarioDetails; ///< the details of the scenario
  Scenario::Results &results; ///< the results for the scenario

  SymbolsTable SymTb; ///< the Symbols Table

  /// Provides the possible raft configurations for a new move
  MovingConfigsManager movingCfgsManager;

  /// Ensures the algorithm doesn't retry a path twice
  vector<unique_ptr<const IState>> examinedStates;
  shared_ptr<IAttempt> steps; ///< the current evolution of the algorithm

  unique_ptr<const BankEntities> targetLeftBank;

  /**
  Absolute difference between the expected entities on the target left bank
  and the entities from each entry of solver.results.closestToTargetLeftBank.
  The value is the size of the symmetric difference between the 2 bank configurations.
  */
  size_t minDistToGoal = SIZE_MAX;

  /**
  This should be a newer / better state than the examined ones.
  However, previous states that are inferior to this one should be removed.
  Since this purge is executed for each new addition, there can be only 1
  dominated state for each call - all previous states
  were independent and dominant and now at most one of them
  can become inferior to the provided state.
  */
  void addExaminedState(unique_ptr<const IState> &&s) {
    auto it = begin(examinedStates), itEnd = end(examinedStates);
    while((it != itEnd) && ( ! (*it)->handledBy(*s))) ++it;

    if(it == itEnd)
      examinedStates.push_back(std::move(s));
    else
      *it = std::move(s);
  }

  /// Updates the statistics to report and Symbols Table if necessary
  void commonTasksAddMove(const Move &move) {
    // wraps around for UINT_MAX
    SymTb["CrossingIndex"] = double(move.index() + 2U);

    assert(nullptr != move.movedEntities().getExtension());
    move.movedEntities().getExtension()->addMovePostProcessing(SymTb);
    results.update(size_t(move.index() + 1U), // wraps around for UINT_MAX
                   targetLeftBank->differencesCount(move.resultedState()->leftBank()),
                   move.resultedState()->leftBank(), minDistToGoal);
  }

  /**
  Provides the raft/bridge configurations which are allowed in a given state
  sorted by the number of entities are on each raft/bridge config.

  When moving from left to right, larger configs are favored, while at return,
  smaller configs are preferred.
  */
  void allowedMovingConfigurations(const IState &s,
                                   vector<const MovingEntities*> &allowedCfgs) {
    const BankEntities &crtBank = s.nextMoveFromLeft() ?
      s.leftBank() : s.rightBank();
    movingCfgsManager.configsForBank(crtBank, allowedCfgs, s.nextMoveFromLeft());
  }

  /**
  Prepares the exploration of a move.
  Cleans up when returning from dead ends.

  Updates SymTb, examinedStates and steps accordingly.
  */
  class StepManager {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

    Solver &solver; ///< parent outer class
    bool committed = false; ///< was the move(step) ok or it should be reverted?
#ifndef NDEBUG
    const Move _move;
#endif // NDEBUG

  public:
    StepManager(Solver &solver_, const Move &move) : solver(solver_)
#ifndef NDEBUG
        , _move(move)
#endif // NDEBUG
        {
#ifndef NDEBUG
      cout<<endl<<endl;

      const unsigned moveIdx = move.index();
      if(UINT_MAX != moveIdx) { // a normal move
        assert( ! move.movedEntities().empty());
        assert((unsigned)solver.steps->length() == moveIdx);
        assert(solver.steps->initialState() != nullptr);

        cout<<*solver.steps->lastMove().resultedState()<<endl;
        cout<<"  DO move "<<(moveIdx + 1U)<<" : "<<move<<endl;

      } else { // about to set the initial state through a fake empty move
        assert(move.movedEntities().empty());
        assert(solver.steps->length() == 0ULL);
        assert(solver.steps->initialState() == nullptr);

        cout<<"  DO initial fake empty move : "<<move<<endl;
      }
#endif // NDEBUG

      solver.steps->append(move);
      if(solver.steps->isSolution()) {
        commitStep();
        return; // no need to update the rest of the information now
      }

      solver.commonTasksAddMove(move);
      solver.addExaminedState(move.resultedState()->clone());
    }

    /// Reverts the dead-end move(step)
    ~StepManager() {
      if(committedStep())
        return;

      // Dead end => backtracking
      solver.steps->pop();

      --solver.SymTb["CrossingIndex"];

      // Allowing the actions of the extensions
      const IMovingEntitiesExt *previousMoveExt =
        solver.steps->lastMove().movedEntities().getExtension();
      assert(nullptr != previousMoveExt);
      previousMoveExt->removeMovePostProcessing(solver.SymTb);

#ifndef NDEBUG
      // _move.index() might return UINT_MAX, while CrossingIndex is reliable
      cout<<endl<<endl<<"UNDO move "<<solver.SymTb["CrossingIndex"]<<" : "
        <<_move<<endl;
#endif // NDEBUG
    }

    void commitStep() {committed = true;} ///< marks the move as ok
    bool committedStep() const {return committed;}
  };

  friend class StepManager;

  /// @return true if a solution was found using BFS
  bool bfsExplore(unique_ptr<const IState> initialState) {
    addExaminedState(initialState->clone());

    queue<shared_ptr<const ChainedMove>> movesToExplore;

    // The initial entry is the fake move producing initial state
    movesToExplore.push(make_shared<const ChainedMove>(
      MovingEntities(scenarioDetails.entities, {},
                     scenarioDetails.createMovingEntitiesExt()),
      std::move(initialState),
      UINT_MAX)); // UINT_MAX index required for the fake initial move

    assert(nullptr == initialState); // moved to movesToExplore[0]

    do {
      const shared_ptr<const ChainedMove> move = movesToExplore.front();
      movesToExplore.pop();

#ifndef NDEBUG
      cout<<endl<<"Discovering successors of move:"<<endl<<*move<<endl;
#endif // NDEBUG

      commonTasksAddMove(*move);

      const shared_ptr<const IState> crtState = move->resultedState();
      vector<const MovingEntities*> allowedMovingConfigs;
      allowedMovingConfigurations(*crtState, allowedMovingConfigs);

      for(const MovingEntities *movingCfg : allowedMovingConfigs) {
        assert(movingCfg);
        unique_ptr<const IState> nextState = crtState->next(*movingCfg);

#ifndef NDEBUG
        cout<<endl<<"Probing move "<<*movingCfg<<" => "<<*nextState<<endl;
#endif // NDEBUG

        if( ! nextState->valid(scenarioDetails.banksConstraints)
            || nextState->handledBy(examinedStates))
          continue; // check next raft/bridge config

        const shared_ptr<const ChainedMove> validNextMove
          = make_shared<const ChainedMove>(
            MovingEntities(scenarioDetails.entities, movingCfg->ids(),
                           movingCfg->getExtension()->clone()),
            std::move(nextState),
            1U + move->index(), // wraps around for UINT_MAX
            move);

        assert(nullptr == nextState); // moved to validNextMove

        // Checking if the new state is a solution.
        // Timing, bank and raft/bridge (capacity & load) constraints all conform here
        // Now it matters only if everyone reached the opposite bank
        if(validNextMove->resultedState()->leftBank() == *targetLeftBank) {
          // Found an optimal solution
          steps = make_shared<Attempt>(*validNextMove);
          return true;
        }

        movesToExplore.push(validNextMove);
        addExaminedState(validNextMove->resultedState()->clone());
      }
    } while( ! movesToExplore.empty());

    return false;
  }

  /// @return true if a solution was found using DFS
  bool dfsExplore(Move &&move) {
    StepManager stepManager(*this, move);
    if(stepManager.committedStep())
      return true; // discovered solution and committed the given final move

    const shared_ptr<const IState> crtState = move.resultedState();
    vector<const MovingEntities*> allowedMovingConfigs;
    allowedMovingConfigurations(*crtState, allowedMovingConfigs);

    for(const MovingEntities *movingCfg : allowedMovingConfigs) {
      assert(movingCfg);
      unique_ptr<const IState> nextState = crtState->next(*movingCfg);

#ifndef NDEBUG
      cout<<endl<<"Simulating move "<<*movingCfg<<" => "<<*nextState<<endl;
#endif // NDEBUG

      if( ! nextState->valid(scenarioDetails.banksConstraints)
          || nextState->handledBy(examinedStates))
        continue; // check next raft/bridge config

      if(dfsExplore({*movingCfg, std::move(nextState), (unsigned)steps->length()})) {
        stepManager.commitStep();
        return true;
      }
    }

    return false;
  }
  /// Pretends the initial state is the result of a previous move (empty raft/bridge)
  bool dfsExplore(unique_ptr<const IState> initialState) {
    steps = make_shared<Attempt>();
    return dfsExplore({
      MovingEntities(scenarioDetails.entities, {},
                     scenarioDetails.createMovingEntitiesExt()),
      move(initialState),
      UINT_MAX}); // UINT_MAX index required for the fake initial move
  }

public:
  Solver(const ScenarioDetails &scenarioDetails_,
         Scenario::Results &results_) :
      scenarioDetails(scenarioDetails_),
      results(results_),
      SymTb(InitialSymbolsTable()),
      movingCfgsManager(scenarioDetails_, SymTb) {}

  /// Looks for a solution either through BFS or through DFS
  void run(bool usingBFS) {
#ifndef NDEBUG
    cout<<"Exploring:"<<endl;
#endif // NDEBUG

    try {
      unique_ptr<const IState> initSt
        = scenarioDetails.createInitialState(SymTb);
      targetLeftBank = make_unique<const BankEntities>(initSt->rightBank());

      if(usingBFS)
        bfsExplore(std::move(initSt));
      else
        dfsExplore(std::move(initSt));
    } catch(const exception &e) {
      cerr<<"Couldn't solve the scenario due to: "<<e.what()<<endl;
      if(nullptr != steps)
        steps->clear();
    }

#ifndef NDEBUG
    cout<<"Finished exploring."<<endl<<endl;

    results.investigatedStates = examinedStates.size();
    for(size_t i=0ULL, lim = results.investigatedStates-1ULL; i<lim; ++i) {
      const auto &oneState = examinedStates[i];
      for(size_t j=i+1ULL; j<=lim; ++j) {
        const auto &otherState = examinedStates[j];
        if(otherState->handledBy(*oneState) || oneState->handledBy(*otherState)) {
          cout<<"Found duplicate/redundancy among the examined states:"<<endl;
          cout<<*oneState<<endl;
          cout<<*otherState<<endl;
          assert(false);
        }
      }
    }
#endif // NDEBUG

    if(nullptr != steps)
      results.attempt = steps;
    else
      results.attempt = make_shared<const Attempt>();
  }
};

} // anonymous namespace

namespace rc {

namespace sol {

const shared_ptr<const DefStateExt>& DefStateExt::INST() {
  static const shared_ptr<const DefStateExt> inst(new DefStateExt);
  return inst;
}

shared_ptr<const IStateExt> DefStateExt::clone() const {
  return INST();
}

shared_ptr<const IStateExt>
    DefStateExt::extensionForNextState(const ent::MovingEntities&) const {
  return DefStateExt::INST();
}

AbsStateExt::AbsStateExt(const ScenarioDetails &info_,
            const shared_ptr<const IStateExt> &nextExt_) :
    info(info_), nextExt(CP(nextExt_)) {}

shared_ptr<const IStateExt> AbsStateExt::clone() const {
  assert(nullptr != nextExt);
  return _clone(nextExt->clone());
}

bool AbsStateExt::validate() const {
  assert(nullptr != nextExt);
  return nextExt->validate() && _validate();
}

bool AbsStateExt::isNotBetterThan(const IState &s2) const {
  assert(nullptr != nextExt);
  return nextExt->isNotBetterThan(s2) && _isNotBetterThan(s2);
}

shared_ptr<const IStateExt>
    AbsStateExt::extensionForNextState(const ent::MovingEntities &me) const {
  assert(nullptr != nextExt);
  const shared_ptr<const IStateExt> fromNextExt =
    nextExt->extensionForNextState(me);
  return _extensionForNextState(me, fromNextExt);
}

string AbsStateExt::toString(bool suffixesInsteadOfPrefixes/* = true*/) const {
  assert(nullptr != nextExt);
  // Only the matching extension categories will return non-empty strings
  // given suffixesInsteadOfPrefixes
  // (some display only as prefixes, the rest only as suffixes)
  return _toString(suffixesInsteadOfPrefixes) +
    nextExt->toString(suffixesInsteadOfPrefixes);
}

} // namespace sol

const SymbolsTable& InitialSymbolsTable(){
  static const SymbolsTable st{{"CrossingIndex", 0.}};
  return st;
}

unique_ptr<const IState>
      ScenarioDetails::createInitialState(const SymbolsTable &SymTb) const {
  shared_ptr<const IStateExt> stateExt = DefStateExt::INST();

  if(nullptr != allowedLoads &&
          allowedLoads->dependsOnVariable("PreviousRaftLoad"))
    stateExt = make_shared<const PrevLoadStateExt>(SymTb, *this, stateExt);

  if(maxDuration != UINT_MAX)
    stateExt = make_shared<const TimeStateExt>(0U, *this, stateExt);

  return make_unique<const State>(
            BankEntities(entities, entities->idsStartingFromLeftBank()),
            BankEntities(entities, entities->idsStartingFromRightBank()),
            true, // always start from left bank
            stateExt);
}

const Scenario::Results& Scenario::solution(bool usingBFS/* = true*/) {
	if(usingBFS) {
    if( ! investigatedByBFS) {
      Solver solver(details, resultsBFS);
      solver.run(usingBFS);

      investigatedByBFS = true;
    }

    return resultsBFS;
	}

	if( ! investigatedByDFS) {
    Solver solver(details, resultsDFS);
    solver.run(usingBFS);

		investigatedByDFS = true;
	}

	return resultsDFS;
}

} // namespace rc

namespace std {

ostream& operator<<(ostream &os, const rc::sol::IState &st) {
  os<<st.toString();
  return os;
}

ostream& operator<<(ostream &os, const rc::sol::IMove &m) {
  os<<m.toString();
  return os;
}

ostream& operator<<(ostream &os, const rc::sol::IAttempt &attempt) {
  os<<attempt.toString();
  return os;
}

} // namespace std

#ifdef UNIT_TESTING

/*
  This include allows recompiling only the Unit tests project when updating the tests.
  It also keeps the count of total code units to recompile to a minimum value.
*/
#define SOLVER_CPP
#include "../test/solver.hpp"

#endif // UNIT_TESTING
