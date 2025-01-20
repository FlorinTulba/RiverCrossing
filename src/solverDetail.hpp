/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef HPP_SOLVER_DETAIL
#define HPP_SOLVER_DETAIL

#include "rowAbilityExt.h"
#include "scenario.h"
#include "util.h"

#include <cstddef>

#include <concepts>
#include <iterator>
#include <queue>
#include <tuple>
#include <utility>

using std::ignore;

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
template <std::bidirectional_iterator BidirIt, typename CombVec>
  requires requires(CombVec v) {
    typename CombVec::value_type;
    requires std::same_as<typename std::iterator_traits<BidirIt>::value_type,
                          typename CombVec::value_type>;
    { v.push_back(std::declval<typename CombVec::value_type>()) };
    { v.pop_back() };
  }
void generateCombinations(const BidirIt first,
                          const BidirIt end,
                          ptrdiff_t k,
                          std::vector<CombVec>& results,
                          const CombVec& prefixComb = {}) {
  using namespace std;

  const ptrdiff_t n{distance(first, end)};
  if (n < k || k < 0LL)
    throw logic_error{
        HERE.function_name() +
        " - Provided k must be non-negative and at most end-first!"s};

  if (!k) {
    results.push_back(prefixComb);
    return;
  }

  CombVec newPrefixComb{prefixComb};
  BidirIt it{first};
  const BidirIt itEnd{prev(end, --k)};
  while (it != itEnd) {
    newPrefixComb.push_back(*it);
    generateCombinations(++it, end, k, results, newPrefixComb);
    newPrefixComb.pop_back();
  }
}

/// Raft/bridge configuration plus the associated validator
class MovingConfigOption {
 public:
  explicit MovingConfigOption(
      const rc::ent::MovingEntities& cfg_,
      const std::shared_ptr<const rc::cond::IContextValidator>& validator_ =
          rc::cond::DefContextValidator::SHARED_INST()) noexcept
      : cfg{cfg_}, validator{validator_} {}
  MovingConfigOption(const MovingConfigOption&) noexcept = default;
  ~MovingConfigOption() noexcept = default;

  void operator=(const MovingConfigOption&) = delete;
  void operator=(MovingConfigOption&&) = delete;

  /**
  Method for checking which raft/bridge configuration from all possible ones
  is valid for a particular bank configuration and considering also
  the context dictated by the algorithm.
  It doesn't check the validity of the resulted bank configurations.
  It sticks to the validity of this raft/bridge configuration.

  @return can the contained raft/bridge configuration result within
    the SymTb context and from the provided configuration of a bank?
  */
  [[nodiscard]] bool validFor(const rc::ent::BankEntities& bank,
                              const rc::SymbolsTable& SymTb) const {
    using namespace std;

    const set<unsigned>&raftIds{cfg.ids()}, &bankIds{bank.ids()};
    for (const unsigned id : raftIds)
      if (!bankIds.contains(id)) {
#ifndef NDEBUG
        cout << "Invalid id [" << id << "] : ";
        ranges::copy(raftIds, ostream_iterator<unsigned>{cout, " "});
        cout << endl;
#endif                 // NDEBUG
        return false;  // cfg should not contain id-s outside bank
      }
    return validator->validate(cfg, SymTb);
  }

  /// @return the contained raft/bridge configuration
  [[nodiscard]] const rc::ent::MovingEntities& get() const noexcept {
    return cfg;
  }

  PROTECTED :

      /// Raft/bridge configuration
      const rc::ent::MovingEntities cfg;

  /// The associated validator
  gsl::not_null<std::shared_ptr<const rc::cond::IContextValidator>> validator;
};

