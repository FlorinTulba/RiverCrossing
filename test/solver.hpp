/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#if !defined CPP_SOLVER || !defined UNIT_TESTING

#error \
    "Please include this file only within `solver.cpp` \
after a `#define CPP_SOLVER` and surrounding the include \
and the define by `#ifdef UNIT_TESTING`!"

#else  // for CPP_SOLVER and UNIT_TESTING

#include "durationExt.h"
#include "entity.h"
#include "mathRelated.h"
#include "solverDetail.hpp"
#include "transferredLoadExt.h"
#include "util.h"

#include <cassert>
#include <climits>
#include <cmath>
#include <concepts>
#include <iterator>
#include <numeric>
#include <tuple>
#include <type_traits>

#include <gsl/pointers>

#include <boost/test/unit_test.hpp>

using std::ignore;

/// factorial function
template <std::integral T>
constexpr T factorial(T n) noexcept {
  return (n <= (T)1) ? 1 : n * factorial(n - (T)1);
}

/// Count of k-combinations of n elements
template <std::integral T>
constexpr T combsCount(T n, T k) noexcept {
  return factorial(n) / (factorial(k) * factorial(n - k));
}

/// Max count of investigated raft configurations based on the parameters
unsigned maxInvestigatedRaftConfigs(unsigned entsCount,
                                    unsigned countEntsWhoCantRow,
                                    unsigned raftCapacity) noexcept {
  assert(entsCount >= 3U);
  assert(countEntsWhoCantRow < entsCount);
  assert(raftCapacity >= 2U);
  assert(raftCapacity < entsCount);

  unsigned result{entsCount - countEntsWhoCantRow};
  for (unsigned i{1U}; i < raftCapacity; ++i)
    for (unsigned j{std::max(countEntsWhoCantRow, i)}; j < entsCount; ++j)
      result += combsCount(j, i);
  return result;
}

// Helper == between returnedConfigs and expectedConfigs (exact order matters)
template <class IfEntities>
  requires std::is_convertible_v<IfEntities*, rc::ent::IEntities*>
[[nodiscard]] bool operator==(
    const std::vector<const IfEntities*>& returnedConfigs,
    const std::vector<std::set<unsigned> >& expectedConfigs) {
  using namespace std;

  const bool result{ranges::equal(
      returnedConfigs, expectedConfigs,
      [](const rc::ent::IEntities* retCfg, const set<unsigned>& cfg) {
        assert(retCfg);
        const bool ret{*retCfg == cfg};
        if (!ret)
          cout << rc::ContView{retCfg->ids(), {"[ ", " ", " ]"}}
               << " != " << rc::ContView{cfg, {"[ ", " ", " ]\n"}};
        return ret;
      })};
  if (!result) {
    cout << "IEntities { ";
    for (const gsl::not_null<const IfEntities*> retCfg : returnedConfigs)
      cout << rc::ContView{retCfg->ids(), {"[ ", " ", " ] "}};
    cout << "} != ExpectedEntities { ";
    for (const set<unsigned>& cfg : expectedConfigs)
      cout << rc::ContView{cfg, {"[ ", " ", " ] "}};
    cout << "}" << endl;
  }
  return result;
}

template <class IfEntities>
  requires std::is_convertible_v<IfEntities*, rc::ent::IEntities*>
[[nodiscard]] bool operator==(
    const std::vector<std::set<unsigned> >& expectedConfigs,
    const std::vector<const IfEntities*>& returnedConfigs) noexcept {
  return returnedConfigs == expectedConfigs;
}

BOOST_AUTO_TEST_SUITE(solver, *boost::unit_test::tolerance(rc::Eps))

