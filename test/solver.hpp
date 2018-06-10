/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#if ! defined SOLVER_CPP || ! defined UNIT_TESTING

  #error \
"Please include this file only at the end of `solver.cpp` \
after a `#define SOLVER_CPP` and surrounding the include and the define \
by `#ifdef UNIT_TESTING`!"

#else // for SOLVER_CPP and UNIT_TESTING

#include <boost/test/unit_test.hpp>

#include "../src/util.h"
#include "../src/entity.h"

#include <numeric>
#include <cmath>

using namespace rc;
using namespace rc::sol;

/// factorial function
template<typename T, typename = enable_if_t<is_integral<T>::value>>
constexpr T factorial(T n) {
  return (n <= (T)1) ? 1 : n * factorial(n - (T)1);
}

/// Count of k-combinations of n elements
template<typename T, typename = enable_if_t<is_integral<T>::value>>
constexpr T combsCount(T n, T k) {
  return factorial(n) / (factorial(k) * factorial(n-k));
}

/// Max count of investigated raft configurations based on the parameters
unsigned maxInvestigatedRaftConfigs(unsigned entsCount,
                                    unsigned countEntsWhoCantRow,
                                    unsigned raftCapacity) {
  assert(entsCount >= 3U);
  assert(countEntsWhoCantRow < entsCount);
  assert(raftCapacity >= 2U);
  assert(raftCapacity < entsCount);

  unsigned result = entsCount - countEntsWhoCantRow;
  for(unsigned i = 1U; i < raftCapacity; ++i)
    for(unsigned j = max(countEntsWhoCantRow, i); j < entsCount; ++j)
      result += combsCount(j, i);
  return result;
}

// Helper == between returnedConfigs and expectedConfigs (exact order matters)
template<class IfEntities,
        typename = enable_if_t<is_convertible<IfEntities*, IEntities*>::value>>
bool operator==(const vector<const IfEntities*> &returnedConfigs,
                const vector<set<unsigned>> &expectedConfigs) {
  const bool result =
    equal(CBOUNDS(returnedConfigs), CBOUNDS(expectedConfigs),
      [](const IEntities *retCfg, const set<unsigned> &cfg) {
        assert(nullptr != retCfg);
        const bool ret = *retCfg == cfg;
        if( ! ret) {
          cout<<"[ ";
          copy(CBOUNDS(retCfg->ids()), ostream_iterator<unsigned>(cout, " "));
          cout<<"] != [ ";
          copy(CBOUNDS(cfg), ostream_iterator<unsigned>(cout, " "));
          cout<<']'<<endl;
        }
        return ret;
      });
  if( ! result) {
    cout<<"IEntities { ";
    for(const IfEntities *retCfg : returnedConfigs) {
      cout<<"[ ";
      copy(CBOUNDS(retCfg->ids()), ostream_iterator<unsigned>(cout, " "));
      cout<<"] ";
    }
    cout<<"} != ExpectedEntities { ";
    for(const set<unsigned> &cfg : expectedConfigs) {
      cout<<"[ ";
      copy(CBOUNDS(cfg), ostream_iterator<unsigned>(cout, " "));
      cout<<"] ";
    }
    cout<<"}"<<endl;
  }
  return result;
}

template<class IfEntities,
        typename = enable_if_t<is_convertible<IfEntities*, IEntities*>::value>>
bool operator==(const vector<set<unsigned>> &expectedConfigs,
                const vector<const IfEntities*> &returnedConfigs) {
  return returnedConfigs == expectedConfigs;
}

BOOST_AUTO_TEST_SUITE(solver, *boost::unit_test::tolerance(Eps))

BOOST_AUTO_TEST_CASE(generateCombinations_usecases) {
  bool b;
  vector<int> vals;
  vector<vector<int>> combs;

  BOOST_CHECK_THROW(generateCombinations(CBOUNDS(vals), -1LL, combs, {}),
                    logic_error); // -1-combinations of the empty set - not ok

  BOOST_CHECK_THROW(generateCombinations(CBOUNDS(vals), 1LL, combs, {}),
                    logic_error); // 1-combinations of the empty set - not ok

  // 0-combinations of the empty set produces a single empty combination
  BOOST_CHECK_NO_THROW({
    generateCombinations(CBOUNDS(vals), 0LL, combs, {});
    BOOST_CHECK(b = (combs.size() == 1ULL));
    if(b)
      BOOST_CHECK(cbegin(combs)->empty());
    });

  ptrdiff_t n = 6, k = 3;
  vals.resize(n);
  iota(BOUNDS(vals), 1);
  combs.clear();

  // 3-combinations of the 1..6 set produces a single empty combination
  BOOST_CHECK_NO_THROW({
    generateCombinations(CBOUNDS(vals), k, combs, {});
    BOOST_CHECK(b = (combs.size() == (size_t)combsCount(n, k)));
    if(b) {
      // 1st combination is the set of 1st k values
      BOOST_CHECK(equal(CBOUNDS(*cbegin(combs)),
                        cbegin(vals), next(cbegin(vals), k)));

      // last combination is the set of last k values
      BOOST_CHECK(equal(CBOUNDS(*crbegin(combs)),
                        prev(cend(vals), k), cend(vals)));
    }});
}