/**
Generates all possible raft/bridge configurations considering all entities
are on the same bank. Adds all necessary context validators.

Determines which raft/bridge configurations can be generated for
a particular bank configuration and within a given context.
It doesn't check the validity of the resulted bank configurations.
*/
class MovingConfigsManager {
 public:
  /**
  Generates all possible raft/bridge configurations considering all entities
  are on the same bank. Adds all necessary context validators.
  */
  MovingConfigsManager(const rc::ScenarioDetails& scenarioDetails_,
                       const rc::SymbolsTable& SymTb_)
      : scenarioDetails{&scenarioDetails_}, SymTb{&SymTb_} {
    using namespace std;
    using namespace rc::ent;
    using namespace rc::cond;

    if (!scenarioDetails->transferConstraints) [[unlikely]]
      throw logic_error{
          HERE.function_name() +
          " - At this point ScenarioDetails::transferConstraints should be "
          "not NULL!"s};

    const shared_ptr<const AllEntities>& entities{scenarioDetails->entities};
    const size_t entsCount{entities->count()};

    // separating entities by row-ability
    const set<unsigned>& allIds{entities->ids()};
    unordered_set<unsigned> alwaysRowIds, rowSometimesIds;
    size_t neverRowIdsCount{};
    for (const unsigned id : allIds) {
      const shared_ptr<const IEntity> ent{(*entities)[id]};
      if (ent->canRow())
        alwaysRowIds.insert(id);
      else if (!ent->canRow())
        ++neverRowIdsCount;
      else
        rowSometimesIds.insert(id);
    }

    if (neverRowIdsCount == entsCount)
      throw domain_error{HERE.function_name() +
                         " - There are no entities that can or might row!"s};

    const unsigned capacity{scenarioDetails->capacity};
    if (capacity >= (unsigned)entsCount)
      throw logic_error{
          HERE.function_name() +
          " - expecting scenario details with a raft/bridge capacity "
          "less than the number of mentioned entities!"s};

    shared_ptr<const IContextValidator> validatorWithoutCanRow{
        scenarioDetails_.createTransferValidator()},
        validatorWithCanRow{
            make_shared<const CanRowValidator>(validatorWithoutCanRow)};

#ifndef NDEBUG
    cout << "All possible raft configs: \n";
#endif  // NDEBUG

    // Storing allConfigs in increasing order of their capacity
    for (unsigned cap{1U}; cap <= capacity; ++cap) {
      const unsigned restCap{cap - 1U};
      vector<vector<unsigned>> alwaysCanCrossConfigs, sometimesCanCrossConfigs;

      // using here `unordered_set` instead of `set`
      // raises sometimes a signal in `generateCombinations`
      set<unsigned> ids(CBOUNDS(allIds));

      for (const unsigned alwaysRowsId : alwaysRowIds) {
        ids.erase(alwaysRowsId);
        if ((unsigned)size(ids) >= restCap)
          generateCombinations(CBOUNDS(ids), ptrdiff_t(restCap),
                               alwaysCanCrossConfigs, {alwaysRowsId});
      }
      for (const auto& cfg : alwaysCanCrossConfigs)
        tackleConfig(cfg, validatorWithoutCanRow);

      for (const unsigned rowsSometimesId : rowSometimesIds) {
        ids.erase(rowsSometimesId);
        if ((unsigned)size(ids) >= restCap)
          generateCombinations(CBOUNDS(ids), ptrdiff_t(restCap),
                               sometimesCanCrossConfigs, {rowsSometimesId});
      }
      for (const auto& cfg : sometimesCanCrossConfigs)
        tackleConfig(cfg, validatorWithCanRow);
    }

#ifndef NDEBUG
    cout << endl;
#endif  // NDEBUG
  }
  MovingConfigsManager(const MovingConfigsManager&) noexcept = default;
  ~MovingConfigsManager() noexcept = default;

  void operator=(const MovingConfigsManager&) = delete;
  void operator=(MovingConfigsManager&&) = delete;

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
  @param largerConfigsFirst how to order the results. Larger to smaller or the
  other way around
  */
  void configsForBank(const rc::ent::BankEntities& bank,
                      std::vector<const rc::ent::MovingEntities*>& result,
                      bool largerConfigsFirst) const {
    using namespace std;

#ifndef NDEBUG
    cout << "\nInvalid raft configs:\n";
#endif  // NDEBUG
    result.clear();
    if (largerConfigsFirst) {
      const auto itEnd = crend(allConfigs);
      for (auto it = crbegin(allConfigs); it != itEnd; ++it) {
        const MovingConfigOption& cfgOption{*it};
        if (cfgOption.validFor(bank, *SymTb))
          result.push_back(&cfgOption.get());
      }
    } else {
      for (const MovingConfigOption& cfgOption : allConfigs)
        if (cfgOption.validFor(bank, *SymTb))
          result.push_back(&cfgOption.get());
    }
#ifndef NDEBUG
    cout << "\nValid raft configs:\n";
    for (const rc::ent::MovingEntities* me : result)
      cout << *me << '\n';
    cout << endl;
#endif  // NDEBUG
  }

  PROTECTED :

