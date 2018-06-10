/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#include "scenario.h"
#include "mathRelated.h"

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
using namespace rc::ent;
using namespace rc::sol;

/// Fall-back context validator - accepts any raft/bridge configuration
struct DefContextValidator final : IContextValidator {
  /// Allows sharing the default instance
  static const shared_ptr<const DefContextValidator>& INST() {
    static const shared_ptr<const DefContextValidator>
      inst(new DefContextValidator);
    return inst;
  }

  bool validate(const MovingEntities&, const SymbolsTable&) const override {
    return true;
  }

    #ifndef UNIT_TESTING // leave ctor public only for Unit tests
protected:
    #endif

  DefContextValidator() {}
};

/**
Calling `IContextValidator::validate()` might throw.
Some valid contexts (expressed mainly by the Symbols Table)
allow this to happen.
In those cases there should actually be a validation result.

This interface allows handling those exceptions so that
whenever they occur, the validation will provide a result instead of throwing.

The rest of the exceptions will still propagate.

When an exception is caught, an instance of a derived class assesses the context.
*/
struct IValidatorExceptionHandler /*abstract*/ {
  virtual ~IValidatorExceptionHandler()/* = 0*/ {}

  /**
  Assesses the context of the exception.
  If it doesn't match the exempted cases, returns `indeterminate`.
  If it matches the exempted cases generates a boolean validation result
  */
  virtual boost::logic::tribool assess(const MovingEntities &ents,
                                       const SymbolsTable &st) const = 0;
};

/// Abstract base class for the context validator decorators.
class AbsContextValidator /*abstract*/ : public IContextValidator {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  // using shared_ptr as more configurations might use these fields
  shared_ptr<const IContextValidator> nextValidator; ///< a chained next validator
  shared_ptr<const IValidatorExceptionHandler> ownValidatorExcHandler; ///< possible handler for particular contexts

  AbsContextValidator(
    const shared_ptr<const IContextValidator> &nextValidator_
      = DefContextValidator::INST(),
    const shared_ptr<const IValidatorExceptionHandler> &ownValidatorExcHandler_
      = nullptr) :
      nextValidator(VP(nextValidator_)),
      ownValidatorExcHandler(ownValidatorExcHandler_) {}

  /// @return true if `ents` is a valid raft/bridge configuration within `st` context
  virtual bool doValidate(const MovingEntities &ents,
                          const SymbolsTable &st) const = 0;

public:
  /**
  Performs local validation and then delegates to the next validator.
  The local validation might throw and the optional handler might
  stop the exception propagation and generate a validation result instead.

  @return true if `ents` is a valid raft/bridge configuration within `st` context
  */
  bool validate(const MovingEntities &ents,
                const SymbolsTable &st) const override final {
    bool resultOwnValidator = false;
    try {
      resultOwnValidator = doValidate(ents, st);
    } catch(const exception&) {
      if(nullptr != ownValidatorExcHandler) {
        const boost::logic::tribool excAssessment =
          ownValidatorExcHandler->assess(ents, st);

        if(boost::logic::indeterminate(excAssessment))
          throw; // not an exempted case

        resultOwnValidator = excAssessment;

      } else throw; // no saving exception handler
    }

    if( ! resultOwnValidator)
      return false;

    assert(nextValidator);
    return nextValidator->validate(ents, st);
  }
};

/// Can row validator
class CanRowValidator : public AbsContextValidator {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  /// @return true if `ents` is a valid raft/bridge configuration within `st` context
  bool doValidate(const MovingEntities &ents,
                  const SymbolsTable &st) const override {
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

public:
  CanRowValidator(
    const shared_ptr<const IContextValidator> &nextValidator_
      = DefContextValidator::INST(),
    const shared_ptr<const IValidatorExceptionHandler> &ownValidatorExcHandler_
      = nullptr) :
    AbsContextValidator(nextValidator_, ownValidatorExcHandler_) {}
};

using namespace rc::cond;

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