BOOST_AUTO_TEST_CASE(contextValidation_usecases) {
  const SymbolsTable emptySt;
  ValueSet *pVs = new ValueSet;
  const shared_ptr<const IValues<double>> vs(pVs);
  AllEntities *pAe = new AllEntities;
  shared_ptr<const AllEntities> ae(pAe);
  MovingEntities me(ae);

  BOOST_CHECK_THROW(InitiallyNoPrevRaftLoadExcHandler(nullptr),
                    invalid_argument); // NULL allowedLoads argument

  BOOST_CHECK_THROW(InitiallyNoPrevRaftLoadExcHandler inprleh(vs),
                    invalid_argument); // empty allowedLoads argument

  BOOST_CHECK_THROW(CanRowValidator(nullptr),
                    invalid_argument); // next validator is NULL

  BOOST_CHECK_THROW(AllowedLoadsValidator(nullptr),
                    invalid_argument); // allowed loads is NULL

  BOOST_CHECK_THROW(AllowedLoadsValidator(vs, nullptr),
                    invalid_argument); // next validator is NULL

  BOOST_CHECK_NO_THROW({
    // DefContextValidator validates anything
    BOOST_CHECK(DefContextValidator().validate(me, emptySt));

    // empty set has no entities who row
    BOOST_CHECK( ! CanRowValidator().validate(me, emptySt));
  });

  BOOST_REQUIRE_NO_THROW({ // Available: 2 x t0 + 2 x t1 + 2 x t2 + t3 + t4 + t5
    *pAe += make_shared<const Entity>(0U, "e0", "t0", false, "true", 1.);
    *pAe += make_shared<const Entity>(1U, "e1", "t0", false, "false", 2.);
    *pAe += make_shared<const Entity>(2U, "e2", "t1", false, "true", 3.);
    *pAe += make_shared<const Entity>(3U, "e3", "t1", false, "false", 4.);
    *pAe += make_shared<const Entity>(4U, "e4", "t2", false, "true", 5.);
    *pAe += make_shared<const Entity>(5U, "e5", "t2", false, "false", 6.);
    *pAe += make_shared<const Entity>(6U, "e6", "t3", false, "true", 7.);
    *pAe += make_shared<const Entity>(7U, "e7", "t4", false, "true", 8.);
    *pAe += make_shared<const Entity>(8U, "e8", "t5", false, "true", 9.);
  });

  BOOST_CHECK_NO_THROW({
    CanRowValidator crv;

    // Entities with id-s 1,3,5 don't row
    BOOST_CHECK( ! crv.validate(
                me = vector<unsigned>{1U, 3U, 5U}, emptySt));

    // Entities with id-s 1,3,5 don't row, but 8 does
    BOOST_CHECK(crv.validate(
                me = vector<unsigned>{1U, 3U, 5U, 8U}, emptySt));
  });

  const shared_ptr<const DefContextValidator> defValid =
    make_shared<const DefContextValidator>();
  constexpr double d = 2.4, d1 = 4.9, d2 = 6.8;
  const string var = "a";
  const SymbolsTable st{{var, d1}}; // a := d1
  const shared_ptr<const NumericExpr>
    c = make_shared<const NumericConst>(d),
    c1 = make_shared<const NumericConst>(d1),
    c2 = make_shared<const NumericConst>(d2),
    v = make_shared<const NumericVariable>(var);
  pVs->add(ValueOrRange(c)). // hold value d   (2.4)
    add(ValueOrRange(v, c2)); // hold range 'a' .. d2  (4.9 .. 6.8)

  BOOST_CHECK_THROW(AllowedLoadsValidator(vs).validate(
                    me = vector<unsigned>{5U},
                    emptySt), out_of_range); // cannot find 'a' in emptySt

  BOOST_CHECK_NO_THROW({
    InitiallyNoPrevRaftLoadExcHandler inprleh(vs);// vs depends only on 'a'

    BOOST_CHECK( ! inprleh.dependsOnPreviousRaftLoad);

    BOOST_CHECK(boost::logic::indeterminate(inprleh.assess(me, emptySt)));
  });

  // 'a' not found in emptySt.
  // Unfortunately, InitiallyNoPrevRaftLoadExcHandler expects
  // a missing `PreviousRaftLoad` instead.
  // Besides vs doesn't depend on `PreviousRaftLoad`
  BOOST_CHECK_THROW(
    AllowedLoadsValidator(vs,         // refers to 'a'
                          defValid,   // next is the default validator
                          make_shared<const InitiallyNoPrevRaftLoadExcHandler>(
                            vs)).     // doesn't depend on `PreviousRaftLoad`
      validate(me = vector<unsigned>{5U},
               emptySt),              // doesn't contain 'a'
    out_of_range); // missing var 'a' continues to propagate

  /**
  Tackles a caught exception for a missing variable from the SymbolsTable.

  When `luckyVarName` misses from SymbolsTable, it makes assess return `outcome`.
  Otherwise, assess will return indeterminate
  */
  struct MissingVarExcHandler : public IValidatorExceptionHandler {
    string luckyVarName;
    bool outcome;

    MissingVarExcHandler(const string &luckyVarName_, bool outcome_) :
        luckyVarName(luckyVarName_), outcome(outcome_) {}

    /**
    @return outcome if `luckyVarName` misses from SymbolsTable;
      otherwise indeterminate
    */
    boost::logic::tribool assess(const MovingEntities &ents,
                                 const SymbolsTable &st) const override {
      if(st.find(luckyVarName) == cend(st))
        return outcome;
      return boost::logic::indeterminate;
    }
  };

  // 'a' not found in {{"b", 1000.}}.
  // Unfortunately, MissingVarExcHandler expects a missing 'b' instead.
  BOOST_CHECK_THROW(
    AllowedLoadsValidator(vs,         // refers to 'a'
                          defValid,   // next is the default validator
                          make_shared<const MissingVarExcHandler>(
                            "b",      // 'b' can be missing, but is present
                            false)).  // outcome if 'b' is missing
      validate(me = vector<unsigned>{5U},
               {{"b", 1000.}}),       // doesn't contain 'a', but contains 'b'
    out_of_range); // missing var 'a' continues to propagate

  BOOST_CHECK_NO_THROW({
    // 'a' not found in emptySet, but in that case
    // MissingVarExcHandler forces the `false` parameter as outcome
    BOOST_CHECK(
      ! AllowedLoadsValidator(vs,         // refers to 'a'
                              defValid,   // next is the default validator
                              make_shared<const MissingVarExcHandler>(
                                var,      // 'a' can be missing
                                false)).  // outcome if 'a' is missing
          validate(me = vector<unsigned>{5U},
                   emptySt));             // doesn't contain 'a'

    AllowedLoadsValidator alv(vs);

    // Entity 6 weighs 7 - outside 4.9..6.8 and different from 2.4
    BOOST_CHECK( ! alv.validate(
                me = vector<unsigned>{6U}, st));

    // Entity 5 weighs 6 - in 4.9..6.8
    BOOST_CHECK(alv.validate(
                me = vector<unsigned>{5U}, st));

    // Entity 1 & 3 weigh together 6 - in 4.9..6.8
    BOOST_CHECK(alv.validate(
                me = vector<unsigned>{1U, 3U}, st));
  });

  BOOST_CHECK_NO_THROW({ // combined CanRow & AllowedLoads validator
    AllowedLoadsValidator alcrv(vs, make_shared<const CanRowValidator>());

    // Entities 1,3,5 don't row
    // and weigh 12 - outside 4.9..6.8 and <> 2.4
    BOOST_CHECK( ! alcrv.validate(
                me = vector<unsigned>{1U, 3U, 5U}, st));

    // Entities with id-s 1,3,5 don't row, but 8 does,
    // but their weight is 21 - outside 4.9..6.8 and <> 2.4
    BOOST_CHECK( ! alcrv.validate(
                me = vector<unsigned>{1U, 3U, 5U, 8U}, st));

    // Entity 6 can row, but weighs 7 - outside 4.9..6.8 and <> 2.4
    BOOST_CHECK( ! alcrv.validate(
                me = vector<unsigned>{6U}, st));

    // Entity 5 weighs 6 - in 4.9..6.8, however it doesn't row
    BOOST_CHECK( ! alcrv.validate(
                me = vector<unsigned>{5U}, st));

    // Entity 1 & 3 weigh together 6 - in 4.9..6.8, however they don't row
    BOOST_CHECK( ! alcrv.validate(
                me = vector<unsigned>{1U, 3U}, st));

    // Entity 4 weighs 5 - in 4.9..6.8 and it does row
    BOOST_CHECK(alcrv.validate(
                me = vector<unsigned>{4U}, st));
  });

  pVs->add(ValueOrRange(make_shared<const NumericVariable>("PreviousRaftLoad")));

  BOOST_CHECK_NO_THROW({ // vs depends now also on 'PreviousRaftLoad'
    shared_ptr<const InitiallyNoPrevRaftLoadExcHandler> inprleh =
      make_shared<const InitiallyNoPrevRaftLoadExcHandler>(vs);

    BOOST_CHECK(inprleh->dependsOnPreviousRaftLoad);

    BOOST_CHECK(inprleh->assess(me, InitialSymbolsTable())); // idx is 0
    BOOST_CHECK(inprleh->assess(me,
                                {{"CrossingIndex",
                                1.}})); // true for idx <=1

    BOOST_CHECK(boost::logic::indeterminate(inprleh->assess(me,
                                     {{"CrossingIndex",
                                     2.}}))); // indeterminate for idx >=2
    // indeterminate as well when PreviousRaftLoad appears
    BOOST_CHECK(boost::logic::indeterminate(inprleh->assess(me,
      {{"PreviousRaftLoad", 15.}})));
    // indeterminate as well when both CrossingIndex and PreviousRaftLoad appear
    BOOST_CHECK(boost::logic::indeterminate(inprleh->assess(me,
      {{"CrossingIndex", 1.}, {"PreviousRaftLoad", 15.}})));
    // indeterminate as well when both CrossingIndex and PreviousRaftLoad miss
    BOOST_CHECK(boost::logic::indeterminate(inprleh->assess(me, emptySt)));

    AllowedLoadsValidator alv(vs, defValid, inprleh);

    // Although 'a' is missing, validation succeeds due to inprleh
    BOOST_CHECK(alv.validate( // no `PreviousRaftLoad`; `CrossingIndex` is 0
                me = vector<unsigned>{6U}, InitialSymbolsTable()));

    // Although 'a' is missing, validation succeeds due to inprleh
    BOOST_CHECK(alv.validate( // no `PreviousRaftLoad`; `CrossingIndex` is 1
                me = vector<unsigned>{6U},
                {{"CrossingIndex", 1.}}));

    // for the cases below the out_of_range due to missing 'a' propagates
    BOOST_CHECK_THROW(alv.validate(me = vector<unsigned>{6U},
                {{"CrossingIndex", 2.}}),
                out_of_range); // idx >= 2
    BOOST_CHECK_THROW(alv.validate(me = vector<unsigned>{6U},
                {{"PreviousRaftLoad", 15.}}),
                out_of_range); // PreviousRaftLoad appears
    BOOST_CHECK_THROW(alv.validate(me = vector<unsigned>{6U},
                {{"CrossingIndex", 1.}, {"PreviousRaftLoad", 15.}}),
                out_of_range); // both CrossingIndex and PreviousRaftLoad appear
    BOOST_CHECK_THROW(alv.validate(me = vector<unsigned>{6U}, emptySt),
                out_of_range); // both CrossingIndex and PreviousRaftLoad miss
  });
}