      /**
      Performs the `static` validation of the `cfg` raft/bridge configuration,
      associates it with its necessary `dynamic` validators and appends it
      to the set of all possible configurations.
      */
      void
      tackleConfig(
          const std::vector<unsigned>& cfg,
          const std::shared_ptr<const rc::cond::IContextValidator>& validator) {
    using namespace std;
    using namespace rc::ent;

    const shared_ptr<const AllEntities>& entities{scenarioDetails->entities};
    MovingEntities me{entities, cfg,
                      scenarioDetails->createMovingEntitiesExt()};

    assert(scenarioDetails->transferConstraints);
    if (scenarioDetails->transferConstraints->check(me)) {
      allConfigs.emplace_back(me, validator);
#ifndef NDEBUG
      ranges::copy(cfg, ostream_iterator<unsigned>{cout, " "});
      cout << endl;
#endif  // NDEBUG
    }
  }

  /// The details of the scenario
  gsl::not_null<const rc::ScenarioDetails*> scenarioDetails;

  gsl::not_null<const rc::SymbolsTable*> SymTb;  ///< the Symbols Table

  /// All possible raft/bridge configurations considering all entities are on
  /// the same bank
  std::vector<MovingConfigOption> allConfigs;
};

/// A state during solving the scenario
class State : public rc::sol::IState {
 public:
  State(const rc::ent::BankEntities& leftBank,
        const rc::ent::BankEntities& rightBank,
        bool nextMoveFromLeft,
        const std::shared_ptr<const rc::sol::IStateExt>& extension_ =
            rc::sol::DefStateExt::SHARED_INST())
      : _leftBank{leftBank},
        _rightBank{rightBank},
        extension{extension_},
        _nextMoveFromLeft{nextMoveFromLeft} {
    if (_leftBank != ~_rightBank)
      throw std::invalid_argument{
          HERE.function_name() +
          " - needs complementary bank configurations!"s};
  }
  State(const State&) = default;
  State(State&&) noexcept = default;
  ~State() noexcept override = default;

  void operator=(const State&) = delete;
  void operator=(State&&) = delete;

  [[nodiscard]] gsl::not_null<std::shared_ptr<const rc::sol::IStateExt>>
  getExtension() const noexcept final {
    return extension;
  }

  /// @return true if this state conforms to all constraints that apply to it
  [[nodiscard]] bool valid(
      const rc::cond::ConfigConstraints* banksConstraints) const override {
    if (!extension->validate())
      return false;

    if (banksConstraints) {
      if (!banksConstraints->check(_leftBank)) {
#ifndef NDEBUG
        std::cout << "violates bank constraint [" << *banksConstraints
                  << "] : " << _leftBank << std::endl;
#endif  // NDEBUG
        return false;
      }
      if (!banksConstraints->check(_rightBank)) {
#ifndef NDEBUG
        std::cout << "violates bank constraint [" << *banksConstraints
                  << "] : " << _rightBank << std::endl;
#endif  // NDEBUG
        return false;
      }
    }

    return true;
  }

  /// @return true if the provided examinedStates do not cover already this
  /// state
  [[nodiscard]] bool handledBy(
      const std::vector<std::unique_ptr<const rc::sol::IState>>& examinedStates)
      const override {
    for (const auto& prevSt : examinedStates)
      if (handledBy(*prevSt)) {
#ifndef NDEBUG
        std::cout << "previously considered state" << std::endl;
#endif  // NDEBUG
        return true;
      }

    return false;
  }

  /// @return true if the `other` state is the same or a better version of this
  /// state
  [[nodiscard]] bool handledBy(const rc::sol::IState& other) const override {
    return (extension->isNotBetterThan(other)) &&
           (_nextMoveFromLeft == other.nextMoveFromLeft()) &&
           ((_leftBank.count() <= _rightBank.count())
                ?  // compare the less crowded bank
                (_leftBank == other.leftBank())
                : (_rightBank == other.rightBank()));
  }

  [[nodiscard]] const rc::ent::BankEntities& leftBank()
      const noexcept override {
    return _leftBank;
  }

  [[nodiscard]] const rc::ent::BankEntities& rightBank()
      const noexcept override {
    return _rightBank;
  }

  [[nodiscard]] bool nextMoveFromLeft() const noexcept override {
    return _nextMoveFromLeft;
  }