BOOST_AUTO_TEST_CASE(generateCombinations_usecases) {
  using namespace std;

  bool b{};
  vector<int> vals;
  vector<vector<int> > combs;

  BOOST_CHECK_THROW(generateCombinations(CBOUNDS(vals), -1LL, combs, {}),
                    logic_error);  // -1-combinations of the empty set - not ok

  BOOST_CHECK_THROW(generateCombinations(CBOUNDS(vals), 1LL, combs, {}),
                    logic_error);  // 1-combinations of the empty set - not ok

  // 0-combinations of the empty set produces a single empty combination
  try {
    generateCombinations(CBOUNDS(vals), 0LL, combs, {});
    BOOST_CHECK(b = (size(combs) == 1ULL));
    if (b)
      BOOST_CHECK(cbegin(combs)->empty());
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  constexpr size_t n{6};
  constexpr ptrdiff_t k{3};
  vals.resize(n);
  iota(BOUNDS(vals), 1);
  combs.clear();

  // 3-combinations of the 1..6 set produces a single empty combination
  try {
    generateCombinations(CBOUNDS(vals), k, combs, {});
    BOOST_CHECK(b = (size(combs) == combsCount(n, (size_t)k)));
    if (b) {
      // 1st combination is the set of 1st k values
      BOOST_CHECK(
          equal(CBOUNDS(*cbegin(combs)), cbegin(vals), next(cbegin(vals), k)));

      // last combination is the set of last k values
      BOOST_CHECK(
          equal(CBOUNDS(*crbegin(combs)), prev(cend(vals), k), cend(vals)));
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(contextValidation_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::cond;
  using namespace rc::ent;

  const SymbolsTable emptySt;
  auto pVs{make_unique<ValueSet>()};
  ValueSet& vs{*pVs};
  const shared_ptr<const IValues<double> > spVs{pVs.release()};
  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  shared_ptr<const AllEntities> spAe{pAe.release()};
  MovingEntities me{spAe};

  BOOST_CHECK_THROW(InitiallyNoPrevRaftLoadExcHandler inprleh(spVs),
                    invalid_argument);  // empty allowedLoads argument

  BOOST_CHECK_NO_THROW({
    // DefContextValidator validates anything
    BOOST_CHECK(DefContextValidator{}.validate(me, emptySt));

    // empty set has no entities who row
    BOOST_CHECK(!CanRowValidator{}.validate(me, emptySt));
  });

  try {  // Available: 2 x t0 + 2 x t1 + 2 x t2 + t3 + t4 + t5
    ae += make_shared<const Entity>(0U, "e0", "t0", false, "true", 1.);
    ae += make_shared<const Entity>(1U, "e1", "t0", false, "false", 2.);
    ae += make_shared<const Entity>(2U, "e2", "t1", false, "true", 3.);
    ae += make_shared<const Entity>(3U, "e3", "t1", false, "false", 4.);
    ae += make_shared<const Entity>(4U, "e4", "t2", false, "true", 5.);
    ae += make_shared<const Entity>(5U, "e5", "t2", false, "false", 6.);
    ae += make_shared<const Entity>(6U, "e6", "t3", false, "true", 7.);
    ae += make_shared<const Entity>(7U, "e7", "t4", false, "true", 8.);
    ae += make_shared<const Entity>(8U, "e8", "t5", false, "true", 9.);
  } catch (...) {
    BOOST_REQUIRE(false);  // Unexpected exception
  }

  BOOST_CHECK_NO_THROW({
    CanRowValidator crv;

    // Entities with id-s 1,3,5 don't row
    BOOST_CHECK(!crv.validate(me = {1U, 3U, 5U}, emptySt));

    // Entities with id-s 1,3,5 don't row, but 8 does
    BOOST_CHECK(crv.validate(me = {1U, 3U, 5U, 8U}, emptySt));
  });

  const shared_ptr<const DefContextValidator> defValid{
      make_shared<const DefContextValidator>()};
  constexpr double d{2.4}, d1{4.9}, d2{6.8};
  const string var{"a"};
  const SymbolsTable st{{var, d1}};  // a := d1
  const shared_ptr<const NumericExpr> c{make_shared<const NumericConst>(d)},
      c1{make_shared<const NumericConst>(d1)},
      c2{make_shared<const NumericConst>(d2)},
      v{make_shared<const NumericVariable>(var)};
  vs.add(ValueOrRange{c})  // hold value d   (2.4)
      .add({v, c2});       // hold range 'a' .. d2  (4.9 .. 6.8)

  MovingEntities me1{spAe, {}, make_unique<TotalLoadExt>(spAe)};

  BOOST_CHECK_THROW(
      ignore = AllowedLoadsValidator{spVs}.validate(me1 = {5U}, emptySt),
      out_of_range);  // cannot find 'a' in emptySt

  BOOST_CHECK_NO_THROW({
    InitiallyNoPrevRaftLoadExcHandler inprleh{spVs};  // vs depends only on 'a'

    BOOST_CHECK(!inprleh.dependsOnPreviousRaftLoad);

    BOOST_CHECK(boost::logic::indeterminate(inprleh.assess(me1, emptySt)));
  });

  // 'a' not found in emptySt.
  // Unfortunately, InitiallyNoPrevRaftLoadExcHandler expects
  // a missing `PreviousRaftLoad` instead.
  // Besides vs doesn't depend on `PreviousRaftLoad`
  BOOST_CHECK_THROW(
      ignore = AllowedLoadsValidator(
                   spVs,      // refers to 'a'
                   defValid,  // next is the default validator
                   make_shared<const InitiallyNoPrevRaftLoadExcHandler>(
                       spVs))  // doesn't depend on `PreviousRaftLoad`
                   .validate(me1 = {5U},
                             emptySt),  // doesn't contain 'a'
      out_of_range);  // missing var 'a' continues to propagate

  /**
  Tackles a caught exception for a missing variable from the SymbolsTable.

  When `luckyVarName` misses from SymbolsTable, it makes assess return
  `outcome`. Otherwise, assess will return indeterminate
  */
  class MissingVarExcHandler : public IValidatorExceptionHandler {
   public:
    MissingVarExcHandler(const string& luckyVarName_, bool outcome_)
        : luckyVarName(luckyVarName_), outcome(outcome_) {}
    ~MissingVarExcHandler() noexcept override = default;

    MissingVarExcHandler(const MissingVarExcHandler&) = delete;
    MissingVarExcHandler(MissingVarExcHandler&&) = delete;
    void operator=(const MissingVarExcHandler&) = delete;
    void operator=(MissingVarExcHandler&&) = delete;

    /**
    @return outcome if `luckyVarName` misses from SymbolsTable;
      otherwise indeterminate
    */
    [[nodiscard]] boost::logic::tribool assess(
        const MovingEntities& /*ents*/,
        const SymbolsTable& st) const noexcept override {
      if (!st.contains(luckyVarName))
        return outcome;
      return boost::logic::indeterminate;
    }

    string luckyVarName;
    bool outcome;
  };

  // 'a' not found in {{"b", 1000.}}.
  // Unfortunately, MissingVarExcHandler expects a missing 'b' instead.
  BOOST_CHECK_THROW(ignore = AllowedLoadsValidator(
                                 spVs,      // refers to 'a'
                                 defValid,  // next is the default validator
                                 make_shared<const MissingVarExcHandler>(
                                     "b",  // 'b' can be missing, but is present
                                     false))  // outcome if 'b' is missing
                                 // doesn't contain 'a', but contains 'b'
                                 .validate(me1 = {5U}, {{"b", 1000.}}),
                    out_of_range);  // missing var 'a' continues to propagate

  try {
    // 'a' not found in emptySet, but in that case
    // MissingVarExcHandler forces the `false` parameter as outcome
    BOOST_CHECK(
        !AllowedLoadsValidator(spVs,      // refers to 'a'
                               defValid,  // next is the default validator
                               make_shared<const MissingVarExcHandler>(
                                   var,     // 'a' can be missing
                                   false))  // outcome if 'a' is missing
             .validate(me1 = {5U},
                       emptySt));  // doesn't contain 'a'

    AllowedLoadsValidator alv{spVs};

    // Entity 6 weighs 7 - outside 4.9..6.8 and different from 2.4
    BOOST_CHECK(!alv.validate(me1 = {6U}, st));

    // Entity 5 weighs 6 - in 4.9..6.8
    BOOST_CHECK(alv.validate(me1 = {5U}, st));

    // Entity 1 & 3 weigh together 6 - in 4.9..6.8
    BOOST_CHECK(alv.validate(me1 = {1U, 3U}, st));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // combined CanRow & AllowedLoads validator
    AllowedLoadsValidator alcrv{spVs, make_shared<const CanRowValidator>()};

    // Entities 1,3,5 don't row
    // and weigh 12 - outside 4.9..6.8 and <> 2.4
    BOOST_CHECK(!alcrv.validate(me1 = {1U, 3U, 5U}, st));

    // Entities with id-s 1,3,5 don't row, but 8 does,
    // but their weight is 21 - outside 4.9..6.8 and <> 2.4
    BOOST_CHECK(!alcrv.validate(me1 = {1U, 3U, 5U, 8U}, st));

    // Entity 6 can row, but weighs 7 - outside 4.9..6.8 and <> 2.4
    BOOST_CHECK(!alcrv.validate(me1 = {6U}, st));

    // Entity 5 weighs 6 - in 4.9..6.8, however it doesn't row
    BOOST_CHECK(!alcrv.validate(me1 = {5U}, st));

    // Entity 1 & 3 weigh together 6 - in 4.9..6.8, however they don't row
    BOOST_CHECK(!alcrv.validate(me1 = {1U, 3U}, st));

    // Entity 4 weighs 5 - in 4.9..6.8 and it does row
    BOOST_CHECK(alcrv.validate(me1 = {4U}, st));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  vs.add(ValueOrRange{make_shared<const NumericVariable>("PreviousRaftLoad")});

  try {  // vs depends now also on 'PreviousRaftLoad'
    shared_ptr<const InitiallyNoPrevRaftLoadExcHandler> inprleh{
        make_shared<const InitiallyNoPrevRaftLoadExcHandler>(spVs)};

    BOOST_CHECK(inprleh->dependsOnPreviousRaftLoad);

    BOOST_CHECK((bool)inprleh->assess(me1, InitialSymbolsTable()));  // idx is 0
    BOOST_CHECK((bool)inprleh->assess(
        me1, {{"CrossingIndex", 1.}}));  // true for idx <=1

    BOOST_CHECK(boost::logic::indeterminate(inprleh->assess(
        me1, {{"CrossingIndex", 2.}})));  // indeterminate for idx >=2

    // indeterminate as well when PreviousRaftLoad appears
    BOOST_CHECK(boost::logic::indeterminate(
        inprleh->assess(me1, {{"PreviousRaftLoad", 15.}})));

    // indeterminate as well when both CrossingIndex and PreviousRaftLoad appear
    BOOST_CHECK(boost::logic::indeterminate(inprleh->assess(
        me1, {{"CrossingIndex", 1.}, {"PreviousRaftLoad", 15.}})));

    // indeterminate as well when both CrossingIndex and PreviousRaftLoad miss
    BOOST_CHECK(boost::logic::indeterminate(inprleh->assess(me1, emptySt)));

    AllowedLoadsValidator alv{spVs, defValid, inprleh};

    // Although 'a' is missing, validation succeeds due to inprleh
    BOOST_CHECK(alv.validate(  // no `PreviousRaftLoad`; `CrossingIndex` is 0
        me1 = {6U}, InitialSymbolsTable()));

    // Although 'a' is missing, validation succeeds due to inprleh
    BOOST_CHECK(alv.validate(  // no `PreviousRaftLoad`; `CrossingIndex` is 1
        me1 = {6U}, {{"CrossingIndex", 1.}}));

    // for the cases below the out_of_range due to missing 'a' propagates
    BOOST_CHECK_THROW(
        ignore = alv.validate(me1 = {6U}, {{"CrossingIndex", 2.}}),
        out_of_range);  // idx >= 2
    BOOST_CHECK_THROW(
        ignore = alv.validate(me1 = {6U}, {{"PreviousRaftLoad", 15.}}),
        out_of_range);  // PreviousRaftLoad appears
    BOOST_CHECK_THROW(
        ignore = alv.validate(
            me1 = {6U}, {{"CrossingIndex", 1.}, {"PreviousRaftLoad", 15.}}),
        out_of_range);  // both CrossingIndex and PreviousRaftLoad appear
    BOOST_CHECK_THROW(
        ignore = alv.validate(me1 = {6U}, emptySt),
        out_of_range);  // both CrossingIndex and PreviousRaftLoad miss
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(movingConfigOption_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::ent;
  using namespace rc::cond;

  const SymbolsTable emptySt;
  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  shared_ptr<const AllEntities> spAe{pAe.release()};

  try {
    ae += make_shared<const Entity>(0U, "e0", "t0", false, "true", 1.);
    ae += make_shared<const Entity>(1U, "e1", "t0", false, "false", 2.);
    ae += make_shared<const Entity>(2U, "e2", "t1", false, "true", 3.);
    ae += make_shared<const Entity>(3U, "e3", "t1", false, "false", 4.);
  } catch (...) {
    BOOST_REQUIRE(false);  // Unexpected exception
  }

  MovingEntities me{spAe};
  BankEntities be{spAe};

  try {
    BOOST_CHECK(
        MovingConfigOption{me}.validFor(be, emptySt));  // empty bank & raft

    BOOST_CHECK(MovingConfigOption{me}
                    // empty set for raft included in any bank
                    .validFor(be = {0U, 1U, 2U, 3U}, emptySt));

    BOOST_CHECK(MovingConfigOption(me = {1U, 2U})
                    // both are in
                    .validFor(be = {0U, 1U, 2U, 3U}, emptySt));

    BOOST_CHECK(!MovingConfigOption(me = {1U, 2U})
                     // 1U is missing
                     .validFor(be = {0U, 2U, 3U}, emptySt));

    BOOST_CHECK(!MovingConfigOption(me = {1U, 2U})
                     // 2U is missing
                     .validFor(be = {0U, 1U, 3U}, emptySt));

    BOOST_CHECK(MovingConfigOption(me = {1U, 2U},  // both are in
                                   make_shared<const CanRowValidator>())
                    // 2U rows
                    .validFor(be = {0U, 1U, 2U, 3U}, emptySt));

    BOOST_CHECK(!MovingConfigOption(me = {1U},  // 1U is in
                                    make_shared<const CanRowValidator>())
                     // 1U doesn't row
                     .validFor(be = {0U, 1U, 2U, 3U}, emptySt));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(movingConfigsManager_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::ent;
  using namespace rc::cond;

  const SymbolsTable emptySt;
  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  ScenarioDetails sd;
  sd.entities = shared_ptr<const AllEntities>(pAe.release());

  try {  // Using initially only entities who cannot row
    ae += make_shared<const Entity>(1U, "e1", "t0", false, "false", 1.);
    ae += make_shared<const Entity>(3U, "e3", "t1", false, "false", 3.);
    ae += make_shared<const Entity>(5U, "e5", "t2", false, "false", 5.);
    ae += make_shared<const Entity>(10U, "e10", "t0", false, "false", 10.);
    ae += make_shared<const Entity>(11U, "e11", "t0", false, "false", 11.);
  } catch (...) {
    BOOST_REQUIRE(false);  // Unexpected exception
  }

  const size_t entsCantRowCount{ae.count()};

  BOOST_CHECK_THROW(MovingConfigsManager(sd, emptySt),
                    logic_error);  // sd.transferConstraints is NULL

  // The default transfer constraints
  sd.transferConstraints = make_unique<const TransferConstraints>(
      grammar::ConstraintsVec{}, *sd.entities, sd.capacity, false);

  BankEntities be{sd.entities};

  BOOST_CHECK_THROW(
      MovingConfigsManager(sd, emptySt),
      domain_error);  // none of the entities is/will be able to row

  try {  // Adding several entities who all can / might row
    ae += make_shared<const Entity>(2U, "e2", "t1", false,
                                    "if (%CrossingIndex% mod 2) in {1}", 2.);
    ae += make_shared<const Entity>(4U, "e4", "t2", false,
                                    "if (%CrossingIndex% mod 2) in {0}", 4.);
    ae += make_shared<const Entity>(6U, "e6", "t3", false,
                                    "if (%CrossingIndex% mod 2) in {1}", 6.);
    ae += make_shared<const Entity>(7U, "e7", "t4", false,
                                    "if (%CrossingIndex% mod 2) in {0}", 7.);
    ae += make_shared<const Entity>(8U, "e8", "t5", false,
                                    "if (%CrossingIndex% mod 2) in {1}", 8.);
    ae += make_shared<const Entity>(9U, "e0", "t0", false, "true", 9.);
  } catch (...) {
    BOOST_REQUIRE(false);  // Unexpected exception
  }

  const size_t entsCount{ae.count()};

  BOOST_CHECK_THROW(MovingConfigsManager(sd, emptySt),
                    logic_error);  // capacity is UINT_MAX >= entsCount

  sd.capacity = (unsigned)entsCount - 1U;

  vector<const MovingEntities*> configsForABank;

  try {
    {
      assert(sd.capacity == (unsigned)entsCount - 1U);
      MovingConfigsManager mcm{sd, emptySt};

      const double investigatedRaftCombs{(double)maxInvestigatedRaftConfigs(
          (unsigned)entsCount, (unsigned)entsCantRowCount, sd.capacity)};

      /*
       About the expression from the test below:

       A: exp2(entsCount) - 1.
        All combinations of the entities
        minus the one including them all
       B: exp2(entsCantRowCount)
        All combinations including only the entities who can't row,
        or no entities

       Let's remark A guarantees the remaining combinations
       miss at least 1 entity, which is what sd.capacity specifies
      */
      BOOST_TEST(exp2(entsCount) - 1.          // A explained above
                     - exp2(entsCantRowCount)  // B explained above
                 == investigatedRaftCombs);
      BOOST_TEST(investigatedRaftCombs == (double)size(mcm.allConfigs));

      // None of 1,3,5 can row
      mcm.configsForBank(be = {1U, 3U, 5U}, configsForABank, true);
      BOOST_CHECK(configsForABank.empty());
      mcm.configsForBank(be, configsForABank, false);
      BOOST_CHECK(configsForABank.empty());

      // 9 can row
      vector<set<unsigned> > expectedConfigs{
          {9U}, {1U, 9U}, {3U, 9U}, {1U, 3U, 9U}};
      mcm.configsForBank(be = {1U, 3U, 9U}, configsForABank, false);
      BOOST_CHECK(configsForABank == expectedConfigs);
      const shared_ptr<const IContextValidator> validator{
          sd.createTransferValidator()};  // allows anything now
      bool b{};
      BOOST_CHECK(b = (bool)validator);
      if (b) {
        // allows even an unexpected config, where no entity can row
        BOOST_CHECK(validator->validate(
            ent::MovingEntities{sd.entities, {1U, 3U, 5U}}, emptySt));

        // allow confirmed configurations
        for (const ent::MovingEntities* cfg : configsForABank) {
          if (cfg)
            BOOST_TEST_CONTEXT("for raft cfg: `" << *cfg << '`') {
              BOOST_CHECK(validator->validate(*cfg, emptySt));
            }
        }
      }

      mcm.configsForBank(be, configsForABank, true);
      ranges::reverse(expectedConfigs);
      BOOST_CHECK(configsForABank == expectedConfigs);
    }

    {
      sd.capacity = 2U;
      MovingConfigsManager mcm{sd, emptySt};

      const double investigatedRaftCombs{(double)maxInvestigatedRaftConfigs(
          (unsigned)entsCount, (unsigned)entsCantRowCount, sd.capacity)};

      BOOST_TEST(investigatedRaftCombs == (double)size(mcm.allConfigs));

      // None of 1,3,5 can row
      mcm.configsForBank(be = {1U, 3U, 5U}, configsForABank, true);
      BOOST_CHECK(configsForABank.empty());
      mcm.configsForBank(be, configsForABank, false);
      BOOST_CHECK(configsForABank.empty());

      // 9 can row
      vector<set<unsigned> > expectedConfigs{{9U}, {1U, 9U}, {3U, 9U}};
      mcm.configsForBank(be = {1U, 3U, 9U}, configsForABank, false);
      BOOST_CHECK(configsForABank == expectedConfigs);

      mcm.configsForBank(be, configsForABank, true);
      ranges::reverse(expectedConfigs);
      BOOST_CHECK(configsForABank == expectedConfigs);
    }

    {
      assert(sd.capacity == 2U);
      sd.maxLoad = 13.;
      sd.createTransferConstraintsExt();  // keep it after setting maxLoad
      sd.transferConstraints = make_unique<const TransferConstraints>(
          grammar::ConstraintsVec{}, *sd.entities, sd.capacity, false,
          *sd.transferConstraintsExt);

      MovingConfigsManager mcm{sd, emptySt};

      const double maxInvestigatedRaftCombs{(double)maxInvestigatedRaftConfigs(
          (unsigned)entsCount, (unsigned)entsCantRowCount, sd.capacity)};

      BOOST_TEST(maxInvestigatedRaftCombs >= (double)size(mcm.allConfigs));

      // 9 can row; 5&9 weigh 14 > max load; The expected configs respect
      // maxLoad
      vector<set<unsigned> > expectedConfigs{{9U}};
      mcm.configsForBank(be = {5U, 9U}, configsForABank, true);
      BOOST_CHECK(configsForABank == expectedConfigs);
      mcm.configsForBank(be, configsForABank, false);
      BOOST_CHECK(configsForABank == expectedConfigs);

      // 9 can row; 5&9 weigh 14 > max load; The expected configs respect
      // maxLoad
      expectedConfigs =
          initializer_list<set<unsigned> >{{9U}, {1U, 9U}, {3U, 9U}};
      mcm.configsForBank(be = {1U, 3U, 5U, 9U}, configsForABank, false);
      BOOST_CHECK(configsForABank == expectedConfigs);

      mcm.configsForBank(be, configsForABank, true);
      ranges::reverse(expectedConfigs);
      BOOST_CHECK(configsForABank == expectedConfigs);
    }

    {
      assert(sd.capacity == 2U);
      assert(sd.maxLoad == 13.);

      auto pIc{make_unique<IdsConstraint>()};
      IdsConstraint& ic{*pIc};
      shared_ptr<const IdsConstraint> spIc{
          shared_ptr<const IdsConstraint>(pIc.release())};
      ic.addOptionalId(9U).addUnspecifiedMandatory();  // 9? *
      sd.createTransferConstraintsExt();  // keep it after setting maxLoad
      sd.transferConstraints = make_unique<const TransferConstraints>(
          grammar::ConstraintsVec{spIc}, *sd.entities, sd.capacity, true,
          *sd.transferConstraintsExt);
      MovingConfigsManager mcm{sd, emptySt};

      const double maxInvestigatedRaftCombs{(double)maxInvestigatedRaftConfigs(
          (unsigned)entsCount, (unsigned)entsCantRowCount, sd.capacity)};

      BOOST_TEST(maxInvestigatedRaftCombs >= (double)size(mcm.allConfigs));

      // 9 can row, 5 doesn't; 9 cannot appear alone; 5&9 weigh 14 > max load
      mcm.configsForBank(be = {5U, 9U}, configsForABank, true);
      BOOST_CHECK(configsForABank.empty());
      mcm.configsForBank(be, configsForABank, false);
      BOOST_CHECK(configsForABank.empty());

      // only 9 can row; 9 cannot appear alone; 5&9 weigh 14 > max load
      vector<set<unsigned> > expectedConfigs{{1U, 9U}, {3U, 9U}};
      mcm.configsForBank(be = {1U, 3U, 5U, 9U}, configsForABank, false);
      BOOST_CHECK(configsForABank == expectedConfigs);

      mcm.configsForBank(be, configsForABank, true);
      ranges::reverse(expectedConfigs);
      BOOST_CHECK(configsForABank == expectedConfigs);
    }

    {
      assert(sd.capacity == 2U);
      assert(sd.maxLoad == 13.);
      assert(sd.transferConstraints);  // 9? *

      ValueSet* pVs{new ValueSet};
      pVs->add(ValueOrRange{make_shared<const NumericConst>(12.)});
      sd.allowedLoads = shared_ptr<const ValueSet>(pVs);

      // CrossingIndex is 0, so 7 can row
      MovingConfigsManager mcm{sd, InitialSymbolsTable()};

      /*
       The config requires one entity besides the optional 9;
       1&3&5&11 don't row; 9 can row and 7 can row sometimes;
       5&9, 7&9, 9&11 and 7&11 weigh > max load;
       1&9 and 9 weigh different than 12;
       5&7 are 2 entities and don't contain 9
      */
      vector<set<unsigned> > expectedConfigs{{3U, 9U}};
      mcm.configsForBank(be = {1U, 3U, 5U, 7U, 9U, 11U}, configsForABank,
                         false);
      BOOST_CHECK(configsForABank == expectedConfigs);
      const shared_ptr<const IContextValidator> validator{
          sd.createTransferValidator()};  // allows anything weighing 12
      bool b{};
      BOOST_CHECK(b = (bool)validator);
      if (b) {
        // allows even an unexpected config weighing 12, where no entity can row
        BOOST_CHECK(validator->validate(
            ent::MovingEntities(sd.entities, {1U, 11U},
                                sd.createMovingEntitiesExt()),
            emptySt));

        // allows 5&7 weighing 12, which violates `9? *` constraint
        BOOST_CHECK(validator->validate(
            ent::MovingEntities(sd.entities, {5U, 7U},
                                sd.createMovingEntitiesExt()),
            InitialSymbolsTable()));

        // allow confirmed configurations
        for (const ent::MovingEntities* cfg : configsForABank) {
          if (cfg)
            BOOST_TEST_CONTEXT("for raft cfg: `" << *cfg << '`') {
              BOOST_CHECK(validator->validate(*cfg, emptySt));
            }
        }
      }

      mcm.configsForBank(be, configsForABank, true);
      BOOST_CHECK(configsForABank == expectedConfigs);
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(algorithmStates_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::ent;
  using namespace rc::cond;
  using namespace rc::sol;

  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  auto spAe{shared_ptr<const AllEntities>(pAe.release())};

  try {
    ae += make_shared<const Entity>(1U, "e1", "t0", false, "false", 1.);
    ae += make_shared<const Entity>(2U, "e2", "t1", false,
                                    "if (%CrossingIndex% mod 2) in {1}", 2.);
    ae += make_shared<const Entity>(3U, "e3", "t1", false, "false", 3.);
    ae += make_shared<const Entity>(4U, "e4", "t2", false,
                                    "if (%CrossingIndex% mod 2) in {0}", 4.);
    ae += make_shared<const Entity>(5U, "e5", "t2", false, "false", 5.);
    ae += make_shared<const Entity>(6U, "e6", "t3", false,
                                    "if (%CrossingIndex% mod 2) in {1}", 6.);
    ae += make_shared<const Entity>(7U, "e7", "t4", false,
                                    "if (%CrossingIndex% mod 2) in {0}", 7.);
    ae += make_shared<const Entity>(8U, "e8", "t5", false,
                                    "if (%CrossingIndex% mod 2) in {1}", 8.);
    ae += make_shared<const Entity>(9U, "e0", "t0", false, "true", 9.);
  } catch (...) {
    BOOST_REQUIRE(false);  // Unexpected exception
  }

  const size_t entsCount{ae.count()};

  BankEntities right{spAe}, left{~right};
  BOOST_CHECK_THROW(State(left, left, true),
                    invalid_argument);  // non-complementary bank configs

  constexpr unsigned t{1234U}, cap{2U};
  constexpr double maxLoad{12.};
  auto pIc{make_unique<IdsConstraint>()};
  IdsConstraint& ic{*pIc};
  shared_ptr<const IdsConstraint> spIc{
      shared_ptr<const IdsConstraint>(pIc.release())};
  ic.addMandatoryId(1U).addUnspecifiedMandatory();  // 1 *
  grammar::ConfigurationsTransferDurationInitType ctdit;
  ctdit.setDuration(4321U).setConstraints({spIc});

  ScenarioDetails info;
  info.entities = spAe;
  info.capacity = cap;
  info.maxLoad = maxLoad;
  info.createTransferConstraintsExt();  // keep it after setting maxLoad
  info.maxDuration = t * t;

  info.ctdItems.emplace_back(std::move(ctdit), *spAe, cap,
                             *info.transferConstraintsExt);

  MovingEntities moved{
      spAe, {2U, 3U}, make_unique<TotalLoadExt>(spAe, maxLoad)};

  {
    BankEntities left5{left}, right5{right};
    left5 -= moved;
    right5 += moved;

    BOOST_CHECK_THROW(State(left, left, true),
                      invalid_argument);  // non-complementary banks

    shared_ptr<const TimeStateExt> stateExt, stateExt3, stateExt4;

    try {
      stateExt = make_shared<const TimeStateExt>(t, info);
      stateExt3 = make_shared<const TimeStateExt>(t + 1U, info);
      stateExt4 = make_shared<const TimeStateExt>(t - 1U, info);
      State s{left, right, true, stateExt};
      State s1{s};
    } catch (...) {
      BOOST_REQUIRE(false);  // Unexpected exception
    }

    State s{left, right, true, stateExt}, s1{s}, s2{s},
        s3{left, right, true, stateExt3}, s4{left, right, true, stateExt4},
        s5{left5, right5, true, stateExt};
    s2._nextMoveFromLeft = !s2._nextMoveFromLeft;
    vector<unique_ptr<const IState> > someStates;
    someStates.emplace_back(s2.clone());
    someStates.emplace_back(s3.clone());
    someStates.emplace_back(s5.clone());

    try {
      shared_ptr<const TimeStateExt> timeExt{
          AbsStateExt::selectExt<TimeStateExt>(s.getExtension())};
      bool b{};
      BOOST_CHECK(b = (bool)timeExt);
      if (b)
        BOOST_CHECK(t == timeExt->time());
    } catch (...) {
      BOOST_CHECK(false);  // Unexpected exception
    }

    BOOST_CHECK_NO_THROW({
      shared_ptr<const PrevLoadStateExt> prevLoadExt{
          AbsStateExt::selectExt<PrevLoadStateExt>(s.getExtension())};
      BOOST_CHECK(!prevLoadExt);
    });

    BOOST_CHECK(s.rightBank().empty());
    BOOST_CHECK(s.leftBank() == ae.ids());
    BOOST_CHECK(s.nextMoveFromLeft());
    BOOST_CHECK(!s.handledBy(s2));  // direction mismatch
    BOOST_CHECK(!s.handledBy(s3));  // s3 occurred later, so s is preferred
    BOOST_CHECK(!s.handledBy(s5));  // different bank configs
    BOOST_CHECK(!s.handledBy(someStates));  // {s2, s3, s5}
    BOOST_CHECK(s.handledBy(s1));           // same
    BOOST_CHECK(s.handledBy(s4));           // s4 occurred earlier than s

    someStates.emplace_back(s4.clone());
    BOOST_CHECK(s.handledBy(someStates));  // {s2, s3, s5, s4}

    unique_ptr<const IState> nextS;
    bool b{};
    BOOST_CHECK_NO_THROW({
      nextS = s.next(moved = {1U, 9U});
      b = true;
    });
    if (b) {
      b = false;
      unique_ptr<const IState> cloneNextS;
      BOOST_CHECK_NO_THROW({
        cloneNextS = nextS->clone();
        b = true;
      });
      if (b) {
        BOOST_CHECK(cloneNextS->rightBank() == set({1U, 9U}));
        BOOST_CHECK(cloneNextS->leftBank().count() == entsCount - 2ULL);
        BOOST_CHECK(!cloneNextS->nextMoveFromLeft());
        try {
          shared_ptr<const TimeStateExt> timeExtCloneNextS{
              AbsStateExt::selectExt<TimeStateExt>(cloneNextS->getExtension())};
          bool b1{};
          BOOST_CHECK(b1 = (bool)timeExtCloneNextS);
          if (b1)
            BOOST_CHECK(5555U == timeExtCloneNextS->time());
        } catch (...) {
          BOOST_CHECK(false);  // Unexpected exception
        }
      }
    }

    // ctds covers only config '1 *'. There is no duration for config 2
    BOOST_CHECK_THROW(ignore = s.next(moved = {2U}), domain_error);
  }

  {
    shared_ptr<const IState> s{
        make_shared<const State>(left, right, true)};  // default extension

    shared_ptr<const TimeStateExt> timeExt{
        make_shared<const TimeStateExt>(t, info)};

    BOOST_CHECK_THROW(PrevLoadStateExt(SymbolsTable(), info),
                      logic_error);  // missing CrossingIndex entry

    // missing PreviousRaftLoad entry when CrossingIndex >= 2
    BOOST_CHECK_THROW(
        PrevLoadStateExt plse(SymbolsTable({{"CrossingIndex", 2.}}), info),
        logic_error);

    try {  // CrossingIndex 0 or 1 and no PreviousRaftLoad is ok
      PrevLoadStateExt plse1{InitialSymbolsTable(), info,
                             timeExt};  // CrossingIndex 0
      State s1{left, right, true, plse1.clone()};
      BOOST_REQUIRE(isnan(
          plse1.prevRaftLoad()));  // Fails if wrong compilation flags around
                                   // '-ffast-math' and NaN handling. See:
                                   // 'mathRelated.h'

      PrevLoadStateExt plse2{SymbolsTable{{{"CrossingIndex", 1.}}}, info,
                             timeExt};
      State s2{left, right, true, plse2.clone()};
      BOOST_CHECK(isnan(plse2.prevRaftLoad()));

      PrevLoadStateExt plse3{
          SymbolsTable{{{"CrossingIndex", 2.}, {"PreviousRaftLoad", 10.}}},
          info, timeExt};
      State s3{left, right, true, plse3.clone()};
      BOOST_TEST(plse3.prevRaftLoad() == 10.);

      BOOST_CHECK(s3.handledBy(s1));  // CrossingIndex 0/1 handles all
      BOOST_CHECK(s3.handledBy(s2));  // CrossingIndex 0/1 handles all

      // s has only the default extension
      // ignore = s3.handledBy(*s) - aborts from not_null

      PrevLoadStateExt plse4{
          SymbolsTable{{{"CrossingIndex", 15.}, {"PreviousRaftLoad", 10.}}},
          info, timeExt};
      State s4{left, right, true, plse4.clone()};
      BOOST_CHECK(s4.handledBy(s1));  // CrossingIndex 0/1 handles all
      BOOST_CHECK(s4.handledBy(s2));  // CrossingIndex 0/1 handles all
      BOOST_CHECK(s4.handledBy(s3));  // same PreviousRaftLoad
      BOOST_CHECK(s3.handledBy(s4));  // same PreviousRaftLoad

      PrevLoadStateExt plse5{
          SymbolsTable{{{"CrossingIndex", 13.}, {"PreviousRaftLoad", 18.}}},
          info, timeExt};
      State s5{left, right, true, plse5.clone()};
      BOOST_CHECK(s5.handledBy(s1));   // CrossingIndex 0/1 handles all
      BOOST_CHECK(s5.handledBy(s2));   // CrossingIndex 0/1 handles all
      BOOST_CHECK(!s5.handledBy(s3));  // different PreviousRaftLoad
      BOOST_CHECK(!s5.handledBy(s4));  // different PreviousRaftLoad

      unique_ptr<const IState> cloneOfS5{s5.clone()};
      BOOST_CHECK(cloneOfS5->rightBank().empty());
      BOOST_CHECK(cloneOfS5->leftBank().count() == entsCount);
      BOOST_CHECK(cloneOfS5->nextMoveFromLeft());

      shared_ptr<const PrevLoadStateExt> pCloneOfPlse5{
          AbsStateExt::selectExt<PrevLoadStateExt>(cloneOfS5->getExtension())};
      shared_ptr<const TimeStateExt> pCloneOfTse5{
          AbsStateExt::selectExt<TimeStateExt>(cloneOfS5->getExtension())};
      bool b{};
      bool b1{};
      BOOST_CHECK(b = (bool)pCloneOfPlse5);
      BOOST_CHECK(b1 = (bool)pCloneOfTse5);
      if (b && b1) {
        BOOST_CHECK(pCloneOfTse5->time() == t);

        BOOST_TEST(pCloneOfPlse5->crossingIdx() == 13U);
        BOOST_TEST(pCloneOfPlse5->prevRaftLoad() == 18.);
      }

      unique_ptr<const IState> nextOfS5{
          s5.next(moved = {1U, 9U})};  // weight 10
      BOOST_CHECK(nextOfS5->rightBank() == set({1U, 9U}));
      BOOST_CHECK(nextOfS5->leftBank().count() == entsCount - 2ULL);
      BOOST_CHECK(!nextOfS5->nextMoveFromLeft());

      shared_ptr<const PrevLoadStateExt> pNextOfPlse5{
          AbsStateExt::selectExt<PrevLoadStateExt>(nextOfS5->getExtension())};
      shared_ptr<const TimeStateExt> pNextOfTse5{
          AbsStateExt::selectExt<TimeStateExt>(nextOfS5->getExtension())};
      b = b1 = false;
      BOOST_CHECK(b = (bool)pNextOfPlse5);
      BOOST_CHECK(b1 = (bool)pNextOfTse5);
      if (b && b1) {
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
    } catch (...) {
      BOOST_CHECK(false);  // Unexpected exception
    }
  }

  {
    ScenarioDetails d;
    d.entities = spAe;
    d.allowedLoads = {};

    // Default extension (allowedLoads is NULL and maxDuration is UINT_MAX)
    try {
      unique_ptr<const IState> initS{
          d.createInitialState(InitialSymbolsTable())};

      bool b{};
      BOOST_CHECK(b = (bool)initS);
      if (b) {
        BOOST_CHECK(
            !AbsStateExt::selectExt<TimeStateExt>(initS->getExtension()));
        BOOST_CHECK(
            !AbsStateExt::selectExt<PrevLoadStateExt>(initS->getExtension()));
      }
    } catch (...) {
      BOOST_CHECK(false);  // Unexpected exception
    }

    ValueSet* pVs{new ValueSet};
    d.allowedLoads = shared_ptr<const ValueSet>(pVs);

    // Default extension (allowedLoads doesn't depend on PreviousRaftLoad;
    // maxDuration is UINT_MAX)
    try {
      unique_ptr<const IState> initS{
          d.createInitialState(InitialSymbolsTable())};

      bool b{};
      BOOST_CHECK(b = (bool)initS);
      if (b) {
        BOOST_CHECK(
            !AbsStateExt::selectExt<TimeStateExt>(initS->getExtension()));
        BOOST_CHECK(
            !AbsStateExt::selectExt<PrevLoadStateExt>(initS->getExtension()));
      }
    } catch (...) {
      BOOST_CHECK(false);  // Unexpected exception
    }

    pVs->add(
        ValueOrRange{make_shared<const NumericVariable>("PreviousRaftLoad")});

    // PrevLoadStateExt (allowedLoads depends on PreviousRaftLoad; maxDuration
    // is UINT_MAX)
    try {
      unique_ptr<const IState> initS{
          d.createInitialState(InitialSymbolsTable())};

      bool b{};
      BOOST_CHECK(b = (bool)initS);
      if (b) {
        BOOST_CHECK(
            !AbsStateExt::selectExt<TimeStateExt>(initS->getExtension()));
        BOOST_CHECK(
            AbsStateExt::selectExt<PrevLoadStateExt>(initS->getExtension()));
      }
    } catch (...) {
      BOOST_CHECK(false);  // Unexpected exception
    }

    d.maxDuration = t * t;

    // PrevLoadStateExt & TimeStateExt (allowedLoads depends on
    // PreviousRaftLoad; maxDuration < UINT_MAX)
    try {
      unique_ptr<const IState> initS{
          d.createInitialState(InitialSymbolsTable())};

      bool b{};
      BOOST_CHECK(b = (bool)initS);
      if (b) {
        BOOST_CHECK(
            AbsStateExt::selectExt<TimeStateExt>(initS->getExtension()));
        BOOST_CHECK(
            AbsStateExt::selectExt<PrevLoadStateExt>(initS->getExtension()));
      }
    } catch (...) {
      BOOST_CHECK(false);  // Unexpected exception
    }
  }
}

BOOST_AUTO_TEST_CASE(algorithmMove_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::ent;
  using namespace rc::cond;
  using namespace rc::sol;

  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  auto spAe{shared_ptr<const AllEntities>(pAe.release())};

  try {
    ae += make_shared<const Entity>(1U, "e1", "t0", false, "false", 1.);
    ae += make_shared<const Entity>(2U, "e2", "t1", false,
                                    "if (%CrossingIndex% mod 2) in {1}", 2.);
    ae += make_shared<const Entity>(3U, "e3", "t1", false, "false", 3.);
    ae += make_shared<const Entity>(4U, "e4", "t2", false,
                                    "if (%CrossingIndex% mod 2) in {0}", 4.);
  } catch (...) {
    BOOST_REQUIRE(false);  // Unexpected exception
  }

  BankEntities right{spAe}, left{~right};
  MovingEntities me{spAe, {2U, 3U}};
  unique_ptr<const IState> s{make_unique<const State>(left, right, true)},
      clonedS{s->clone()};

  BOOST_CHECK_THROW(
      Move m(me, make_unique<const State>(left, right, false), 0U),
      logic_error);  // Some moved entities missing from receiver bank

  try {
    Move m(me, std::move(s), 123U);
    BOOST_CHECK(123U == m.index());
    BOOST_CHECK(me == m.movedEntities());
    BOOST_CHECK(m.resultedState()->handledBy(*clonedS));
    BOOST_CHECK(clonedS->handledBy(*m.resultedState()));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(currentAttempt_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::ent;
  using namespace rc::cond;
  using namespace rc::sol;

  Attempt a;

  BOOST_CHECK_NO_THROW(a.pop());

  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  auto spAe{shared_ptr<const AllEntities>(pAe.release())};

  try {
    ae += make_shared<const Entity>(1U, "a", "", false, "true");
    ae += make_shared<const Entity>(2U, "b", "", false, "false");
    ae += make_shared<const Entity>(3U, "c", "", false, "false");
  } catch (...) {
    BOOST_REQUIRE(false);  // Unexpected exception
  }

  ScenarioDetails d;
  d.entities = spAe;
  d.capacity = 2U;

  BOOST_CHECK_THROW(
      a.append(Move(MovingEntities{spAe, {1U}},
                    d.createInitialState(InitialSymbolsTable()), UINT_MAX)),
      logic_error);  // MovingEntities for first append not empty

  BOOST_CHECK(!a.isSolution());
  BOOST_CHECK(!a.length());
  BOOST_CHECK(!a.initialState());
  BOOST_CHECK(!a.targetLeftBank);
  BOOST_CHECK_THROW(ignore = a.lastMove(),
                    out_of_range);  // no moves, nor the initial state yet

  try {
    MovingEntities moved{spAe};
    Attempt a1;

    // The move creating the initial state
    a1.append(Move(moved,  // empty right now
                   d.createInitialState(InitialSymbolsTable()),
                   UINT_MAX));  // doesn't need a meaningful index
    BOOST_CHECK(!a1.isSolution());
    BOOST_CHECK(!a1.length());
    BOOST_CHECK(a1.initialState());
    BOOST_CHECK(a1.targetLeftBank);
    const IMove& fakeMove = a1.lastMove();

    // First move
    moved = {1U, 2U};

    BOOST_CHECK_THROW(
        a1.append(
            Move(moved, CP(fakeMove.resultedState())->next(moved), 1234U)),
        logic_error);  // first move needs index 0, not 1234
    a1.append(Move(moved, CP(fakeMove.resultedState())->next(moved), 0U));
    BOOST_CHECK(!a1.isSolution());
    BOOST_CHECK(a1.length() == 1ULL);
    BOOST_CHECK(a1.initialState());
    BOOST_CHECK(a1.targetLeftBank);
    const IMove& firstMove{a1.lastMove()};

    // Second move
    moved = {1U};

    BOOST_CHECK_THROW(
        a1.append(
            Move(moved, CP(firstMove.resultedState())->next(moved), 1234U)),
        logic_error);  // second move needs index 1, not 1234
    a1.append(Move(moved, CP(firstMove.resultedState())->next(moved), 1U));
    BOOST_CHECK(!a1.isSolution());
    BOOST_CHECK(a1.length() == 2ULL);
    BOOST_CHECK(a1.initialState());
    BOOST_CHECK(a1.targetLeftBank);
    const IMove& secondMove = a1.lastMove();

    // Third move
    moved = {1U, 3U};

    BOOST_CHECK_THROW(
        a1.append(
            Move(moved, CP(secondMove.resultedState())->next(moved), 1234U)),
        logic_error);  // third move needs index 2, not 1234

    a1.append(Move(moved, CP(secondMove.resultedState())->next(moved), 2U));
    BOOST_CHECK(a1.isSolution());
    BOOST_CHECK(a1.length() == 3ULL);
    BOOST_CHECK(a1.initialState());
    BOOST_CHECK(a1.targetLeftBank);
    const IMove& thirdMove{a1.lastMove()};

    BOOST_CHECK(CP(thirdMove.resultedState())->leftBank().empty());

    a1.pop();
    BOOST_CHECK(!a1.isSolution());
    BOOST_CHECK(a1.length() == 2ULL);
    BOOST_CHECK(a1.initialState());
    BOOST_CHECK(a1.targetLeftBank);
    ignore = a1.lastMove();

    a1.pop();
    BOOST_CHECK(!a1.isSolution());
    BOOST_CHECK(a1.length() == 1ULL);
    BOOST_CHECK(a1.initialState());
    BOOST_CHECK(a1.targetLeftBank);
    ignore = a1.lastMove();

    a1.pop();
    BOOST_CHECK(!a1.isSolution());
    BOOST_CHECK(!a1.length());
    BOOST_CHECK(a1.initialState());
    BOOST_CHECK(a1.targetLeftBank);
    ignore = a1.lastMove();

    // There are already no more registered moves, however initFakeMove
    // remains
    a1.pop();
    BOOST_CHECK(!a1.isSolution());
    BOOST_CHECK(!a1.length());
    BOOST_CHECK(a1.initialState());
    BOOST_CHECK(a1.targetLeftBank);
    ignore = a1.lastMove();

    // Further pop operations don't change anything
    a1.pop();
    BOOST_CHECK(!a1.isSolution());
    BOOST_CHECK(!a1.length());
    BOOST_CHECK(a1.initialState());
    BOOST_CHECK(a1.targetLeftBank);
    ignore = a1.lastMove();
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // checking `clear`
    MovingEntities moved{spAe};
    Attempt a2;

    // The move creating the initial state
    a2.append(Move(moved,  // empty right now
                   d.createInitialState(InitialSymbolsTable()),
                   UINT_MAX));  // doesn't need a meaningful index
    const IMove& fakeMove{a2.lastMove()};

    // First move
    moved = {1U, 2U};

    a2.append(Move(moved, CP(fakeMove.resultedState())->next(moved), 0U));
    const IMove& firstMove{a2.lastMove()};

    // Second move
    moved = {1U};

    a2.append(Move(moved, CP(firstMove.resultedState())->next(moved), 1U));
    const IMove& secondMove{a2.lastMove()};

    // Third move
    moved = {1U, 3U};

    a2.append(Move(moved, CP(secondMove.resultedState())->next(moved), 2U));
    ignore = a2.lastMove();

    BOOST_CHECK(a2.isSolution());

    a2.clear();

    BOOST_CHECK(!a2.isSolution());
    BOOST_CHECK(!a2.length());
    BOOST_CHECK(a2.initialState());
    BOOST_CHECK(a2.targetLeftBank);
    ignore = a2.lastMove();
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(solvingVariousScenarios) {
  using namespace std;
  using namespace rc;
  using namespace rc::ent;
  using namespace rc::cond;
  using namespace rc::sol;

  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  auto spAe{shared_ptr<const AllEntities>(pAe.release())};

  try {
    ae += make_shared<const Entity>(1U, "a", "", false, "true", 1.);
    ae += make_shared<const Entity>(2U, "b", "", false, "false", 2.);
    ae += make_shared<const Entity>(3U, "c", "", false, "false", 3.);
  } catch (...) {
    BOOST_REQUIRE(false);  // Unexpected exception
  }

  ScenarioDetails d;
  d.entities = spAe;
  d.capacity = 2U;

  // Default transfer constraints
  d.transferConstraints = make_unique<const TransferConstraints>(
      grammar::ConstraintsVec{}, *d.entities, d.capacity, false);

  try {
    assert(d.capacity == 2U);
    assert(d.maxLoad == DBL_MAX);
    Scenario::Results oBfs;
    Solver s{d, oBfs};
    s.run(true);
    const shared_ptr<const IAttempt>& sol{oBfs.attempt};
    BOOST_REQUIRE(sol);
    BOOST_CHECK(sol->isSolution());
    bool b{};
    BOOST_CHECK(b = (sol->length() == 3ULL));
    if (b) {
      const IMove& firstMove{sol->move(0ULL)};
      const IMove& secondMove{sol->move(1ULL)};
      const IMove& thirdMove{sol->lastMove()};
      BOOST_CHECK(firstMove.movedEntities() == set({1U, 2U}) ||
                  firstMove.movedEntities() == set({1U, 3U}));
      BOOST_CHECK(secondMove.movedEntities() == set{1U});
      BOOST_CHECK(thirdMove.movedEntities() == set({1U, 2U}) ||
                  thirdMove.movedEntities() == set({1U, 3U}));
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {
    assert(d.capacity == 2U);
    assert(d.maxLoad == DBL_MAX);
    Scenario::Results oDfs;
    Solver s{d, oDfs};
    s.run(false);
    const shared_ptr<const IAttempt>& sol{oDfs.attempt};
    BOOST_REQUIRE(sol);
    BOOST_CHECK(sol->isSolution());
    bool b{};
    BOOST_CHECK(b = (sol->length() == 3ULL));
    if (b) {
      const IMove& firstMove{sol->move(0ULL)};
      const IMove& secondMove{sol->move(1ULL)};
      const IMove& thirdMove{sol->lastMove()};
      BOOST_CHECK(firstMove.movedEntities() == set({1U, 2U}) ||
                  firstMove.movedEntities() == set({1U, 3U}));
      BOOST_CHECK(secondMove.movedEntities() == set{1U});
      BOOST_CHECK(thirdMove.movedEntities() == set({1U, 2U}) ||
                  thirdMove.movedEntities() == set({1U, 3U}));
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  d.maxLoad = 3.;
  d.createTransferConstraintsExt();  // keep it after setting maxLoad
  d.transferConstraints = make_unique<const TransferConstraints>(
      grammar::ConstraintsVec{}, *d.entities, d.capacity, false,
      *d.transferConstraintsExt);

  // Entity 3 doesn't row and the raft supports only its weight => no solution
  try {
    assert(d.capacity == 2U);
    assert(d.maxLoad == 3.);
    Scenario::Results oBfs;
    Solver s{d, oBfs};
    s.run(true);
    const shared_ptr<const IAttempt>& sol{oBfs.attempt};
    BOOST_REQUIRE(sol);
    BOOST_CHECK(!sol->isSolution());

    // abc - empty (initial state not counted), then c - ab, then ac - b
    BOOST_CHECK(oBfs.longestInvestigatedPath == 2ULL);

    // abc - empty ; bc - a ; c - ab ; ac - b
    BOOST_CHECK(oBfs.investigatedStates == 4ULL);

    // c - ab
    BOOST_CHECK(size(oBfs.closestToTargetLeftBank) == 1ULL);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // Entity 3 doesn't row and the raft supports only its weight => no solution
  try {
    assert(d.capacity == 2U);
    assert(d.maxLoad == 3.);
    Scenario::Results oDfs;
    Solver s{d, oDfs};
    s.run(false);
    const shared_ptr<const IAttempt>& sol{oDfs.attempt};
    BOOST_REQUIRE(sol);
    BOOST_CHECK(!sol->isSolution());

    // abc - empty (initial state not counted), then c - ab, then ac - b
    BOOST_CHECK(oDfs.longestInvestigatedPath == 2ULL);

    // abc - empty ; bc - a ; c - ab ; ac - b
    BOOST_CHECK(oDfs.investigatedStates == 4ULL);

    // c - ab
    BOOST_CHECK(size(oDfs.closestToTargetLeftBank) == 1ULL);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_SUITE_END()

#endif  // for CPP_SOLVER and UNIT_TESTING