  shared_ptr<const IValues<double>> _allowedLoads; ///< the allowed loads
  bool dependsOnPreviousRaftLoad;

public:
  InitiallyNoPrevRaftLoadExcHandler(
        const shared_ptr<const IValues<double>> &allowedLoads) :
      _allowedLoads(VP(allowedLoads)) {
    if(_allowedLoads->empty())
      throw invalid_argument(string(__func__) +
        " - doesn't accept empty allowedLoads parameter! "
        "Some loads must be allowed!");

    dependsOnPreviousRaftLoad =
        _allowedLoads->dependsOnVariable("PreviousRaftLoad");
  }

  /**
  Tries to detect if the algorithm is in / has back-tracked to the initial state.
  If it isn't, it returns `indeterminate`.
  If it is, it returns true, to validate any possible raft/bridge load.
  */
  boost::logic::tribool assess(const MovingEntities&,
                               const SymbolsTable &st) const override {
    // Ensure first that the allowed loads depend on `PreviousRaftLoad`
    if( ! dependsOnPreviousRaftLoad)
      return boost::logic::indeterminate;

    const auto stEnd = cend(st),
      itCrossingIndex = st.find("CrossingIndex");
    const bool
      isInitialState =
        (st.find("PreviousRaftLoad") == stEnd)      // missing PreviousRaftLoad
        && (itCrossingIndex != stEnd)               // existing CrossingIndex
        && (itCrossingIndex->second <= 1. + Eps);   // CrossingIndex <= 1
    if(isInitialState)
      return true;

    return boost::logic::indeterminate;
  }
};

/// Allowed loads validator
class AllowedLoadsValidator : public AbsContextValidator {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  shared_ptr<const IValues<double>> _allowedLoads; ///< the allowed loads