  /// @return the next state of the algorithm when moving `movedEnts` to the
  /// opposite bank
  [[nodiscard]] std::unique_ptr<const rc::sol::IState> next(
      const rc::ent::MovingEntities& movedEnts) const override {
    rc::ent::BankEntities left{_leftBank}, right{_rightBank};
    if (_nextMoveFromLeft) {
      left -= movedEnts;
      right += movedEnts;
    } else {
      left += movedEnts;
      right -= movedEnts;
    }
    return std::make_unique<const State>(
        left, right, !_nextMoveFromLeft,
        extension->extensionForNextState(movedEnts));
  }

  /// Clones this state
  std::unique_ptr<const rc::sol::IState> clone() const noexcept override {
    return std::make_unique<const State>(_leftBank, _rightBank,
                                         _nextMoveFromLeft, extension->clone());
  }

  std::string toString(bool showNextMoveDir /* = true*/) const override {
    using namespace std;

    ostringstream oss;
    {
      // extension wrapper
      rc::ToStringManager<rc::sol::IStateExt> tsm{*extension, oss};

      oss << "Left bank: " << _leftBank << " ; "
          << "Right bank: " << _rightBank;
      if (showNextMoveDir) {
        oss << " ; Next move direction: ";
        if (_nextMoveFromLeft)
          oss << " --> ";
        else
          oss << " <-- ";
      }
    }  // ensures tsm's destructor flushes to oss before the return
    return oss.str();
  }

  PROTECTED :

      rc::ent::BankEntities _leftBank;
  rc::ent::BankEntities _rightBank;

  gsl::not_null<std::shared_ptr<const rc::sol::IStateExt>> extension;

  /// Is the direction of next move from left to right?
  bool _nextMoveFromLeft;
};

/// The moved entities and the resulted state
class Move : public rc::sol::IMove {
 public:
  /*
  resultedSt_ is passed by value, thus triggers
  WARN_MSVC_BRACED_INIT_LIST_ORDER when braced-initialized.
  */
  Move(const rc::ent::MovingEntities& movedEnts_,
       std::unique_ptr<const rc::sol::IState> resultedSt_,
       unsigned idx_)
      : movedEnts{movedEnts_}, resultedSt{std::move(resultedSt_)}, idx{idx_} {
    using namespace std;

    const rc::ent::BankEntities& receiverBank{(resultedSt->nextMoveFromLeft())
                                                  ? resultedSt->leftBank()
                                                  : resultedSt->rightBank()};
    const set<unsigned>&movedIds{movedEnts.ids()},
        &receiverBankIds{receiverBank.ids()};

    for (const unsigned movedId : movedIds)
      if (!receiverBankIds.contains(movedId))
        throw logic_error{
            HERE.function_name() +
            " - Not all moved entities were found on the receiver bank!"s};
  }

  Move(const rc::sol::IMove& other)
      :  ///< copy-ctor from the base IMove, not from Move
        Move(other.movedEntities(),
             other.resultedState()->clone(),
             other.index()) {}

  Move(const Move&) = default;
  Move(Move&&) noexcept = default;

  ///< copy assignment operator from the base IMove, not from Move
  Move& operator=(const rc::sol::IMove& other) noexcept {
    if (this != &other) {
      movedEnts = other.movedEntities();
      resultedSt = other.resultedState()->clone();
      idx = other.index();
    }
    return *this;
  }
  Move& operator=(const Move&) = default;
  Move& operator=(Move&&) noexcept = default;
  ~Move() noexcept = default;

  [[nodiscard]] const rc::ent::MovingEntities& movedEntities()
      const noexcept override {
    return movedEnts;
  }

  [[nodiscard]] std::shared_ptr<const rc::sol::IState> resultedState()
      const noexcept override {
    return resultedSt;
  }

  [[nodiscard]] unsigned index() const noexcept override { return idx; }

  [[nodiscard]] std::string toString(
      bool showNextMoveDir /* = true*/) const override {
    using namespace std;

    ostringstream oss;
    if (!movedEnts.empty()) {
      assert(idx != UINT_MAX);
      const char dirSign{(resultedSt->nextMoveFromLeft() ? '<' : '>')};
      const string dirStr(4ULL, dirSign);
      oss << "\n\n\t\ttransfer " << setw(3) << (1U + idx) << ":\t" << dirStr
          << ' ' << movedEnts << ' ' << dirStr << "\n\n";
    }
    oss << resultedSt->toString(showNextMoveDir);
    return oss.str();
  }

  PROTECTED :