BOOST_AUTO_TEST_CASE(movingConfigOption_usecases) {
  const SymbolsTable emptySt;
  AllEntities *pAe = new AllEntities;
  shared_ptr<const AllEntities> ae(pAe);

  BOOST_REQUIRE_NO_THROW({
    *pAe += make_shared<const Entity>(0U, "e0", "t0", false, "true", 1.);
    *pAe += make_shared<const Entity>(1U, "e1", "t0", false, "false", 2.);
    *pAe += make_shared<const Entity>(2U, "e2", "t1", false, "true", 3.);
    *pAe += make_shared<const Entity>(3U, "e3", "t1", false, "false", 4.);
  });

  MovingEntities me(ae);
  BankEntities be(ae);

  BOOST_CHECK_THROW(MovingConfigOption(me, nullptr),
                    invalid_argument); // NULL validator

  BOOST_CHECK_NO_THROW({
    BOOST_CHECK(MovingConfigOption(me).validFor(be, emptySt)); // empty bank & raft

    BOOST_CHECK(MovingConfigOption(me). // empty set for raft included in any bank
                validFor(be = vector<unsigned>{0U, 1U, 2U, 3U}, emptySt));

    BOOST_CHECK(MovingConfigOption(me = vector<unsigned>{1U, 2U}). // both are in
                validFor(be = vector<unsigned>{0U, 1U, 2U, 3U}, emptySt));

    BOOST_CHECK( ! MovingConfigOption(me = vector<unsigned>{1U, 2U}). // 1U is missing
                validFor(be = vector<unsigned>{0U, 2U, 3U}, emptySt));

    BOOST_CHECK( ! MovingConfigOption(me = vector<unsigned>{1U, 2U}). // 2U is missing
                validFor(be = vector<unsigned>{0U, 1U, 3U}, emptySt));

    BOOST_CHECK(MovingConfigOption(me = vector<unsigned>{1U, 2U}, // both are in
                              make_shared<const CanRowValidator>()). // 2U rows
                validFor(be = vector<unsigned>{0U, 1U, 2U, 3U}, emptySt));

    BOOST_CHECK( ! MovingConfigOption(me = vector<unsigned>{1U}, // 1U is in
                      make_shared<const CanRowValidator>()). // 1U doesn't row
                validFor(be = vector<unsigned>{0U, 1U, 2U, 3U}, emptySt));
  });
}