  /// @return true if `ents` is a valid raft/bridge configuration within `st` context
  bool doValidate(const MovingEntities &ents,
                  const SymbolsTable &st) const override {
    const bool valid = _allowedLoads->contains(ents.weight(), st);
#ifndef NDEBUG
    if( ! valid) {
      cout<<"Invalid load ["<<ents.weight()<<" outside "<<*_allowedLoads<<"] : ";
      copy(CBOUNDS(ents.ids()), ostream_iterator<unsigned>(cout, " "));
      cout<<endl;
    }
#endif // NDEBUG
    return valid;
  }

public:
  AllowedLoadsValidator(
    const shared_ptr<const IValues<double>> &allowedLoads,
    const shared_ptr<const IContextValidator> &nextValidator_
      = DefContextValidator::INST(),
    const shared_ptr<const IValidatorExceptionHandler> &ownValidatorExcHandler_
      = nullptr) :
      AbsContextValidator(nextValidator_, ownValidatorExcHandler_),
      _allowedLoads(VP(allowedLoads)) {}
};

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
      cfg(cfg_), validator(VP(validator_)) {}

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

  const Scenario::Details &scenarioDetails; ///< the details of the scenario
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
      scenarioDetails._transferConstraints;

    MovingEntities me(entities, cfg);

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
  MovingConfigsManager(const Scenario::Details &scenarioDetails_,
                       const SymbolsTable &SymTb_) :
      scenarioDetails(scenarioDetails_), SymTb(SymTb_) {
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

    const unsigned capacity = scenarioDetails._capacity;
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

using namespace rc::sol;

/// Default State extension, which does nothing
struct DefStateExt final : IStateExt {
  /// Allows sharing the default instance
  static const shared_ptr<const DefStateExt>& INST() {
    static const shared_ptr<const DefStateExt> inst(new DefStateExt);
    return inst;
  }

  /// Clones the State extension
  shared_ptr<const IStateExt> clone() const override final {
    return INST();
  }

  /// Validates the parameter state based on the constraints of the extension
  bool validate() const override final {return true;}

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  bool isNotBetterThan(const IState&) const override final {
    return true;
  }

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  shared_ptr<const IStateExt>
      extensionForNextState(const MovingEntities&) const override final {
    return DefStateExt::INST();
  }

  string toString(bool suffixesInsteadOfPrefixes/* = true*/) const override final {return "";}

    #ifndef UNIT_TESTING // leave ctor public only for Unit tests
protected:
    #endif

  DefStateExt() {}
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
      _nextMoveFromLeft(nextMoveFromLeft), extension(VP(extension_)) {
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
    { IStateExt::ToStringManager tsm(*extension, oss); // extension wrapper

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

/**
Base class for state extensions decorators.
Some of the new virtual methods are abstract and must be implemented
by every derived class.
*/
class AbsStateExt /*abstract*/ : public IStateExt {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  const Scenario::Details &info;
  shared_ptr<const IStateExt> nextExt;

  AbsStateExt(const Scenario::Details &info_,
              const shared_ptr<const IStateExt> &nextExt_) :
      info(info_), nextExt(VP(nextExt_)) {}

  /// Clones the State extension
  virtual shared_ptr<const IStateExt>
    _clone(const shared_ptr<const IStateExt> &nextExt_) const = 0;

  /// Validates the parameter state based on the constraints of the extension
  virtual bool _validate() const {return true;}

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  virtual bool _isNotBetterThan(const IState&) const {
    return true;
  }

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  virtual shared_ptr<const IStateExt>
      _extensionForNextState(const MovingEntities&,
                             const shared_ptr<const IStateExt> &fromNextExt)
                      const = 0;

  virtual string _toString(bool suffixesInsteadOfPrefixes = true) const {
    return "";
  }

  template<class ExtType,
          typename = enable_if_t<is_convertible<ExtType*, AbsStateExt*>::value>>
  static shared_ptr<const ExtType>
      selectExt(const shared_ptr<const IStateExt> &ext) {
    shared_ptr<const ExtType> resExt = dynamic_pointer_cast<const ExtType>(ext);
    if(nullptr != resExt)
      return resExt;

    shared_ptr<const AbsStateExt> hostExt =
      dynamic_pointer_cast<const AbsStateExt>(ext);
    while(nullptr != hostExt) {
      resExt = dynamic_pointer_cast<const ExtType>(hostExt->nextExt);
      if(nullptr != resExt)
        return resExt;

      hostExt = dynamic_pointer_cast<const AbsStateExt>(hostExt->nextExt);
    }

    return nullptr;
  }

public:
  /// Clones the State extension
  shared_ptr<const IStateExt> clone() const override final {
    assert(nullptr != nextExt);
    return _clone(nextExt->clone());
  }

  /// Validates the parameter state based on the constraints of the extension
  bool validate() const override final {
    assert(nullptr != nextExt);
    return nextExt->validate() && _validate();
  }

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  bool isNotBetterThan(const IState &s2) const override final {
    assert(nullptr != nextExt);
    return nextExt->isNotBetterThan(s2) && _isNotBetterThan(s2);
  }

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  shared_ptr<const IStateExt>
      extensionForNextState(const MovingEntities &me) const override final {
    assert(nullptr != nextExt);
    const shared_ptr<const IStateExt> fromNextExt =
      nextExt->extensionForNextState(me);
    return _extensionForNextState(me, fromNextExt);
  }

  string toString(bool suffixesInsteadOfPrefixes/* = true*/) const override final {
    assert(nullptr != nextExt);
    // Only the matching extension categories will return non-empty strings
    // given suffixesInsteadOfPrefixes
    // (some display only as prefixes, the rest only as suffixes)
    return _toString(suffixesInsteadOfPrefixes) +
      nextExt->toString(suffixesInsteadOfPrefixes);
  }
};

/// Allows State to contain a time entry - the moment the state is reached
class TimeStateExt : public AbsStateExt {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  unsigned _time; ///< the moment this state is reached

  shared_ptr<const IStateExt>
      _clone(const shared_ptr<const IStateExt> &nextExt_) const override {
    return make_shared<const TimeStateExt>(_time, info, nextExt_);
  }

  /// Validates the parameter state based on the constraints of the extension
  bool _validate() const override {
    if(_time > info._maxDuration) {
#ifndef NDEBUG
      cout<<"violates duration constraint ["
        <<_time<<" > "<<info._maxDuration<<']'<<endl;
#endif // NDEBUG
      return false;
    }
    return true;
  }

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  bool _isNotBetterThan(const IState &s2) const override {
    const shared_ptr<const IStateExt> extensions2 = s2.getExtension();
    assert(nullptr != extensions2);

    shared_ptr<const TimeStateExt> timeExt2 =
      AbsStateExt::selectExt<TimeStateExt>(extensions2);

    // if the other state was reached earlier, it is better
    return _time >=
      VP_EX_MSG(timeExt2,
                logic_error,
                "The parameter must be a state "
                "with a TimeStateExt extension!")->_time;
  }

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  shared_ptr<const IStateExt>
      _extensionForNextState(const MovingEntities &movedEnts,
                             const shared_ptr<const IStateExt> &fromNextExt) const override {
    unsigned timeOfNextState = _time;
    bool foundMatch = false;
    for(const ConfigurationsTransferDuration &ctdItem : info.ctdItems) {
      const TransferConstraints& config = ctdItem.configConstraints();
      if( ! config.check(movedEnts))
        continue;

      foundMatch = true;
      timeOfNextState += ctdItem.duration();
      break;
    }

    if( ! foundMatch)
      throw domain_error(string(__func__) +
        " - Provided CrossingDurationsOfConfigurations items don't cover "
        "raft configuration: "s + movedEnts.toString());

    return make_shared<const TimeStateExt>(timeOfNextState, info, fromNextExt);
  }

  string _toString(bool suffixesInsteadOfPrefixes/* = true*/) const override {
    // This is displayed only as prefix information
    if(suffixesInsteadOfPrefixes)
      return "";

    ostringstream oss;
    oss<<"[Time "<<setw(4)<<_time<<"] ";
    return oss.str();
  }

public:
  TimeStateExt(unsigned time_, const Scenario::Details &info_,
               const shared_ptr<const IStateExt> &nextExt_ = DefStateExt::INST()) :
      AbsStateExt(info_, nextExt_), _time(time_) {}

  unsigned time() const {return _time;}
};

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
  shared_ptr<const IStateExt>
      _clone(const shared_ptr<const IStateExt> &nextExt_) const override {
    return make_shared<const PrevLoadStateExt>(crossingIndex, previousRaftLoad,
                                               info, nextExt_);
  }

  /**
  @return true if the state which is extended is not better than provided state
    based on the constraints of the extension
  */
  bool _isNotBetterThan(const IState &s2) const override {
    const shared_ptr<const IStateExt> extensions2 = s2.getExtension();
    assert(nullptr != extensions2);

    shared_ptr<const PrevLoadStateExt> prevLoadStateExt2 =
      AbsStateExt::selectExt<PrevLoadStateExt>(extensions2);

    const double otherPreviousRaftLoad =
      VP_EX_MSG(prevLoadStateExt2,
                logic_error,
                "The parameter must be a state "
                "with a PrevLoadStateExt extension!")->previousRaftLoad;

    /*
    PreviousRaftLoad on NaN means the initial state.
    Whenever the algorithm reaches back to initial state
    (by advancing, not backtracking), it should backtrack,
    as the length of the solution would be longer (when continuing in this manner)
    than the length of the solution starting fresh with the move about to consider
    next from this revisited initial state.
    */
    if(isNaN(otherPreviousRaftLoad))
      return true; // deciding to disallow revisiting the initial state

    return abs(previousRaftLoad - otherPreviousRaftLoad) < Eps;
  }

  /**
  @return the extension to be used by the next state,
    based on current extension and the parameters
  */
  shared_ptr<const IStateExt>
      _extensionForNextState(const MovingEntities &movedEnts,
                             const shared_ptr<const IStateExt> &fromNextExt)
                      const override {
    return make_shared<const PrevLoadStateExt>(crossingIndex + 1U,
                                               movedEnts.weight(),
                                               info, fromNextExt);
  }

  string _toString(bool suffixesInsteadOfPrefixes/* = true*/) const override {
    // This is displayed only as suffix information
    if( ! suffixesInsteadOfPrefixes)
      return "";

    ostringstream oss;
    oss<<" ; PrevRaftLoad: "<<previousRaftLoad;
    return oss.str();
  }

public:
  PrevLoadStateExt(unsigned crossingIndex_, double previousRaftLoad_,
                   const Scenario::Details &info_,
                   const shared_ptr<const IStateExt> &nextExt_
                      = DefStateExt::INST()) :
      AbsStateExt(info_, nextExt_),
      crossingIndex(crossingIndex_), previousRaftLoad(previousRaftLoad_) {}

  PrevLoadStateExt(const SymbolsTable &symbols,
                   const Scenario::Details &info_,
                   const shared_ptr<const IStateExt> &nextExt_
                      = DefStateExt::INST()) :
      AbsStateExt(info_, nextExt_) {
    // PreviousRaftLoad should miss from Symbols Table when CrossingIndex <= 1
    const auto stEnd = cend(symbols);
    const auto itCrossingIndex = symbols.find("CrossingIndex"),
      itPreviousRaftLoad = symbols.find("PreviousRaftLoad");
    if(itCrossingIndex == stEnd)
      throw logic_error(string(__func__) +
        " - needs to get `symbols` table containing an entry for CrossingIndex!");

    crossingIndex = (unsigned)floor(.5 + itCrossingIndex->second); // rounded value
    if(itPreviousRaftLoad == stEnd) {
      if(crossingIndex >= 2U)
        throw logic_error(string(__func__) +
          " - needs to get `symbols` table containing an entry for PreviousRaftLoad "
          "when the CrossingIndex entry is >= 2 !");

      previousRaftLoad = numeric_limits<double>::quiet_NaN();
    } else previousRaftLoad = itPreviousRaftLoad->second;
  }

  double prevRaftLoad() const {return previousRaftLoad;}
  unsigned crossingIdx() const {return crossingIndex;}
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
    VP(resultedSt);

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

/**
The current states from the path of the backtracking algorithm.
If the algorithm finds a solution, the attempt will express
the path required to reach that solution.
*/
class Attempt : public IAttempt {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  shared_ptr<const IState> initSt; ///< initial state

  /// The configuration of the left bank for a solution
  unique_ptr<const BankEntities> targetLeftBank;

  vector<Move> moves; ///< the moves to be extended by the algorithm

public:
  /// First call sets the initial state. Next calls are the actually moves.
  void append(const Move &move) {
    if(nullptr == initSt) {
      if( ! move.movedEntities().empty())
        throw logic_error(string(__func__) +
          " should be called the first time with a `move` parameter "
          "using an empty `movedEntities`!");
      initSt = move.resultedState();
      targetLeftBank = make_unique<const BankEntities>(~initSt->leftBank());
      return;
    }
    if(move.index() != (unsigned)moves.size())
      throw logic_error(string(__func__) +
        " - Expecting move index "s + to_string(moves.size()) +
        ", but the provided move has index "s + to_string(move.index()));
    moves.push_back(move);
  }

  /// Removes last move, if any left, or even the initial state when called again
  void pop() {
    if(moves.empty()) {
      initSt = nullptr;
      targetLeftBank = nullptr;
      return;
    }
    moves.pop_back();
  }

  /// Ensures the attempt won't show corrupt data after a difficult to trace exception
  void clear() {
    moves.clear();
    initSt = nullptr;
    targetLeftBank = nullptr;
  }

  shared_ptr<const IState> initialState() const {return initSt;}
  size_t length() const override {return moves.size();} ///< number of moves from the current path
  const IMove& move(size_t idx) const override { ///< n-th move
    return moves.at(idx);
  }

  /**
  @return size of the symmetric difference between the entities from
    the target left bank and current left bank
  */
  size_t distToSolution() const override {
    if(nullptr == initSt)
      return SIZE_MAX;

    const BankEntities &crtLeftBank = moves.empty() ?
      initSt->leftBank() : moves[length() - 1ULL].resultedState()->leftBank();
    return initSt->rightBank().differencesCount(crtLeftBank);
  }

  bool isSolution() const override { ///< @return true for a solution path
    return (nullptr != initSt) && ( ! moves.empty())
      && (*targetLeftBank == moves.back().resultedState()->leftBank());
  }

  string toString() const override {
    if(nullptr == initSt)
      return "";
    ostringstream oss;
    oss<<*initSt;
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

  const Scenario::Details &scenarioDetails; ///< the details of the scenario
  Scenario::Results &results; ///< the results for the scenario

  SymbolsTable SymTb; ///< the Symbols Table

  /// Provides the possible raft configurations for a new move
  MovingConfigsManager movingCfgsManager;

  /// Ensures the algorithm doesn't retry a path twice
  vector<unique_ptr<const IState>> examinedStates;
  shared_ptr<Attempt> steps; ///< the current evolution of the algorithm

  /**
  Absolute difference between the expected entities on the target left bank
  and the entities from each entry of solver.results.closestToTargetLeftBank.
  The value is the size of the symmetric difference between the 2 bank configurations.
  */
  size_t minDistToGoal = SIZE_MAX;

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
      const int moveIdx = (int)solver.SymTb["CrossingIndex"] - 2;
      if(moveIdx >= 0 && moveIdx < (int)solver.steps->length())
        cout<<*solver.steps->move(moveIdx).resultedState()<<endl;
      else if(nullptr != solver.steps->initialState())
        cout<<*solver.steps->initialState()<<endl;
      cout<<"  DO move "<<(moveIdx + 2)<<" : "<<move<<endl;
#endif // NDEBUG

      solver.steps->append(move);
      if(solver.steps->isSolution()) {
        commitStep();
        return; // no need to update the rest of the information now
      }

      const size_t attemptLen = solver.steps->length();
      if(attemptLen > solver.results.longestInvestigatedPath)
        solver.results.longestInvestigatedPath = attemptLen;

      const size_t crtDistToSol = solver.steps->distToSolution();
      if(solver.minDistToGoal > crtDistToSol) {
        solver.minDistToGoal = crtDistToSol;
        solver.results.closestToTargetLeftBank =
          {move.resultedState()->leftBank()};

      } else if(solver.minDistToGoal == crtDistToSol) {
        solver.results.closestToTargetLeftBank.
          push_back(move.resultedState()->leftBank());
      }

      const double prevRaftLoad = move.movedEntities().weight();
      if(prevRaftLoad > 0.) solver.SymTb["PreviousRaftLoad"] = prevRaftLoad;
      ++solver.SymTb["CrossingIndex"];

      const shared_ptr<const IState> crtState = move.resultedState();
      solver.examinedStates.emplace_back(crtState->clone());
    }

    /// Reverts the dead-end move(step)
    ~StepManager() {
      if(committedStep())
        return;

      // Dead end => backtracking
      solver.steps->pop();
      const size_t attemptLen = solver.steps->length();
      if(attemptLen > 0ULL) {
        const double theLoad =
          solver.steps->move(attemptLen-1ULL).movedEntities().weight();
        if(theLoad > 0.)
          solver.SymTb["PreviousRaftLoad"] = theLoad;
      } else
        solver.SymTb.erase("PreviousRaftLoad");

      --solver.SymTb["CrossingIndex"];

#ifndef NDEBUG
      cout<<endl<<endl<<"UNDO move "<<solver.SymTb["CrossingIndex"]<<" : "<<_move<<endl;
#endif // NDEBUG
    }

    void commitStep() {committed = true;} ///< marks the move as ok
    bool committedStep() const {return committed;}
  };

  friend class StepManager;

  /// @return true if a solution was found
  bool explore(Move &&move) {
    StepManager stepManager(*this, move);
    if(stepManager.committedStep())
      return true; // discovered solution and committed the given final move

    const shared_ptr<const IState> crtState = move.resultedState();
    const BankEntities &crtBank = crtState->nextMoveFromLeft() ?
      crtState->leftBank() : crtState->rightBank();
    vector<const MovingEntities*> allowedMovingConfigs;
    movingCfgsManager.configsForBank(crtBank, allowedMovingConfigs,
                                     crtState->nextMoveFromLeft());

    for(const MovingEntities *movingCfg : allowedMovingConfigs) {
      assert(movingCfg);
      unique_ptr<const IState> nextState = crtState->next(*movingCfg);

#ifndef NDEBUG
      cout<<endl<<"Simulating move "<<*movingCfg<<" => "<<*nextState<<endl;
#endif // NDEBUG

      if( ! nextState->valid(scenarioDetails._banksConstraints)
          || nextState->handledBy(examinedStates))
        continue; // check next raft/bridge config

      if(explore({*movingCfg, std::move(nextState), (unsigned)steps->length()})) {
        stepManager.commitStep();
        return true;
      }
    }

    return false;
  }
  /// Pretends the initial state is the result of a previous move (empty raft/bridge)
  bool explore(unique_ptr<const IState> initialState) {
    return explore({
      MovingEntities(scenarioDetails.entities),
      move(initialState),
      UINT_MAX}); // UINT_MAX because the fake initial move can't have a valid index
  }

public:
  Solver(const Scenario::Details &scenarioDetails_,
         Scenario::Results &results_) :
      scenarioDetails(scenarioDetails_),
      results(results_),
      SymTb(InitialSymbolsTable()),
      movingCfgsManager(scenarioDetails_, SymTb),
      steps(make_shared<Attempt>()) {}

  /// Looks for a solution
  void run() {
#ifndef NDEBUG
    cout<<"Exploring:"<<endl;
#endif // NDEBUG

		try {
      explore(scenarioDetails.createInitialState(SymTb));
		} catch(const exception &e) {
		  cerr<<"Couldn't solve the scenario due to: "<<e.what()<<endl;
		  steps->clear();
		}
#ifndef NDEBUG
    cout<<"Finished exploring."<<endl<<endl;
#endif // NDEBUG

    results.investigatedStates = examinedStates.size();

    assert(nullptr != steps);
    results.attempt = steps;
  }
};

} // anonymous namespace

namespace rc {

const SymbolsTable& InitialSymbolsTable(){
  static const SymbolsTable st{{"CrossingIndex", 0.}};
  return st;
}

unique_ptr<const IState>
      Scenario::Details::createInitialState(const SymbolsTable &SymTb) const {
  shared_ptr<const IStateExt> stateExt = DefStateExt::INST();

  if(nullptr != allowedLoads &&
          allowedLoads->dependsOnVariable("PreviousRaftLoad"))
    stateExt = make_shared<const PrevLoadStateExt>(SymTb, *this, stateExt);

  if(_maxDuration != UINT_MAX)
    stateExt = make_shared<const TimeStateExt>(0U, *this, stateExt);

  return make_unique<const State>(
            BankEntities(entities, entities->idsStartingFromLeftBank()),
            BankEntities(entities, entities->idsStartingFromRightBank()),
            true, // always start from left bank
            stateExt);
}

shared_ptr<const IContextValidator>
      Scenario::Details::createTransferValidator() const {
  const shared_ptr<const IContextValidator> &res = DefContextValidator::INST();
  if(nullptr == allowedLoads)
    return res;

  return
    make_shared<const AllowedLoadsValidator>(allowedLoads, res,
      make_shared<const InitiallyNoPrevRaftLoadExcHandler>(allowedLoads));
}

const Scenario::Results& Scenario::solution() {
	if( ! investigated) {
    Solver solver(details, results);
    solver.run();

		investigated = true;
	}

	return results;
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