      /// The moved entities
      rc::ent::MovingEntities movedEnts;

  /// The resulted state
  gsl::not_null<std::shared_ptr<const rc::sol::IState>> resultedSt;

  unsigned idx;  ///< 0-based index of the move
};

/// A move plus a link to the previous move
class ChainedMove : public Move {
 public:
  ChainedMove(const rc::ent::MovingEntities& movedEnts_,
              std::unique_ptr<const rc::sol::IState> resultedSt_,
              unsigned idx_,
              const std::shared_ptr<const ChainedMove>& prevMove = {})
      : Move(movedEnts_, std::move(resultedSt_), idx_), prev{prevMove} {}
  ChainedMove(const ChainedMove&) = default;
  ChainedMove(ChainedMove&&) noexcept = default;
  ChainedMove& operator=(const ChainedMove&) = default;
  ChainedMove& operator=(ChainedMove&&) noexcept = default;
  ~ChainedMove() noexcept override = default;

  [[nodiscard]] std::shared_ptr<const ChainedMove> prevMove() const noexcept {
    return prev;
  }

  PROTECTED :

      /// The link to the previous move or NULL if first move
      std::shared_ptr<const ChainedMove>
          prev;
};

/**
The current states from the path of the search algorithm.
If the algorithm finds a solution, the attempt will express
the path required to reach that solution.
*/
class Attempt : public rc::sol::IAttempt {
 public:
  Attempt() noexcept = default;  ///< used by Depth-First search

  /// Builds the chronological Breadth-First solution based on the last move
  explicit Attempt(const ChainedMove& chainedMove) noexcept {
    // Calling below Attempt instead of _Attempt would just create temporary
    // objects and won't create the desired effects of the recursion.

    // Function that must be called only within this ctor, so kept as a lambda:
    std::function<void(const ChainedMove&)> _Attempt{
        // cannot refer to itself below without capturing its own name
        [&_Attempt, this](const ChainedMove& chMove) noexcept {
          if (chMove.prevMove())
            // this required the mentioned capture
            _Attempt(*chMove.prevMove());
          append(chMove);
        }};
    _Attempt(chainedMove);
  }
  ~Attempt() noexcept override = default;

  Attempt(const Attempt&) = delete;
  Attempt(Attempt&&) = delete;
  void operator=(const Attempt&) = delete;
  void operator=(Attempt&&) = delete;

  /// First call sets the initial state. Next calls are the actually moves.
  void append(const rc::sol::IMove& move) override {
    using namespace std;

    if (!initFakeMove) {
      if (!move.movedEntities().empty())
        throw logic_error{
            HERE.function_name() +
            " should be called the first time with a `move` parameter "
            "using an empty `movedEntities`!"s};
      initFakeMove = make_unique<const Move>(move);
      targetLeftBank = make_unique<const rc::ent::BankEntities>(
          initFakeMove->resultedState()->rightBank());
      return;
    }
    if (move.index() != (unsigned)size(moves))
      throw logic_error{HERE.function_name() + " - Expecting move index "s +
                        to_string(size(moves)) +
                        ", but the provided move has index "s +
                        to_string(move.index())};
    moves.push_back(move);
  }

  /// Removes last move, if any left
  void pop() noexcept override {
    if (moves.empty())
      return;
    moves.pop_back();
  }

  /// Ensures the attempt won't show corrupt data after a difficult to trace
  /// exception
  void clear() noexcept override { moves.clear(); }

  [[nodiscard]] std::shared_ptr<const rc::sol::IState> initialState()
      const noexcept override {
    if (initFakeMove)
      return initFakeMove->resultedState();

    return {};
  }

  [[nodiscard]] size_t length() const noexcept override {
    return size(moves);
  }  ///< number of moves from the current path

  [[nodiscard]] const rc::sol::IMove& move(size_t idx) const override {
    return moves.at(idx);
  }

  /**
  @return last performed move or at least the initial fake empty move
     that set the initial state
  @throw out_of_range when there is not even the fake initial one
  */
  [[nodiscard]] const rc::sol::IMove& lastMove() const override {
    if (!moves.empty())
      return moves.back();

    if (initFakeMove)
      return *initFakeMove;

    throw std::out_of_range{HERE.function_name() +
                            " - Called when there are no moves yet and not "
                            "even the initial state!"s};
  }

