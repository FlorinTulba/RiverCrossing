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
    cout<<"IEntities {[ ";
    for(const IfEntities *retCfg : returnedConfigs)
      copy(CBOUNDS(retCfg->ids()), ostream_iterator<unsigned>(cout, " "));
    cout<<"]} != ExpectedEntities {[ ";
    for(const set<unsigned> &cfg : expectedConfigs)
      copy(CBOUNDS(cfg), ostream_iterator<unsigned>(cout, " "));
    cout<<"]}"<<endl;
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

      MovingConfigsManager mcm(sd, emptySt);

      // only 9 can row; 9 cannot appear alone;
      // 5&9 weigh 14 > max load; 1&9 weigh different than 12.
      vector<set<unsigned>> expectedConfigs({{3, 9}});
      mcm.configsForBank(be = vector<unsigned>({1U, 3U, 5U, 9U}), configsForABank,
                         false);
      BOOST_CHECK(configsForABank == expectedConfigs);

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

  MovingEntities moved(ae, vector<unsigned>({2U, 3U}));
  const unsigned t = 1234U, cap = 2U;
  const double maxLoad = 12.;
  IdsConstraint *pIc = new IdsConstraint;
  pIc->addMandatoryId(1U).addUnspecifiedMandatory(); // 1 *
  shared_ptr<const IConfigConstraint> ic =
    shared_ptr<const IdsConstraint>(pIc);
  grammar::ConfigurationsTransferDurationInitType ctdit;
  ctdit.setDuration(4321U).setConstraints({ic});
  vector<ConfigurationsTransferDuration> ctds({
    ConfigurationsTransferDuration(std::move(ctdit), ae, cap, maxLoad)});

  {
    BankEntities left5(left), right5(right);
    left5 -= moved; right5 += moved;
    State s(left, right, true, t), s1(s), s2(s), s3(s), s4(s),
          s5(left5, right5, true, t);
    s2._nextMoveFromLeft = ! s2._nextMoveFromLeft;
    ++s3._time;
    --s4._time;

    BOOST_CHECK(s.rightBank().empty());
    BOOST_CHECK(s.leftBank() == pAe->ids());
    BOOST_CHECK(s.nextMoveFromLeft());
    BOOST_CHECK(s.time() == t);
    BOOST_CHECK(s.handledBy(s1)); // same
    BOOST_CHECK( ! s.handledBy(s2)); // direction mismatch
    BOOST_CHECK( ! s.handledBy(s3)); // s3 occurred later, so s is preferred
    BOOST_CHECK(s.handledBy(s4)); // s4 occurred earlier than s
    BOOST_CHECK( ! s.handledBy(s5)); // different bank configs

    unique_ptr<const IState> nextS;
    bool b = false;
    BOOST_CHECK_NO_THROW({
      s.next(moved = vector<unsigned>({1U,9U})); // next state ignoring time
      nextS = s.next(moved = vector<unsigned>({1U,9U}), ctds);
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
        BOOST_CHECK(cloneNextS->time() == 5555U);
      }
    }

    BOOST_CHECK_THROW(s.next(moved = vector<unsigned>({2U}),
                             ctds), // covers only config '1 *'
                      domain_error); // no duration for config 2
  }

  {
    /// Used to check some parameter validity for other children of StatePlusSymbols
    struct DummyDecorator : public StatePlusSymbols {
      DummyDecorator(const shared_ptr<const IState> &decoratedState_) :
        StatePlusSymbols(decoratedState_, {}) {}

      string _toString() const override {return "";}
      unique_ptr<const IState> _clone(unique_ptr<const IState>) const override {
        return nullptr;
      }
      bool _handledBy(const IState&) const override {return false;}
      unique_ptr<const IState> _next(const MovingEntities&,
                                     unique_ptr<const IState>,
                                     const vector<ConfigurationsTransferDuration>&
                                        = {}) const override {return nullptr;}
    };

    shared_ptr<const IState> decoratedS =
      make_shared<const State>(left, right, true, t);

    BOOST_CHECK_THROW(PrevLoadStateDecorator(nullptr, InitialSymbolsTable()),
                      invalid_argument); // NULL decorated state

    BOOST_CHECK_THROW(PrevLoadStateDecorator(decoratedS, SymbolsTable()),
                      logic_error); // missing CrossingIndex entry

    // missing PreviousRaftLoad entry when CrossingIndex >= 2
    BOOST_CHECK_THROW(PrevLoadStateDecorator plsd(decoratedS,
                                        SymbolsTable({{"CrossingIndex", 2.}})),
                      logic_error);

    BOOST_CHECK_NO_THROW({ // CrossingIndex 0 or 1 and no PreviousRaftLoad is ok
      PrevLoadStateDecorator plsd1(decoratedS,
                                   InitialSymbolsTable()); // CrossingIndex 0
      const SymbolsTable &syms1 = plsd1.symbolsValues();
      bool b = false;
      BOOST_CHECK(b = (syms1.find("PreviousRaftLoad") != cend(syms1)));
      if(b)
        BOOST_CHECK(isNaN(syms1.at("PreviousRaftLoad")));

      PrevLoadStateDecorator plsd2(decoratedS,
                                   SymbolsTable({{"CrossingIndex", 1.}}));
      const SymbolsTable &syms2 = plsd2.symbolsValues();
      b = false;
      BOOST_CHECK(b = (syms2.find("PreviousRaftLoad") != cend(syms2)));
      if(b)
        BOOST_CHECK(isNaN(syms2.at("PreviousRaftLoad")));

      PrevLoadStateDecorator plsd3(decoratedS,
                                   SymbolsTable({{"CrossingIndex", 2.},
                                                {"PreviousRaftLoad", 10.}}));
      const SymbolsTable &syms3 = plsd3.symbolsValues();
      b = false;
      BOOST_CHECK(b = (syms3.find("PreviousRaftLoad") != cend(syms3)));
      if(b)
        BOOST_TEST(syms3.at("PreviousRaftLoad") == 10.);

      BOOST_CHECK(plsd3.handledBy(plsd1)); // CrossingIndex 0/1 handles all
      BOOST_CHECK(plsd3.handledBy(plsd2)); // CrossingIndex 0/1 handles all

      // decoratedS isn't derived from StatePlusSymbols
      BOOST_CHECK_THROW(plsd3.handledBy(*decoratedS),
                        invalid_argument);

      // PreviousRaftLoad is missing from DummyDecorator's SymbolsTable
      BOOST_CHECK_THROW(plsd3.handledBy(DummyDecorator(decoratedS)),
                        invalid_argument);

      PrevLoadStateDecorator plsd4(decoratedS,
                                   SymbolsTable({{"CrossingIndex", 15.},
                                                {"PreviousRaftLoad", 10.}}));
      BOOST_CHECK(plsd4.handledBy(plsd1)); // CrossingIndex 0/1 handles all
      BOOST_CHECK(plsd4.handledBy(plsd2)); // CrossingIndex 0/1 handles all
      BOOST_CHECK(plsd4.handledBy(plsd3)); // same PreviousRaftLoad
      BOOST_CHECK(plsd3.handledBy(plsd4)); // same PreviousRaftLoad

      PrevLoadStateDecorator plsd5(decoratedS,
                                   SymbolsTable({{"CrossingIndex", 13.},
                                                {"PreviousRaftLoad", 18.}}));
      BOOST_CHECK(plsd5.handledBy(plsd1)); // CrossingIndex 0/1 handles all
      BOOST_CHECK(plsd5.handledBy(plsd2)); // CrossingIndex 0/1 handles all
      BOOST_CHECK( ! plsd5.handledBy(plsd3)); // different PreviousRaftLoad
      BOOST_CHECK( ! plsd5.handledBy(plsd4)); // different PreviousRaftLoad

      unique_ptr<const IState> cloneOfPlsd5 = plsd5.clone();
      const PrevLoadStateDecorator *pCloneOfPlsd5 =
        dynamic_cast<const PrevLoadStateDecorator*>(cloneOfPlsd5.get());
      b = false;
      BOOST_CHECK(b = (nullptr != pCloneOfPlsd5));
      if(b) {
        BOOST_CHECK(pCloneOfPlsd5->rightBank().empty());
        BOOST_CHECK(pCloneOfPlsd5->leftBank().count() == entsCount);
        BOOST_CHECK(pCloneOfPlsd5->nextMoveFromLeft());
        BOOST_CHECK(pCloneOfPlsd5->time() == t);

        const SymbolsTable &symsClone = pCloneOfPlsd5->symbolsValues();
        b = false;
        BOOST_CHECK(b = (cend(symsClone) != symsClone.find("CrossingIndex")));
        if(b)
          BOOST_TEST(symsClone.at("CrossingIndex") == 13.);

        b = false;
        BOOST_CHECK(b = (cend(symsClone) != symsClone.find("PreviousRaftLoad")));
        if(b)
          BOOST_TEST(symsClone.at("PreviousRaftLoad") == 18.);
      }

      unique_ptr<const IState> nextOfPlsd5 =
        plsd5.next(moved = vector<unsigned>({1U, 9U})); // weight 10; ignoring time
      const PrevLoadStateDecorator *pNextOfPlsd5 =
        dynamic_cast<const PrevLoadStateDecorator*>(nextOfPlsd5.get());
      b = false;
      BOOST_CHECK(b = (nullptr != pNextOfPlsd5));
      if(b) {
        BOOST_CHECK(pNextOfPlsd5->rightBank() == set<unsigned>({1U,9U}));
        BOOST_CHECK(pNextOfPlsd5->leftBank().count() == entsCount - 2ULL);
        BOOST_CHECK( ! pNextOfPlsd5->nextMoveFromLeft());
        BOOST_CHECK(pNextOfPlsd5->time() == t);

        const SymbolsTable &symsNext = pNextOfPlsd5->symbolsValues();
        b = false;
        BOOST_CHECK(b = (cend(symsNext) != symsNext.find("CrossingIndex")));
        if(b)
          BOOST_TEST(symsNext.at("CrossingIndex") == 14.);

        b = false;
        BOOST_CHECK(b = (cend(symsNext) != symsNext.find("PreviousRaftLoad")));
        if(b)
          BOOST_TEST(symsNext.at("PreviousRaftLoad") == 10.);
      }
    });
  }

  {
    Scenario::Details d;
    d.entities = ae;
    d.allowedLoads = nullptr;

    BOOST_CHECK_NO_THROW({ // Creates State, since allowedLoads is NULL
      unique_ptr<const IState> initS = d.createInitialState(InitialSymbolsTable());
      BOOST_CHECK(nullptr != dynamic_cast<const State*>(initS.get()));
      BOOST_CHECK(nullptr == dynamic_cast<const StatePlusSymbols*>(initS.get()));
    });

    ValueSet *pVs = new ValueSet;
    d.allowedLoads = shared_ptr<const ValueSet>(pVs);

    // Creates State, since allowedLoads doesn't depend on PreviousRaftLoad
    BOOST_CHECK_NO_THROW({
      unique_ptr<const IState> initS = d.createInitialState(InitialSymbolsTable());
      BOOST_CHECK(nullptr != dynamic_cast<const State*>(initS.get()));
      BOOST_CHECK(nullptr == dynamic_cast<const StatePlusSymbols*>(initS.get()));
    });

    pVs->add(ValueOrRange(make_shared<const NumericVariable>("PreviousRaftLoad")));

    // Creates PrevLoadStateDecorator, since allowedLoads depends on PreviousRaftLoad
    BOOST_CHECK_NO_THROW({
      unique_ptr<const IState> initS = d.createInitialState(InitialSymbolsTable());
      BOOST_CHECK(nullptr != dynamic_cast<const PrevLoadStateDecorator*>(initS.get()));
      BOOST_CHECK(nullptr != dynamic_cast<const StatePlusSymbols*>(initS.get()));
    });
  }
  // AllowedLoads depending on PreviousRaftLoad to createInitialState
  //  PrevLoadStateDecorator<const State>
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
