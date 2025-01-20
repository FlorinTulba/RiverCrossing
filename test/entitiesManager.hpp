/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#if !defined CPP_ENTITIES_MANAGER || !defined UNIT_TESTING

#error \
    "Please include this file only within `entitiesManager.cpp` \
after a `#define CPP_ENTITIES_MANAGER` and surrounding the include \
and the define by `#ifdef UNIT_TESTING`!"

#else  // for CPP_ENTITIES_MANAGER and UNIT_TESTING

#include "entity.h"
#include "mathRelated.h"
#include "transferredLoadExt.h"

#include <tuple>

#include <boost/property_tree/json_parser.hpp>
#include <boost/test/unit_test.hpp>

using std::ignore;

BOOST_AUTO_TEST_SUITE(entitiesManager, *boost::unit_test::tolerance(rc::Eps))

BOOST_AUTO_TEST_CASE(jsonEntitiesValidation) {
  using namespace std;
  using namespace rc;
  using namespace rc::ent;
  using namespace boost::property_tree;

  ptree entsSection, entsTree;
  istringstream iss;

  // entities should be an array of Entity elements
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : 10}
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // entities should be an array of Entity elements
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : "abc"}
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // entities should be an array of Entity elements
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : {"a":5}}
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // zero entities - not allowed
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : []}
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // 1 entity is still not ok (trivial scenario)
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Name": "Athlete"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // 2 entities is still not ok (trivial scenario)
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "CanRow": "true",
          "Name": "Coach"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // duplicate entity id isn't allowed
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 0,
          "CanRow": "true",
          "Name": "Coach"},
          {"Id": 2,
          "CanRow": "true",
          "Name": "Fan"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // duplicate entity name isn't allowed
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 2,
          "CanRow": "true",
          "Name": "Fan"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // one entity has type information, the others don't - not allowed
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Type" : "male",
          "Name": "Athlete"},
          {"Id": 1,
          "CanRow": "true",
          "Name": "Coach"},
          {"Id": 2,
          "CanRow": "true",
          "Name": "Fan"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // all entities lack type / weight information - ok
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "CanRow": "true",
          "Name": "Coach"},
          {"Id": 2,
          "CanRow": "true",
          "Name": "Fan"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  try {
    const AllEntities ae{entsTree};
    BOOST_CHECK(!ae.empty());
    BOOST_CHECK(ae.count() == 3ULL);
    BOOST_CHECK(ae == set({0U, 1U, 2U}));
    BOOST_CHECK(ae != set({0U, 1U, 2U, 3U}));
    BOOST_CHECK(!(ae != set{0U, 1U, 2U}));
    BOOST_CHECK(size(ae.idsByTypes()) == 1U);
    BOOST_CHECK(size(ae.idsByWeights()) == 1U);
    BOOST_CHECK(size(ae.idsStartingFromLeftBank()) == 3ULL);
    BOOST_CHECK(ae.idsStartingFromRightBank().empty());
    BOOST_CHECK_THROW(ignore = ae[3U], out_of_range);
    BOOST_CHECK(ae[2U]);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // there is only one mentioned type - not allowed
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "Type" : "male",
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "Type" : "male",
          "CanRow": "true",
          "Name": "Coach"},
          {"Id": 2,
          "Type" : "male",
          "CanRow": "true",
          "Name": "Fan"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // all entities have type information and there are at least 2 such types - ok
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "Type" : "male",
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "Type" : "female",
          "CanRow": "true",
          "Name": "Coach"},
          {"Id": 2,
          "Type" : "male",
          "CanRow": "true",
          "Name": "Fan"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  try {
    const AllEntities ae{entsTree};
    BOOST_CHECK(!ae.empty());
    BOOST_CHECK(ae.count() == 3ULL);
    BOOST_CHECK(ae == set({0U, 1U, 2U}));
    BOOST_CHECK(ae != set({0U, 1U, 2U, 3U}));
    BOOST_CHECK(!(ae != set{0U, 1U, 2U}));
    BOOST_CHECK(size(ae.idsByTypes()) == 2U);
    BOOST_CHECK(size(ae.idsByWeights()) == 1U);
    BOOST_CHECK(size(ae.idsStartingFromLeftBank()) == 3ULL);
    BOOST_CHECK(ae.idsStartingFromRightBank().empty());
    BOOST_CHECK_THROW(ignore = ae[3U], out_of_range);
    BOOST_CHECK(ae[2U]);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // one entity has weight information, the others don't - not allowed
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "Weight" : 60,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "CanRow": "true",
          "Name": "Coach"},
          {"Id": 2,
          "CanRow": "true",
          "Name": "Fan"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // all entities have weight information - ok
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "Weight" : 60,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "Weight" : 65,
          "CanRow": "true",
          "Name": "Coach"},
          {"Id": 2,
          "Weight" : 70,
          "CanRow": "true",
          "Name": "Fan"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  try {
    const AllEntities ae{entsTree};
    BOOST_CHECK(!ae.empty());
    BOOST_CHECK(ae.count() == 3ULL);
    BOOST_CHECK(ae == set({0U, 1U, 2U}));
    BOOST_CHECK(ae != set({0U, 1U, 2U, 3U}));
    BOOST_CHECK(!(ae != set{0U, 1U, 2U}));
    BOOST_CHECK(size(ae.idsByTypes()) == 1U);
    BOOST_CHECK(size(ae.idsByWeights()) == 3U);
    BOOST_CHECK(size(ae.idsStartingFromLeftBank()) == 3ULL);
    BOOST_CHECK(ae.idsStartingFromRightBank().empty());
    BOOST_CHECK_THROW(ignore = ae[3U], out_of_range);
    BOOST_CHECK(ae[2U]);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // All entities starting from the right bank - not ok (raft is on the left)
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "StartsFromRightBank" : true,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "StartsFromRightBank" : true,
          "CanRow": "true",
          "Name": "Coach"},
          {"Id": 2,
          "StartsFromRightBank" : true,
          "CanRow": "true",
          "Name": "Fan"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // The single entity starting from the left bank cannot row - not ok
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "StartsFromRightBank" : true,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "StartsFromRightBank" : true,
          "CanRow": "true",
          "Name": "Coach"},
          {"Id": 2,
          "Name": "Fan"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // At least 1 entity starts from the left bank and can row - ok
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : [
          {"Id": 0,
          "StartsFromRightBank" : true,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "StartsFromRightBank" : true,
          "CanRow": "true",
          "Name": "Coach"},
          {"Id": 2,
          "CanRow": "true",
          "Name": "Fan"}
          ]
        }
      )"),
               iss),
              entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  try {
    const AllEntities ae{entsTree};
    BOOST_CHECK(!ae.empty());
    BOOST_CHECK(ae.count() == 3ULL);
    BOOST_CHECK(ae == set({0U, 1U, 2U}));
    BOOST_CHECK(ae != set({0U, 1U, 2U, 3U}));
    BOOST_CHECK(!(ae != set{0U, 1U, 2U}));
    BOOST_CHECK(size(ae.idsByTypes()) == 1U);
    BOOST_CHECK(size(ae.idsByWeights()) == 1U);
    BOOST_CHECK(size(ae.idsStartingFromLeftBank()) == 1ULL);
    BOOST_CHECK(size(ae.idsStartingFromRightBank()) == 2ULL);
    BOOST_CHECK_THROW(ignore = ae[3U], out_of_range);
    BOOST_CHECK(ae[2U]);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(movingEntities_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::ent;

  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  shared_ptr<const AllEntities> spAe{
      shared_ptr<const AllEntities>(pAe.release())},
      aeForeign{make_shared<const AllEntities>()};

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

  try {
    MovingEntities me{spAe, {}, make_unique<TotalLoadExt>(spAe)};
    MovingEntities me1{aeForeign, {}, make_unique<TotalLoadExt>(spAe)};
    BOOST_CHECK_THROW(me = MovingEntities(aeForeign), logic_error);
    BOOST_CHECK_THROW(me = me1, logic_error);
    BOOST_CHECK_THROW(me += 10U, out_of_range);  // no entity with id 10
    BOOST_CHECK(me.empty());
    const TotalLoadExt* meLoadExt{std::move(
        AbsMovingEntitiesExt::selectExt<TotalLoadExt>(me.getExtension()))};
    bool b{};
    BOOST_CHECK(b = (bool)meLoadExt);
    if (b)
      BOOST_TEST(!meLoadExt->totalLoad());

    me += 1U;
    BOOST_CHECK(me == set{1U});
    BOOST_CHECK(me != set({1U, 2U}));
    BOOST_CHECK(!(me != set{1U}));
    BOOST_CHECK(!me.empty());
    BOOST_CHECK(me.count() == 1ULL);

    b = false;
    BOOST_CHECK(b = (bool)meLoadExt);
    if (b)
      BOOST_TEST(meLoadExt->totalLoad() == 1.);

    BOOST_CHECK_THROW(me += 1U, domain_error);  // duplicate id
    BOOST_CHECK_THROW(me -= 2U, domain_error);  // entity 2 wasn't in me

    me -= 1U;
    BOOST_CHECK(me.empty());
    BOOST_CHECK(!me.count());

    (me += 3U).clear();
    BOOST_CHECK(me.empty());

    b = false;
    BOOST_CHECK(b = (bool)meLoadExt);
    if (b)
      BOOST_TEST(!meLoadExt->totalLoad());

    me = {1U, 2U};
    set expectedIds{1U, 2U};
    BOOST_CHECK(me == expectedIds);
    BOOST_CHECK(me != set({1U, 2U, 3U}));
    BOOST_CHECK(!(me != expectedIds));
    BOOST_CHECK(ranges::equal(me.ids(), expectedIds));
    BOOST_CHECK(size(me.idsByTypes()) == 2ULL);  // t0 and t1
    BOOST_CHECK(!me.anyRowCapableEnts(
        InitialSymbolsTable()));  // CrossingIndex % 2 == 0

    b = false;
    BOOST_CHECK(b = (bool)meLoadExt);
    if (b)
      BOOST_TEST(meLoadExt->totalLoad() == 3.);

    me += 4U;  // Entity 4 rows for CrossingIndex % 2 == 0
    expectedIds = {1U, 2U, 4U};
    BOOST_CHECK(me == expectedIds);
    BOOST_CHECK(me != set({1U, 2U}));
    BOOST_CHECK(!(me != expectedIds));
    BOOST_CHECK(ranges::equal(me.ids(), expectedIds));
    BOOST_CHECK(size(me.idsByTypes()) == 3ULL);                // t0, t1 and t2
    BOOST_CHECK(me.anyRowCapableEnts(InitialSymbolsTable()));  // 4 can row
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(bankEntities_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::ent;

  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  auto spAe{shared_ptr<const AllEntities>(pAe.release())};
  auto aeForeign{make_shared<const AllEntities>()};

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

  MovingEntities me{spAe};

  try {
    BankEntities be{spAe};
    BankEntities be1{spAe};
    BankEntities beForeign{aeForeign};

    BOOST_CHECK_THROW(be = BankEntities{aeForeign}, logic_error);
    BOOST_CHECK_THROW(be = beForeign, logic_error);
    BOOST_CHECK_THROW(be += me = {10U},
                      out_of_range);  // no entity with id 10
    BOOST_CHECK(be.empty());
    BOOST_CHECK(be == set<unsigned>{});
    BOOST_CHECK(be != set({1U, 2U}));
    BOOST_CHECK(!(be != set<unsigned>{}));
    BOOST_CHECK(~be == spAe->ids());
    BOOST_CHECK(!be.differencesCount(be));
    BOOST_CHECK(!be.differencesCount(be1));

    be += me = {1U};
    BOOST_CHECK(be == set{1U});
    BOOST_CHECK(be != set({1U, 2U}));
    BOOST_CHECK(!(be != set{1U}));
    BOOST_CHECK(!be.empty());
    BOOST_CHECK(be.count() == 1ULL);
    BOOST_CHECK((~be).count() == entsCount - 1ULL);
    BOOST_CHECK_THROW(be += me = {1U},
                      domain_error);  // duplicate id
    BOOST_CHECK_THROW(be -= me = {2U},
                      domain_error);  // entity 2 wasn't in be
    BOOST_CHECK(!be.differencesCount(be));
    BOOST_CHECK(be.differencesCount(be1) == 1ULL);

    be -= me = {1U};
    BOOST_CHECK(be.empty());
    BOOST_CHECK(!be.count());
    BOOST_CHECK(~be == spAe->ids());

    (be += me = {3U}).clear();
    BOOST_CHECK(be.empty());
    BOOST_CHECK(~be == spAe->ids());

    be = {1U, 2U};
    set expectedIds{1U, 2U};
    BOOST_CHECK(be == expectedIds);
    BOOST_CHECK(be != set({1U, 2U, 3U}));
    BOOST_CHECK(!(be != expectedIds));
    BOOST_CHECK((~be).count() == entsCount - 2ULL);
    BOOST_CHECK(ranges::equal(be.ids(), expectedIds));
    BOOST_CHECK(size(be.idsByTypes()) == 2ULL);  // t0 and t1
    BOOST_CHECK(!be.anyRowCapableEnts(
        InitialSymbolsTable()));  // CrossingIndex % 2 == 0
    BOOST_CHECK(!be.differencesCount(be));
    BOOST_CHECK(be.differencesCount(be1) == 2ULL);

    be += me = {4U};  // Entity 4 rows for CrossingIndex % 2 == 0
    expectedIds = {1U, 2U, 4U};
    BOOST_CHECK(be == expectedIds);
    BOOST_CHECK(be != set({1U, 2U}));
    BOOST_CHECK(!(be != expectedIds));
    BOOST_CHECK((~be).count() == entsCount - 3ULL);
    BOOST_CHECK(ranges::equal(be.ids(), expectedIds));
    BOOST_CHECK(size(be.idsByTypes()) == 3ULL);                // t0, t1 and t2
    BOOST_CHECK(be.anyRowCapableEnts(InitialSymbolsTable()));  // 4 can row
    BOOST_CHECK(!be.differencesCount(be));
    BOOST_CHECK(be.differencesCount(be1) == 3ULL);

    be1 += me = {4U, 5U, 6U};
    BOOST_CHECK(!be1.differencesCount(be1));
    BOOST_CHECK(be.differencesCount(be1) == 4ULL);  // 1,2,5,6
    BOOST_CHECK(be1.differencesCount(be) == 4ULL);  // 1,2,5,6
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_SUITE_END()

#endif  // for CPP_ENTITIES_MANAGER and UNIT_TESTING