  /// @return true for a solution path
  [[nodiscard]] bool isSolution() const noexcept override {
    // Timing, bank and raft/bridge (capacity & load) constraints all conform
    // here.
    // Now it matters only if everyone reached the opposite bank
    return (!moves.empty()) &&
           (*targetLeftBank == moves.back().resultedState()->leftBank());
  }

  [[nodiscard]] std::string toString() const override {
    if (!initFakeMove)
      return {};
    std::ostringstream oss;
    oss << initFakeMove->resultedState()->toString(false);
    if (isSolution()) {
      for (const Move& m : moves)
        oss << m.toString(false);
    }
    return oss.str();
  }

  PROTECTED :

      /// Initial fake empty move setting the initial state
      std::unique_ptr<const Move>
          initFakeMove;

  /// The configuration of the left bank for a solution
  std::unique_ptr<const rc::ent::BankEntities> targetLeftBank;

  std::vector<Move> moves;  ///< the moves to be extended by the algorithm
};

/// Performs the required backtracking
class Solver {
 public:
  Solver(const rc::ScenarioDetails& scenarioDetails_,
         rc::Scenario::Results& results_)
      : scenarioDetails{&scenarioDetails_},
        results{&results_},
        SymTb{rc::InitialSymbolsTable()},
        movingCfgsManager{scenarioDetails_, SymTb} {}
  ~Solver() noexcept = default;

  Solver(const Solver&) = delete;
  Solver(Solver&&) = delete;
  void operator=(const Solver&) = delete;
  void operator=(Solver&&) = delete;

  /// Looks for a solution either through BFS or through DFS
  void run(bool usingBFS) {
    using namespace std;

#ifndef NDEBUG
    cout << "Exploring:\n";
#endif  // NDEBUG

    try {
      unique_ptr<const rc::sol::IState> initSt{
          scenarioDetails->createInitialState(SymTb)};
      targetLeftBank =
          make_unique<const rc::ent::BankEntities>(initSt->rightBank());

      if (usingBFS)
        ignore = bfsExplore(std::move(initSt));
      else
        ignore = dfsExplore(std::move(initSt));
    } catch (const exception& e) {
      cerr << "Couldn't solve the scenario due to: " << e.what() << endl;
      if (steps)
        steps->clear();
    }

#ifndef NDEBUG
    cout << "Finished exploring.\n" << endl;

    /*
    Testing duplicate/redundancy among the examined states.
    This part wasn't moved to Unit tests on purpose, to capture such problems
    even in dynamic, more complex scenarios which weren't reproduced in Unit
    tests.
    */
    assert(results->investigatedStates > 0ULL);
    const auto lim{size(examinedStates) - 1ULL};
    for (auto i{0ULL}; i < lim; ++i) {
      const auto& oneState = examinedStates[i];
      for (auto j{i + 1ULL}; j <= lim; ++j) {
        const auto& otherState = examinedStates[j];
        if (otherState->handledBy(*oneState) ||
            oneState->handledBy(*otherState)) {
          cout << "Found duplicate/redundancy among the examined states:\n"
               << *oneState << '\n'
               << *otherState << endl;
          assert(false);
        }
      }
    }
#endif  // NDEBUG

    if (steps)
      results->attempt = steps;
    else
      results->attempt = make_shared<const Attempt>();
  }

  PROTECTED :

      /**
      This should be a newer / better state than the examined ones.
      However, previous states that are inferior to this one should be removed.
      Since this purge is executed for each new addition, there can be only 1
      dominated state for each call - all previous states
      were independent and dominant and now at most one of them
      can become inferior to the provided state.
      */
      void
      addExaminedState(std::unique_ptr<const rc::sol::IState> s) noexcept {
    ++results->investigatedStates;  // needs to be counted in any case

    auto it = begin(examinedStates);
    const auto itEnd = end(examinedStates);
    while ((it != itEnd) && (!(*it)->handledBy(*s)))
      ++it;

    if (it == itEnd)
      examinedStates.push_back(std::move(s));
    else
      *it = std::move(s);
  }

  /// Updates the statistics to report and Symbols Table if necessary
  void commonTasksAddMove(const Move& move) noexcept {
    // wraps around for UINT_MAX
    SymTb["CrossingIndex"] = double(move.index() + 2U);

    move.movedEntities().getExtension()->addMovePostProcessing(SymTb);
    results->update(
        size_t(move.index() + 1U),  // wraps around for UINT_MAX
        targetLeftBank->differencesCount(move.resultedState()->leftBank()),
        move.resultedState()->leftBank(), minDistToGoal);
  }