BOOST_AUTO_TEST_CASE(movingConfigsManager_usecases) {
  const SymbolsTable emptySt;
  AllEntities *pAe = new AllEntities;
  BOOST_REQUIRE_NO_THROW({ // Using initially only entities who cannot row
    *pAe += make_shared<const Entity>(1U, "e1", "t0", false, "false", 1.);
    *pAe += make_shared<const Entity>(3U, "e3", "t1", false, "false", 3.);
    *pAe += make_shared<const Entity>(5U, "e5", "t2", false, "false", 5.);
    *pAe += make_shared<const Entity>(10U, "e10", "t0", false, "false", 10.);
    *pAe += make_shared<const Entity>(11U, "e11", "t0", false, "false", 11.);
  });
  const size_t entsCantRowCount = pAe->count();

  Scenario::Details sd;
  sd.entities = shared_ptr<const AllEntities>(pAe);
  BankEntities be(sd.entities);

  BOOST_CHECK_THROW(MovingConfigsManager(sd, emptySt),
                     domain_error); // none of the entities is/will be able to row

  BOOST_REQUIRE_NO_THROW({ // Adding several entities who all can / might row
    *pAe += make_shared<const Entity>(2U, "e2", "t1", false,
                                      "if (%CrossingIndex% mod 2) in {1}", 2.);
    *pAe += make_shared<const Entity>(4U, "e4", "t2", false,
                                      "if (%CrossingIndex% mod 2) in {0}", 4.);
    *pAe += make_shared<const Entity>(6U, "e6", "t3", false,
                                      "if (%CrossingIndex% mod 2) in {1}", 6.);
    *pAe += make_shared<const Entity>(7U, "e7", "t4", false,
                                      "if (%CrossingIndex% mod 2) in {0}", 7.);
    *pAe += make_shared<const Entity>(8U, "e8", "t5", false,
                                      "if (%CrossingIndex% mod 2) in {1}", 8.);
    *pAe += make_shared<const Entity>(9U, "e0", "t0", false,
                                      "true", 9.);
  });

  const size_t entsCount = pAe->count();

  BOOST_CHECK_THROW(MovingConfigsManager(sd, emptySt),
                     logic_error); // capacity is UINT_MAX >= entsCount

  sd._capacity = (unsigned)entsCount - 1U;

  // The default transfer constraints
  sd._transferConstraints = make_unique<const TransferConstraints>(
            grammar::ConstraintsVec{}, sd.entities,
            sd._capacity, sd._maxLoad,
            false);
  vector<const MovingEntities*> configsForABank;

  BOOST_CHECK_NO_THROW({
    {
      assert(sd._capacity == (unsigned)entsCount - 1U);
      MovingConfigsManager mcm(sd, emptySt);

      const double investigatedRaftCombs =
        maxInvestigatedRaftConfigs((unsigned)entsCount,
                                   (unsigned)entsCantRowCount,
                                   sd._capacity);

      /*
       About the expression from the test below:

       A: exp2(entsCount) - 1.
        All combinations of the entities
        minus the one including them all
       B: exp2(entsCantRowCount)
        All combinations including only the entities who can't row,
        or no entities

       Let's remark A guarantees the remaining combinations
       miss at least 1 entity, which is what sd._capacity specifies
      */
      BOOST_TEST(exp2(entsCount) - 1.       // A explained above
                 - exp2(entsCantRowCount)   // B explained above

                 == investigatedRaftCombs);
      BOOST_TEST(investigatedRaftCombs == (double)mcm.allConfigs.size());

      // None of 1,3,5 can row
      mcm.configsForBank(be = vector<unsigned>({1U, 3U, 5U}), configsForABank, true);
      BOOST_CHECK(configsForABank.empty());
      mcm.configsForBank(be, configsForABank, false);
      BOOST_CHECK(configsForABank.empty());

      // 9 can row
      vector<set<unsigned>> expectedConfigs({{9}, {1, 9}, {3, 9}, {1, 3, 9}});
      mcm.configsForBank(be = vector<unsigned>({1U, 3U, 9U}), configsForABank, false);
      BOOST_CHECK(configsForABank == expectedConfigs);
      const shared_ptr<const sol::IContextValidator> validator =
        sd.createTransferValidator(); // allows anything now
      bool b = false;
      BOOST_CHECK(b = (nullptr != validator));
      if(b) {
        // allows even an unexpected config, where no entity can row
        BOOST_CHECK(validator->validate(
          ent::MovingEntities(sd.entities, vector<unsigned>({1U, 3U, 5U})),
          emptySt));

        // allow confirmed configurations
        for(const ent::MovingEntities *cfg : configsForABank) {
          if(nullptr != cfg)
            BOOST_TEST_CONTEXT("for raft cfg: `"<<*cfg<<'`') {
              BOOST_CHECK(validator->validate(*cfg, emptySt));
            }
        }
      }

      mcm.configsForBank(be, configsForABank, true);
      reverse(BOUNDS(expectedConfigs));
      BOOST_CHECK(configsForABank == expectedConfigs);
    }

    {
      sd._capacity = 2U;
      MovingConfigsManager mcm(sd, emptySt);

      const double investigatedRaftCombs =
        maxInvestigatedRaftConfigs((unsigned)entsCount,
                                   (unsigned)entsCantRowCount,
                                   sd._capacity);

      BOOST_TEST(investigatedRaftCombs == (double)mcm.allConfigs.size());

      // None of 1,3,5 can row
      mcm.configsForBank(be = vector<unsigned>({1U, 3U, 5U}), configsForABank, true);
      BOOST_CHECK(configsForABank.empty());
      mcm.configsForBank(be, configsForABank, false);
      BOOST_CHECK(configsForABank.empty());

      // 9 can row
      vector<set<unsigned>> expectedConfigs({{9}, {1, 9}, {3, 9}});
      mcm.configsForBank(be = vector<unsigned>({1U, 3U, 9U}), configsForABank, false);
      BOOST_CHECK(configsForABank == expectedConfigs);

      mcm.configsForBank(be, configsForABank, true);
      reverse(BOUNDS(expectedConfigs));
      BOOST_CHECK(configsForABank == expectedConfigs);
    }

    {
      assert(sd._capacity == 2U);
      sd._maxLoad = 13.;
      MovingConfigsManager mcm(sd, emptySt);

      const double maxInvestigatedRaftCombs =
        maxInvestigatedRaftConfigs((unsigned)entsCount,
                                   (unsigned)entsCantRowCount,
                                   sd._capacity);

      BOOST_TEST(maxInvestigatedRaftCombs >= (double)mcm.allConfigs.size());

      // 9 can row; 5&9 weigh 14 > max load; The expected configs respect maxLoad
      vector<set<unsigned>> expectedConfigs({{9}});
      mcm.configsForBank(be = vector<unsigned>({5U, 9U}), configsForABank, true);
      BOOST_CHECK(configsForABank == expectedConfigs);
      mcm.configsForBank(be, configsForABank, false);
      BOOST_CHECK(configsForABank == expectedConfigs);

      // 9 can row; 5&9 weigh 14 > max load; The expected configs respect maxLoad
      expectedConfigs = initializer_list<set<unsigned>>({{9}, {1, 9}, {3, 9}});
      mcm.configsForBank(be = vector<unsigned>({1U, 3U, 5U, 9U}), configsForABank,
                         false);
      BOOST_CHECK(configsForABank == expectedConfigs);

      mcm.configsForBank(be, configsForABank, true);
      reverse(BOUNDS(expectedConfigs));
      BOOST_CHECK(configsForABank == expectedConfigs);
    }

    {
      assert(sd._capacity == 2U);
      assert(sd._maxLoad == 13.);

      IdsConstraint *pIc = new IdsConstraint;
      shared_ptr<const IdsConstraint> ic = shared_ptr<const IdsConstraint>(pIc);
      pIc->addOptionalId(9U).addUnspecifiedMandatory(); // 9? *
      sd._transferConstraints =
        make_unique<const TransferConstraints>(grammar::ConstraintsVec{ic},
                                               sd.entities,
                                               sd._capacity, sd._maxLoad,
                                               true);
      MovingConfigsManager mcm(sd, emptySt);

      const double maxInvestigatedRaftCombs =
        maxInvestigatedRaftConfigs((unsigned)entsCount,
                                   (unsigned)entsCantRowCount,
                                   sd._capacity);

      BOOST_TEST(maxInvestigatedRaftCombs >= (double)mcm.allConfigs.size());

      // 9 can row, 5 doesn't; 9 cannot appear alone; 5&9 weigh 14 > max load
      mcm.configsForBank(be = vector<unsigned>({5U, 9U}), configsForABank, true);
      BOOST_CHECK(configsForABank.empty());
      mcm.configsForBank(be, configsForABank, false);
      BOOST_CHECK(configsForABank.empty());

      // only 9 can row; 9 cannot appear alone; 5&9 weigh 14 > max load
      vector<set<unsigned>> expectedConfigs({{1, 9}, {3, 9}});
      mcm.configsForBank(be = vector<unsigned>({1U, 3U, 5U, 9U}), configsForABank,
                         false);
      BOOST_CHECK(configsForABank == expectedConfigs);

      mcm.configsForBank(be, configsForABank, true);
      reverse(BOUNDS(expectedConfigs));
      BOOST_CHECK(configsForABank == expectedConfigs);
    }

    {
      assert(sd._capacity == 2U);
      assert(sd._maxLoad == 13.);
      assert(sd._transferConstraints != nullptr); // 9? *

      ValueSet *pVs = new ValueSet;
      pVs->add(ValueOrRange(make_shared<const NumericConst>(12.)));
      sd.allowedLoads = shared_ptr<const ValueSet>(pVs);

      // CrossingIndex is 0, so 7 can row
      MovingConfigsManager mcm(sd, InitialSymbolsTable());

      /*
       The config requires one entity besides the optional 9;
       1&3&5&11 don't row; 9 can row and 7 can row sometimes;
       5&9, 7&9, 9&11 and 7&11 weigh > max load;
       1&9 and 9 weigh different than 12;
       5&7 are 2 entities and don't contain 9
      */
      vector<set<unsigned>> expectedConfigs({{3U, 9U}});
      mcm.configsForBank(be = vector<unsigned>({1U, 3U, 5U, 7U, 9U, 11U}),
                         configsForABank,
                         false);
      BOOST_CHECK(configsForABank == expectedConfigs);
      const shared_ptr<const sol::IContextValidator> validator =
        sd.createTransferValidator(); // allows anything weighing 12
      bool b = false;
      BOOST_CHECK(b = (nullptr != validator));
      if(b) {
        // allows even an unexpected config weighing 12, where no entity can row
        BOOST_CHECK(validator->validate(
          ent::MovingEntities(sd.entities, vector<unsigned>({1U, 11U})),
          emptySt));

        // allows 5&7 weighing 12, which violates `9? *` constraint
        BOOST_CHECK(validator->validate(
          ent::MovingEntities(sd.entities, vector<unsigned>({5U, 7U})),
          InitialSymbolsTable()));

        // allow confirmed configurations
        for(const ent::MovingEntities *cfg : configsForABank) {
          if(nullptr != cfg)
            BOOST_TEST_CONTEXT("for raft cfg: `"<<*cfg<<'`') {
              BOOST_CHECK(validator->validate(*cfg, emptySt));
            }
        }
      }

      mcm.configsForBank(be, configsForABank, true);
      BOOST_CHECK(configsForABank == expectedConfigs);
    }
  });
}

BOOST_AUTO_TEST_CASE(algorithmStates_usecases) {
  AllEntities *pAe = new AllEntities;
  shared_ptr<const AllEntities> ae = shared_ptr<const AllEntities>(pAe);

  BOOST_REQUIRE_NO_THROW({
    *pAe += make_shared<const Entity>(1U, "e1", "t0", false, "false", 1.);
    *pAe += make_shared<const Entity>(2U, "e2", "t1", false,
                                      "if (%CrossingIndex% mod 2) in {1}", 2.);
    *pAe += make_shared<const Entity>(3U, "e3", "t1", false, "false", 3.);
    *pAe += make_shared<const Entity>(4U, "e4", "t2", false,
                                      "if (%CrossingIndex% mod 2) in {0}", 4.);
    *pAe += make_shared<const Entity>(5U, "e5", "t2", false, "false", 5.);
    *pAe += make_shared<const Entity>(6U, "e6", "t3", false,
                                      "if (%CrossingIndex% mod 2) in {1}", 6.);
    *pAe += make_shared<const Entity>(7U, "e7", "t4", false,
                                      "if (%CrossingIndex% mod 2) in {0}", 7.);
    *pAe += make_shared<const Entity>(8U, "e8", "t5", false,
                                      "if (%CrossingIndex% mod 2) in {1}", 8.);
    *pAe += make_shared<const Entity>(9U, "e0", "t0", false,
                                      "true", 9.);
  });
  const size_t entsCount = pAe->count();

  BankEntities right(ae), left = ~right;
  BOOST_CHECK_THROW(State(left, left, true),
                    invalid_argument); // uncomplementary bank configs

  const unsigned t = 1234U, cap = 2U;
  const double maxLoad = 12.;
  IdsConstraint *pIc = new IdsConstraint;
  pIc->addMandatoryId(1U).addUnspecifiedMandatory(); // 1 *
  shared_ptr<const IConfigConstraint> ic =
    shared_ptr<const IdsConstraint>(pIc);
  grammar::ConfigurationsTransferDurationInitType ctdit;
  ctdit.setDuration(4321U).setConstraints({ic});

  Scenario::Details info;
  info.entities = ae;
  info._capacity = cap;
  info._maxLoad = maxLoad;
  info._maxDuration = t*t;
  info.ctdItems.emplace_back(std::move(ctdit), ae, cap, maxLoad);

  MovingEntities moved(ae, vector<unsigned>({2U, 3U}));

  {
    BankEntities left5(left), right5(right);
    left5 -= moved; right5 += moved;

    BOOST_CHECK_THROW(State(left, left, true),
                      invalid_argument); // non-complementary banks

    shared_ptr<const TimeStateExt> stateExt, stateExt3, stateExt4;
    BOOST_CHECK_THROW(TimeStateExt(t, info, stateExt),
                      invalid_argument); // stateExt is nullptr now

    BOOST_REQUIRE_NO_THROW({
      stateExt = make_shared<const TimeStateExt>(t, info);
      stateExt3 = make_shared<const TimeStateExt>(t+1U, info);
      stateExt4 = make_shared<const TimeStateExt>(t-1U, info);
      State s(left, right, true, stateExt);
      State s1(s);
    });
    State s(left, right, true, stateExt), s1(s), s2(s),
          s3(left, right, true, stateExt3),
          s4(left, right, true, stateExt4),
          s5(left5, right5, true, stateExt);
    s2._nextMoveFromLeft = ! s2._nextMoveFromLeft;
    vector<unique_ptr<const IState>> someStates;
    someStates.emplace_back(s2.clone());
    someStates.emplace_back(s3.clone());
    someStates.emplace_back(s5.clone());

    BOOST_CHECK_NO_THROW({
      shared_ptr<const TimeStateExt> timeExt =
        AbsStateExt::selectExt<TimeStateExt>(s.getExtension());
      bool b = false;
      BOOST_CHECK(b = (nullptr != timeExt));
      if(b)
        BOOST_CHECK(t == timeExt->time());
    });
    BOOST_CHECK_NO_THROW({
      shared_ptr<const PrevLoadStateExt> prevLoadExt =
        AbsStateExt::selectExt<PrevLoadStateExt>(s.getExtension());
      BOOST_CHECK(nullptr == prevLoadExt);
    });

    BOOST_CHECK(s.rightBank().empty());
    BOOST_CHECK(s.leftBank() == pAe->ids());
    BOOST_CHECK(s.nextMoveFromLeft());
    BOOST_CHECK( ! s.handledBy(s2)); // direction mismatch
    BOOST_CHECK( ! s.handledBy(s3)); // s3 occurred later, so s is preferred
    BOOST_CHECK( ! s.handledBy(s5)); // different bank configs
    BOOST_CHECK( ! s.handledBy(someStates)); // {s2, s3, s5}
    BOOST_CHECK(s.handledBy(s1)); // same
    BOOST_CHECK(s.handledBy(s4)); // s4 occurred earlier than s

    someStates.emplace_back(s4.clone());
    BOOST_CHECK(s.handledBy(someStates)); // {s2, s3, s5, s4}

    unique_ptr<const IState> nextS;
    bool b = false;
    BOOST_CHECK_NO_THROW({
      nextS = s.next(moved = vector<unsigned>({1U,9U}));
      b = true;
    });
    if(b) {
      b = false;
      unique_ptr<const IState> cloneNextS;
      BOOST_CHECK_NO_THROW({
        cloneNextS = nextS->clone();
        b = true;
      });
      if(b) {
        BOOST_CHECK(cloneNextS->rightBank() == set<unsigned>({1U,9U}));
        BOOST_CHECK(cloneNextS->leftBank().count() == entsCount - 2ULL);
        BOOST_CHECK( ! cloneNextS->nextMoveFromLeft());
        BOOST_CHECK_NO_THROW({
          shared_ptr<const TimeStateExt> timeExtCloneNextS =
            AbsStateExt::selectExt<TimeStateExt>(cloneNextS->getExtension());
          bool b = false;
          BOOST_CHECK(b = (nullptr != timeExtCloneNextS));
          if(b)
            BOOST_CHECK(5555U == timeExtCloneNextS->time());
        });
      }
    }

    // ctds covers only config '1 *'. There is no duration for config 2
    BOOST_CHECK_THROW(s.next(moved = vector<unsigned>({2U})),
                      domain_error);
  }

  {
    shared_ptr<const IState> s =
      make_shared<const State>(left, right, true); // default extension

    shared_ptr<const IStateExt> nullExt;
    shared_ptr<const TimeStateExt> timeExt =
      make_shared<const TimeStateExt>(t, info);

    BOOST_CHECK_THROW(PrevLoadStateExt(InitialSymbolsTable(), info, nullExt),
                      invalid_argument); // NULL next extension

    BOOST_CHECK_THROW(PrevLoadStateExt(SymbolsTable(), info),
                      logic_error); // missing CrossingIndex entry

    // missing PreviousRaftLoad entry when CrossingIndex >= 2
    BOOST_CHECK_THROW(PrevLoadStateExt plse(SymbolsTable({{"CrossingIndex", 2.}}),
                                            info),
                      logic_error);

    BOOST_CHECK_NO_THROW({ // CrossingIndex 0 or 1 and no PreviousRaftLoad is ok
      PrevLoadStateExt plse1(InitialSymbolsTable(), info,
                             timeExt); // CrossingIndex 0
      State s1(left, right, true, plse1.clone());
      BOOST_CHECK(isNaN(plse1.prevRaftLoad()));

      PrevLoadStateExt plse2(SymbolsTable({{"CrossingIndex", 1.}}), info,
                             timeExt);
      State s2(left, right, true, plse2.clone());
      BOOST_CHECK(isNaN(plse2.prevRaftLoad()));

      PrevLoadStateExt plse3(SymbolsTable({{"CrossingIndex", 2.},
                                          {"PreviousRaftLoad", 10.}}),
                             info, timeExt);
      State s3(left, right, true, plse3.clone());
      BOOST_TEST(plse3.prevRaftLoad() == 10.);

      BOOST_CHECK(s3.handledBy(s1)); // CrossingIndex 0/1 handles all
      BOOST_CHECK(s3.handledBy(s2)); // CrossingIndex 0/1 handles all

      // s has only the default extension
      BOOST_CHECK_THROW(s3.handledBy(*s),
                        logic_error);

      PrevLoadStateExt plse4(SymbolsTable({{"CrossingIndex", 15.},
                                          {"PreviousRaftLoad", 10.}}),
                             info, timeExt);
      State s4(left, right, true, plse4.clone());
      BOOST_CHECK(s4.handledBy(s1)); // CrossingIndex 0/1 handles all
      BOOST_CHECK(s4.handledBy(s2)); // CrossingIndex 0/1 handles all
      BOOST_CHECK(s4.handledBy(s3)); // same PreviousRaftLoad
      BOOST_CHECK(s3.handledBy(s4)); // same PreviousRaftLoad

      PrevLoadStateExt plse5(SymbolsTable({{"CrossingIndex", 13.},
                                          {"PreviousRaftLoad", 18.}}),
                             info, timeExt);
      State s5(left, right, true, plse5.clone());
      BOOST_CHECK(s5.handledBy(s1)); // CrossingIndex 0/1 handles all
      BOOST_CHECK(s5.handledBy(s2)); // CrossingIndex 0/1 handles all
      BOOST_CHECK( ! s5.handledBy(s3)); // different PreviousRaftLoad
      BOOST_CHECK( ! s5.handledBy(s4)); // different PreviousRaftLoad

      unique_ptr<const IState> cloneOfS5 = s5.clone();
      BOOST_CHECK(cloneOfS5->rightBank().empty());
      BOOST_CHECK(cloneOfS5->leftBank().count() == entsCount);
      BOOST_CHECK(cloneOfS5->nextMoveFromLeft());

      shared_ptr<const PrevLoadStateExt> pCloneOfPlse5 =
        AbsStateExt::selectExt<PrevLoadStateExt>(cloneOfS5->getExtension());
      shared_ptr<const TimeStateExt> pCloneOfTse5 =
        AbsStateExt::selectExt<TimeStateExt>(cloneOfS5->getExtension());
      bool b = false;
      bool b1 = false;
      BOOST_CHECK(b = (nullptr != pCloneOfPlse5));
      BOOST_CHECK(b1 = (nullptr != pCloneOfTse5));
      if(b && b1) {
        BOOST_CHECK(pCloneOfTse5->time() == t);

        BOOST_TEST(pCloneOfPlse5->crossingIdx() == 13U);
        BOOST_TEST(pCloneOfPlse5->prevRaftLoad() == 18.);
      }

      unique_ptr<const IState> nextOfS5 =
        s5.next(moved = vector<unsigned>({1U, 9U})); // weight 10
      BOOST_CHECK(nextOfS5->rightBank() == set<unsigned>({1U,9U}));
      BOOST_CHECK(nextOfS5->leftBank().count() == entsCount - 2ULL);
      BOOST_CHECK( ! nextOfS5->nextMoveFromLeft());

      shared_ptr<const PrevLoadStateExt> pNextOfPlse5 =
        AbsStateExt::selectExt<PrevLoadStateExt>(nextOfS5->getExtension());
      shared_ptr<const TimeStateExt> pNextOfTse5 =
        AbsStateExt::selectExt<TimeStateExt>(nextOfS5->getExtension());
      b = b1 = false;
      BOOST_CHECK(b = (nullptr != pNextOfPlse5));
      BOOST_CHECK(b1 = (nullptr != pNextOfTse5));
      if(b && b1) {
        BOOST_CHECK(pNextOfTse5->time() == 5555U);

        BOOST_TEST(pNextOfPlse5->crossingIdx() == 14U);
        BOOST_TEST(pNextOfPlse5->prevRaftLoad() == 10.);
      }

      // s has the default extension and all those states are equivalent
      // when no other extensions are considered
      BOOST_CHECK(s->handledBy(s1));
      BOOST_CHECK(s->handledBy(s2));
      BOOST_CHECK(s->handledBy(s3));
      BOOST_CHECK(s->handledBy(s4));
      BOOST_CHECK(s->handledBy(s5));
    });
  }

  {
    Scenario::Details d;
    d.entities = ae;
    d.allowedLoads = nullptr;

    // Default extension (allowedLoads is NULL and _maxDuration is UINT_MAX)
    BOOST_CHECK_NO_THROW({
      unique_ptr<const IState> initS = d.createInitialState(InitialSymbolsTable());

      bool b = false;
      BOOST_CHECK(b = (nullptr != initS));
      if(b) {
        BOOST_CHECK(nullptr ==
                    AbsStateExt::selectExt<TimeStateExt>(initS->getExtension()));
        BOOST_CHECK(nullptr ==
                    AbsStateExt::selectExt<PrevLoadStateExt>(initS->getExtension()));
      }
    });

    ValueSet *pVs = new ValueSet;
    d.allowedLoads = shared_ptr<const ValueSet>(pVs);

    // Default extension (allowedLoads doesn't depend on PreviousRaftLoad; _maxDuration is UINT_MAX)
    BOOST_CHECK_NO_THROW({
      unique_ptr<const IState> initS = d.createInitialState(InitialSymbolsTable());

      bool b = false;
      BOOST_CHECK(b = (nullptr != initS));
      if(b) {
        BOOST_CHECK(nullptr ==
                    AbsStateExt::selectExt<TimeStateExt>(initS->getExtension()));
        BOOST_CHECK(nullptr ==
                    AbsStateExt::selectExt<PrevLoadStateExt>(initS->getExtension()));
      }
    });

    pVs->add(ValueOrRange(make_shared<const NumericVariable>("PreviousRaftLoad")));

    // PrevLoadStateExt (allowedLoads depends on PreviousRaftLoad; _maxDuration is UINT_MAX)
    BOOST_CHECK_NO_THROW({
      unique_ptr<const IState> initS = d.createInitialState(InitialSymbolsTable());

      bool b = false;
      BOOST_CHECK(b = (nullptr != initS));
      if(b) {
        BOOST_CHECK(nullptr ==
                    AbsStateExt::selectExt<TimeStateExt>(initS->getExtension()));
        BOOST_CHECK(nullptr !=
                    AbsStateExt::selectExt<PrevLoadStateExt>(initS->getExtension()));
      }
    });

    d._maxDuration = t * t;

    // PrevLoadStateExt & TimeStateExt (allowedLoads depends on PreviousRaftLoad; _maxDuration < UINT_MAX)
    BOOST_CHECK_NO_THROW({
      unique_ptr<const IState> initS = d.createInitialState(InitialSymbolsTable());

      bool b = false;
      BOOST_CHECK(b = (nullptr != initS));
      if(b) {
        BOOST_CHECK(nullptr !=
                    AbsStateExt::selectExt<TimeStateExt>(initS->getExtension()));
        BOOST_CHECK(nullptr !=
                    AbsStateExt::selectExt<PrevLoadStateExt>(initS->getExtension()));
      }
    });
  }
}