  /**
  Provides the raft/bridge configurations which are allowed in a given state
  sorted by the number of entities are on each raft/bridge config.

  When moving from left to right, larger configs are favored, while at return,
  smaller configs are preferred.
  */
  void allowedMovingConfigurations(
      const rc::sol::IState& s,
      std::vector<const rc::ent::MovingEntities*>& allowedCfgs) {
    const rc::ent::BankEntities& crtBank{s.nextMoveFromLeft() ? s.leftBank()
                                                              : s.rightBank()};
    movingCfgsManager.configsForBank(crtBank, allowedCfgs,
                                     s.nextMoveFromLeft());
  }

  /**
  Prepares the exploration of a move.
  Cleans up when returning from dead ends.

  Updates SymTb, examinedStates and steps accordingly.
  */
  class StepManager {
   public:
    StepManager(Solver& solver_, const Move& move)
        : solver{&solver_}
#ifndef NDEBUG
          ,
          _move{move}
#endif  // NDEBUG
    {
#ifndef NDEBUG
      using namespace std;

      cout << "\n\n";

      const unsigned moveIdx{move.index()};
      if (UINT_MAX != moveIdx) {  // a normal move
        assert(!move.movedEntities().empty());
        assert((unsigned)solver->steps->length() == moveIdx);
        assert(solver->steps->initialState());

        cout << *solver->steps->lastMove().resultedState() << "\n  DO move "
             << (moveIdx + 1U) << " : " << move << endl;

      } else {  // about to set the initial state through a fake empty move
        assert(move.movedEntities().empty());
        assert(!solver->steps->length());
        assert(!solver->steps->initialState());

        cout << "  DO initial fake empty move : " << move << endl;
      }
#endif  // NDEBUG

      solver->steps->append(move);
      if (solver->steps->isSolution()) {
        commitStep();
        return;  // no need to update the rest of the information now
      }

      solver->commonTasksAddMove(move);
      solver->addExaminedState(move.resultedState()->clone());
    }

    /// Reverts the dead-end move(step)
    ~StepManager() noexcept {
      if (committedStep())
        return;

      // Dead end => backtracking
      solver->steps->pop();

      --solver->SymTb["CrossingIndex"];

      // Allowing the actions of the extensions
      const gsl::not_null<const rc::ent::IMovingEntitiesExt*> previousMoveExt{
          solver->steps->lastMove().movedEntities().getExtension()};
      previousMoveExt->removeMovePostProcessing(solver->SymTb);

#ifndef NDEBUG
      using namespace std;

      // _move.index() might return UINT_MAX, while CrossingIndex is reliable
      cout << "\n\nUNDO move " << solver->SymTb["CrossingIndex"] << " : "
           << _move << endl;
#endif  // NDEBUG
    }

    StepManager(const StepManager&) = delete;
    StepManager(StepManager&&) = delete;
    void operator=(const StepManager&) = delete;
    void operator=(StepManager&&) = delete;

    void commitStep() noexcept { committed = true; }  ///< marks the move as ok
    [[nodiscard]] bool committedStep() const noexcept { return committed; }

    PROTECTED :

        /// Parent outer class
        gsl::not_null<Solver*>
            solver;

#ifndef NDEBUG
    Move _move;
#endif  // NDEBUG

    /// Was the move(step) ok or it should be reverted?
    bool committed{};
  };

  friend class StepManager;