BOOST_AUTO_TEST_CASE(algorithmMove_usecases) {
  AllEntities *pAe = new AllEntities;
  shared_ptr<const AllEntities> ae = shared_ptr<const AllEntities>(pAe);

  BOOST_REQUIRE_NO_THROW({
    *pAe += make_shared<const Entity>(1U, "e1", "t0", false, "false", 1.);
    *pAe += make_shared<const Entity>(2U, "e2", "t1", false,
                                      "if (%CrossingIndex% mod 2) in {1}", 2.);
    *pAe += make_shared<const Entity>(3U, "e3", "t1", false, "false", 3.);
    *pAe += make_shared<const Entity>(4U, "e4", "t2", false,
                                      "if (%CrossingIndex% mod 2) in {0}", 4.);
  });

  BankEntities right(ae), left = ~right;
  MovingEntities me(ae, vector<unsigned>({2U, 3U}));
  unique_ptr<const IState> s = make_unique<const State>(left, right, true),
    clonedS = s->clone();

  BOOST_CHECK_THROW(Move m(me, nullptr, 0U),
                    invalid_argument); // NULL resulted state

  BOOST_CHECK_THROW(Move m(me,
                           make_unique<const State>(left, right, false), 0U),
                    logic_error); // Some moved entities missing from receiver bank

  BOOST_CHECK_NO_THROW({
    Move m(me, std::move(s), 123U);
    BOOST_CHECK(123U == m.index());
    BOOST_CHECK(me == m.movedEntities());
    BOOST_CHECK(m.resultedState()->handledBy(*clonedS));
    BOOST_CHECK(clonedS->handledBy(*m.resultedState()));
  });
}

BOOST_AUTO_TEST_CASE(currentAttempt_usecases) {
  Attempt a;

  BOOST_CHECK_NO_THROW(a.pop());

  AllEntities *pAe = new AllEntities;
  shared_ptr<const AllEntities> ae = shared_ptr<const AllEntities>(pAe);

  BOOST_REQUIRE_NO_THROW({
    *pAe += make_shared<const Entity>(1U, "a", "", false, "true");
    *pAe += make_shared<const Entity>(2U, "b", "", false, "false");
    *pAe += make_shared<const Entity>(3U, "c", "", false, "false");
  });

  Scenario::Details d;
  d.entities = ae;
  d._capacity = 2U;

  BOOST_CHECK_THROW(a.append({MovingEntities(ae, vector<unsigned>({1U})),
                             d.createInitialState(InitialSymbolsTable()),
                             UINT_MAX}),
                    logic_error); // MovingEntities for first append not empty

  BOOST_CHECK( ! a.isSolution());
  BOOST_CHECK(a.length() == 0ULL);
  BOOST_CHECK(a.distToSolution() == SIZE_MAX);
  BOOST_CHECK(a.initialState() == nullptr);
  BOOST_CHECK(a.targetLeftBank == nullptr);
  BOOST_CHECK_THROW(a.move(0ULL),
                    out_of_range); // no moves yet

  BOOST_CHECK_NO_THROW({
    MovingEntities moved(ae);

    // The move creating the initial state
    a.append({moved, // empty right now
             d.createInitialState(InitialSymbolsTable()),
             UINT_MAX}); // doesn't need a meaningful index
    BOOST_CHECK( ! a.isSolution());
    BOOST_CHECK(a.length() == 0ULL);
    BOOST_CHECK(a.distToSolution() == 3ULL);
    BOOST_CHECK(a.initialState() != nullptr);
    BOOST_CHECK(a.targetLeftBank != nullptr);
    BOOST_CHECK_THROW(a.move(0ULL),
                      out_of_range); // no moves yet

    // First move
    moved = vector<unsigned>({1U, 2U});
    BOOST_CHECK_THROW(a.append({moved,
                               VP(a.initSt)->next(moved),
                               1234U}),
                      logic_error); // first move needs index 0, not 1234
    a.append({moved,
             VP(a.initSt)->next(moved),
             0U});
    BOOST_CHECK( ! a.isSolution());
    BOOST_CHECK(a.length() == 1ULL);
    BOOST_CHECK(a.distToSolution() == 1ULL);
    BOOST_CHECK(a.initialState() != nullptr);
    BOOST_CHECK(a.targetLeftBank != nullptr);
    BOOST_CHECK_THROW(a.move(a.length()),
                      out_of_range); // right beyond last valid index
    const IMove &firstMove = a.move(0ULL);

    // Second move
    moved = vector<unsigned>({1U});
    BOOST_CHECK_THROW(a.append({moved,
                               VP(firstMove.resultedState())->next(moved),
                               1234U}),
                      logic_error); // second move needs index 1, not 1234
    a.append({moved,
             VP(firstMove.resultedState())->next(moved),
             1U});
    BOOST_CHECK( ! a.isSolution());
    BOOST_CHECK(a.length() == 2ULL);
    BOOST_CHECK(a.distToSolution() == 2ULL);
    BOOST_CHECK(a.initialState() != nullptr);
    BOOST_CHECK(a.targetLeftBank != nullptr);
    BOOST_CHECK_THROW(a.move(a.length()),
                      out_of_range); // right beyond last valid index
    const IMove &secondMove = a.move(1ULL);

    // Third move
    moved = vector<unsigned>({1U, 3U});
    BOOST_CHECK_THROW(a.append({moved,
                               VP(secondMove.resultedState())->next(moved),
                               1234U}),
                      logic_error); // third move needs index 2, not 1234
    a.append({moved,
             VP(secondMove.resultedState())->next(moved),
             2U});
    BOOST_CHECK(a.isSolution());
    BOOST_CHECK(a.length() == 3ULL);
    BOOST_CHECK(a.distToSolution() == 0ULL);
    BOOST_CHECK(a.initialState() != nullptr);
    BOOST_CHECK(a.targetLeftBank != nullptr);
    BOOST_CHECK_THROW(a.move(a.length()),
                      out_of_range); // right beyond last valid index
    const IMove &thirdMove = a.move(2ULL);

    BOOST_CHECK(VP(thirdMove.resultedState())->leftBank().empty());

    a.pop();
    BOOST_CHECK( ! a.isSolution());
    BOOST_CHECK(a.length() == 2ULL);
    BOOST_CHECK(a.distToSolution() == 2ULL);
    BOOST_CHECK(a.initialState() != nullptr);
    BOOST_CHECK(a.targetLeftBank != nullptr);
    BOOST_CHECK_THROW(a.move(a.length()),
                      out_of_range); // right beyond last valid index

    a.pop();
    BOOST_CHECK( ! a.isSolution());
    BOOST_CHECK(a.length() == 1ULL);
    BOOST_CHECK(a.distToSolution() == 1ULL);
    BOOST_CHECK(a.initialState() != nullptr);
    BOOST_CHECK(a.targetLeftBank != nullptr);
    BOOST_CHECK_THROW(a.move(a.length()),
                      out_of_range); // right beyond last valid index

    a.pop();
    BOOST_CHECK( ! a.isSolution());
    BOOST_CHECK(a.length() == 0ULL);
    BOOST_CHECK(a.distToSolution() == 3ULL);
    BOOST_CHECK(a.initialState() != nullptr);
    BOOST_CHECK(a.targetLeftBank != nullptr);
    BOOST_CHECK_THROW(a.move(a.length()),
                      out_of_range); // right beyond last valid index

    a.pop();
    BOOST_CHECK( ! a.isSolution());
    BOOST_CHECK(a.length() == 0ULL);
    BOOST_CHECK(a.distToSolution() == SIZE_MAX);
    BOOST_CHECK(a.initialState() == nullptr);
    BOOST_CHECK(a.targetLeftBank == nullptr);
    BOOST_CHECK_THROW(a.move(a.length()),
                      out_of_range); // right beyond last valid index

    // further pop operations don't get affected
    a.pop();
    BOOST_CHECK( ! a.isSolution());
    BOOST_CHECK(a.length() == 0ULL);
    BOOST_CHECK(a.distToSolution() == SIZE_MAX);
    BOOST_CHECK(a.initialState() == nullptr);
    BOOST_CHECK(a.targetLeftBank == nullptr);
    BOOST_CHECK_THROW(a.move(a.length()),
                      out_of_range); // right beyond last valid index
  });

  BOOST_CHECK_NO_THROW({ // checking `clear`
    MovingEntities moved(ae);

    // The move creating the initial state
    a.append({moved, // empty right now
             d.createInitialState(InitialSymbolsTable()),
             UINT_MAX}); // doesn't need a meaningful index

    // First move
    moved = vector<unsigned>({1U, 2U});
    a.append({moved,
             VP(a.initSt)->next(moved),
             0U});
    const IMove &firstMove = a.move(0ULL);

    // Second move
    moved = vector<unsigned>({1U});
    a.append({moved,
             VP(firstMove.resultedState())->next(moved),
             1U});
    const IMove &secondMove = a.move(1ULL);

    // Third move
    moved = vector<unsigned>({1U, 3U});
    a.append({moved,
             VP(secondMove.resultedState())->next(moved),
             2U});

    BOOST_CHECK(a.isSolution());

    a.clear();

    BOOST_CHECK( ! a.isSolution());
    BOOST_CHECK(a.length() == 0ULL);
    BOOST_CHECK(a.distToSolution() == SIZE_MAX);
    BOOST_CHECK(a.initialState() == nullptr);
    BOOST_CHECK(a.targetLeftBank == nullptr);
    BOOST_CHECK_THROW(a.move(a.length()),
                      out_of_range); // right beyond last valid index
  });
}