  /// @return true if a solution was found using BFS
  [[nodiscard]] bool bfsExplore(
      std::unique_ptr<const rc::sol::IState> initialState) {
    using namespace std;
    using namespace rc::ent;
    using namespace rc::sol;

    addExaminedState(initialState->clone());

    queue<shared_ptr<const ChainedMove>> movesToExplore;

    // The initial entry is the fake move producing initial state
    movesToExplore.push(make_shared<const ChainedMove>(
        MovingEntities(scenarioDetails->entities, {},
                       scenarioDetails->createMovingEntitiesExt()),
        std::move(initialState),
        UINT_MAX));  // UINT_MAX index required for the fake initial move

    assert(!initialState);  // moved to movesToExplore[0]

    do {
      const shared_ptr<const ChainedMove> move{movesToExplore.front()};
      movesToExplore.pop();

#ifndef NDEBUG
      cout << "\nDiscovering successors of move:\n" << *move << endl;
#endif  // NDEBUG

      commonTasksAddMove(*move);

      const shared_ptr<const IState> crtState{move->resultedState()};
      vector<const MovingEntities*> allowedMovingConfigs;
      allowedMovingConfigurations(*crtState, allowedMovingConfigs);

      for (const MovingEntities* movingCfg : allowedMovingConfigs) {
        assert(movingCfg);
        unique_ptr<const IState> nextState{crtState->next(*movingCfg)};

#ifndef NDEBUG
        cout << "\nProbing move " << *movingCfg << " => " << *nextState << endl;
#endif  // NDEBUG

        if (!nextState->valid(scenarioDetails->banksConstraints.get()) ||
            nextState->handledBy(examinedStates))
          continue;  // check next raft/bridge config

        const shared_ptr<const ChainedMove> validNextMove{
            make_shared<const ChainedMove>(
                MovingEntities(scenarioDetails->entities, movingCfg->ids(),
                               movingCfg->getExtension()->clone()),
                std::move(nextState),
                1U + move->index(),  // wraps around for UINT_MAX
                move)};

        assert(!nextState);  // moved to validNextMove

        // Checking if the new state is a solution.
        // Timing, bank and raft/bridge (capacity & load) constraints all
        // conform here.
        // Now it matters only if everyone reached the opposite bank
        if (validNextMove->resultedState()->leftBank() == *targetLeftBank) {
          // Found an optimal solution
          steps = make_shared<Attempt>(*validNextMove);
          return true;
        }

        movesToExplore.push(validNextMove);
        addExaminedState(validNextMove->resultedState()->clone());
      }
    } while (!movesToExplore.empty());

    return false;
  }

  /// @return true if a solution was found using DFS
  [[nodiscard]] bool dfsExplore(const Move& move) {
    using namespace std;
    using namespace rc::ent;
    using namespace rc::sol;

    StepManager stepManager{*this, move};
    if (stepManager.committedStep())
      return true;  // discovered solution and committed the given final move

    const shared_ptr<const IState> crtState{move.resultedState()};
    vector<const MovingEntities*> allowedMovingConfigs;
    allowedMovingConfigurations(*crtState, allowedMovingConfigs);

    for (const MovingEntities* movingCfg : allowedMovingConfigs) {
      assert(movingCfg);
      unique_ptr<const IState> nextState{crtState->next(*movingCfg)};

#ifndef NDEBUG
      cout << "\nSimulating move " << *movingCfg << " => " << *nextState
           << endl;
#endif  // NDEBUG

      if (!nextState->valid(scenarioDetails->banksConstraints.get()) ||
          nextState->handledBy(examinedStates))
        continue;  // check next raft/bridge config

      if (dfsExplore(Move(*movingCfg, std::move(nextState),
                          (unsigned)steps->length()))) {
        stepManager.commitStep();
        return true;
      }
    }

    return false;
  }

  /// Pretends the initial state is the result of a previous move (empty
  /// raft/bridge)
  [[nodiscard]] bool dfsExplore(
      std::unique_ptr<const rc::sol::IState> initialState) {
    steps = std::make_shared<Attempt>();

    return dfsExplore(Move(
        rc::ent::MovingEntities{scenarioDetails->entities,
                                {},
                                scenarioDetails->createMovingEntitiesExt()},
        std::move(initialState),
        UINT_MAX));  // UINT_MAX index required for the fake initial move
  }

  /// The details of the scenario
  gsl::not_null<const rc::ScenarioDetails*> scenarioDetails;

  /// The results for the scenario
  gsl::not_null<rc::Scenario::Results*> results;

  rc::SymbolsTable SymTb;  ///< the Symbols Table

  /// Provides the possible raft configurations for a new move
  MovingConfigsManager movingCfgsManager;

  /// Ensures the algorithm doesn't retry a path twice
  std::vector<std::unique_ptr<const rc::sol::IState>> examinedStates;

  /// The current evolution of the algorithm
  std::shared_ptr<rc::sol::IAttempt> steps;

  std::unique_ptr<const rc::ent::BankEntities> targetLeftBank;

  /**
  Absolute difference between the expected entities on the target left bank
  and the entities from each entry of solver.results.closestToTargetLeftBank.
  The value is the size of the symmetric difference between the 2 bank
  configurations.
  */
  size_t minDistToGoal{SIZE_MAX};
};

}  // anonymous namespace

#endif  // !HPP_SOLVER_DETAIL