BOOST_AUTO_TEST_CASE(solvingVariousScenarios) {
  AllEntities *pAe = new AllEntities;
  shared_ptr<const AllEntities> ae = shared_ptr<const AllEntities>(pAe);

  BOOST_REQUIRE_NO_THROW({
    *pAe += make_shared<const Entity>(1U, "a", "", false, "true", 1.);
    *pAe += make_shared<const Entity>(2U, "b", "", false, "false", 2.);
    *pAe += make_shared<const Entity>(3U, "c", "", false, "false", 3.);
  });

  Scenario::Results o;
  Scenario::Details d;
  d.entities = ae;
  d._capacity = 2U;

  // Default transfer constraints
  d._transferConstraints = make_unique<const TransferConstraints>(
            grammar::ConstraintsVec{}, d.entities,
            d._capacity, d._maxLoad,
            false);

  BOOST_CHECK_NO_THROW({
    assert(d._capacity == 2U);
    assert(d._maxLoad == DBL_MAX);
    Solver s(d, o);
    s.run();
    shared_ptr<const IAttempt> &sol = o.attempt;
    BOOST_REQUIRE(nullptr != sol);
    BOOST_CHECK(sol->isSolution());
    bool b = false;
    BOOST_CHECK(b = (sol->length() == 3ULL));
    if(b) {
      const IMove &firstMove = sol->move(0ULL);
      const IMove &secondMove = sol->move(1ULL);
      const IMove &thirdMove = sol->move(2ULL);
      BOOST_CHECK(firstMove.movedEntities() == set<unsigned>({1U, 2U}) ||
                  firstMove.movedEntities() == set<unsigned>({1U, 3U}));
      BOOST_CHECK(secondMove.movedEntities() == set<unsigned>({1U}));
      BOOST_CHECK(thirdMove.movedEntities() == set<unsigned>({1U, 2U}) ||
                  thirdMove.movedEntities() == set<unsigned>({1U, 3U}));
    }
  });

  d._maxLoad = 3.;

  // Entity 3 doesn't row and the raft supports only its weight => no solution
  BOOST_CHECK_NO_THROW({
    assert(d._capacity == 2U);
    assert(d._maxLoad == 3.);
    Solver s(d, o);
    s.run();
    shared_ptr<const IAttempt> &sol = o.attempt;
    BOOST_REQUIRE(nullptr != sol);
    BOOST_CHECK( ! sol->isSolution());

    // abc - empty (initial state not counted), then c - ab, then ac - b
    BOOST_CHECK(o.longestInvestigatedPath == 2ULL);

    // abc - empty ; bc - a ; c - ab ; ac - b
    BOOST_CHECK(o.investigatedStates == 4ULL);

    // c - ab
    BOOST_CHECK(o.closestToTargetLeftBank.size() == 1ULL);
  });
}

BOOST_AUTO_TEST_SUITE_END()

#endif // for SOLVER_CPP and UNIT_TESTING
