/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 @2018 Florin Tulba (florintulba@yahoo.com)
*/

#if ! defined ENTITIES_MANAGER_CPP || ! defined UNIT_TESTING

  #error \
"Please include this file only at the end of `entitiesManager.cpp` \
after a `#define ENTITIES_MANAGER_CPP` and surrounding the include and the define \
by `#ifdef UNIT_TESTING`!"

#else // for ENTITIES_MANAGER_CPP and UNIT_TESTING

#include "../src/mathRelated.h"

#include <boost/test/unit_test.hpp>

#include <boost/property_tree/json_parser.hpp>

using namespace rc;
using namespace rc::ent;

BOOST_AUTO_TEST_SUITE(entitiesManager, *boost::unit_test::tolerance(Eps))

BOOST_AUTO_TEST_CASE(jsonEntitiesValidation) {
  ptree entsSection, entsTree;
  istringstream iss;

  // entities should be an array of Entity elements
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : 10}
      )"), iss), entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // entities should be an array of Entity elements
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : "abc"}
      )"), iss), entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // entities should be an array of Entity elements
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : {"a":5}}
      )"), iss), entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_THROW(AllEntities ae(entsTree), domain_error);

  // zero entities - not allowed
  BOOST_REQUIRE_NO_THROW({
    read_json((iss.str(R"(
        {"Entities" : []}
      )"), iss), entsSection);
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
      )"), iss), entsSection);
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
      )"), iss), entsSection);
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
      )"), iss), entsSection);
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
      )"), iss), entsSection);
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
      )"), iss), entsSection);
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
      )"), iss), entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_NO_THROW({
    const AllEntities ae(entsTree);
    BOOST_CHECK( ! ae.empty());
    BOOST_CHECK(ae.count() == 3ULL);
    BOOST_CHECK(ae == set<unsigned>({0U, 1U, 2U}));
    BOOST_CHECK(ae != set<unsigned>({0U, 1U, 2U, 3U}));
    BOOST_CHECK( ! (ae != set<unsigned>({0U, 1U, 2U})));
    BOOST_CHECK(ae.idsByTypes().size() == 1U);
    BOOST_CHECK(ae.idsByWeights().size() == 1U);
    BOOST_CHECK(ae.idsStartingFromLeftBank().size() == 3ULL);
    BOOST_CHECK(ae.idsStartingFromRightBank().empty());
    BOOST_CHECK_THROW(ae[3U], out_of_range);
    BOOST_CHECK(nullptr != ae[2U]);
  });

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
      )"), iss), entsSection);
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
      )"), iss), entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_NO_THROW({
    const AllEntities ae(entsTree);
    BOOST_CHECK( ! ae.empty());
    BOOST_CHECK(ae.count() == 3ULL);
    BOOST_CHECK(ae == set<unsigned>({0U, 1U, 2U}));
    BOOST_CHECK(ae != set<unsigned>({0U, 1U, 2U, 3U}));
    BOOST_CHECK( ! (ae != set<unsigned>({0U, 1U, 2U})));
    BOOST_CHECK(ae.idsByTypes().size() == 2U);
    BOOST_CHECK(ae.idsByWeights().size() == 1U);
    BOOST_CHECK(ae.idsStartingFromLeftBank().size() == 3ULL);
    BOOST_CHECK(ae.idsStartingFromRightBank().empty());
    BOOST_CHECK_THROW(ae[3U], out_of_range);
    BOOST_CHECK(nullptr != ae[2U]);
  });

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
      )"), iss), entsSection);
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
      )"), iss), entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_NO_THROW({
    const AllEntities ae(entsTree);
    BOOST_CHECK( ! ae.empty());
    BOOST_CHECK(ae.count() == 3ULL);
    BOOST_CHECK(ae == set<unsigned>({0U, 1U, 2U}));
    BOOST_CHECK(ae != set<unsigned>({0U, 1U, 2U, 3U}));
    BOOST_CHECK( ! (ae != set<unsigned>({0U, 1U, 2U})));
    BOOST_CHECK(ae.idsByTypes().size() == 1U);
    BOOST_CHECK(ae.idsByWeights().size() == 3U);
    BOOST_CHECK(ae.idsStartingFromLeftBank().size() == 3ULL);
    BOOST_CHECK(ae.idsStartingFromRightBank().empty());
    BOOST_CHECK_THROW(ae[3U], out_of_range);
    BOOST_CHECK(nullptr != ae[2U]);
  });

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
      )"), iss), entsSection);
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
      )"), iss), entsSection);
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
      )"), iss), entsSection);
    entsTree = entsSection.get_child("Entities");
  });
  BOOST_CHECK_NO_THROW({
    const AllEntities ae(entsTree);
    BOOST_CHECK( ! ae.empty());
    BOOST_CHECK(ae.count() == 3ULL);
    BOOST_CHECK(ae == set<unsigned>({0U, 1U, 2U}));
    BOOST_CHECK(ae != set<unsigned>({0U, 1U, 2U, 3U}));
    BOOST_CHECK( ! (ae != set<unsigned>({0U, 1U, 2U})));
    BOOST_CHECK(ae.idsByTypes().size() == 1U);
    BOOST_CHECK(ae.idsByWeights().size() == 1U);
    BOOST_CHECK(ae.idsStartingFromLeftBank().size() == 1ULL);
    BOOST_CHECK(ae.idsStartingFromRightBank().size() == 2ULL);
    BOOST_CHECK_THROW(ae[3U], out_of_range);
    BOOST_CHECK(nullptr != ae[2U]);
  });
}

BOOST_AUTO_TEST_CASE(movingEntities_usecases) {
  BOOST_CHECK_THROW(MovingEntities(nullptr, {}), invalid_argument);

  AllEntities *pAe = new AllEntities;
  shared_ptr<const AllEntities> ae = shared_ptr<const AllEntities>(pAe),
    aeForeign = make_shared<const AllEntities>();

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
    *pAe += make_shared<const Entity>(9U, "e0", "t0", false, "true", 9.);
  });

  BOOST_CHECK_NO_THROW({
    MovingEntities me(ae);
    MovingEntities me1(aeForeign);
    BOOST_CHECK_THROW(me = MovingEntities(aeForeign), logic_error);
    BOOST_CHECK_THROW(me = me1, logic_error);
    BOOST_CHECK_THROW(me += 10U, out_of_range); // no entity with id 10
    BOOST_CHECK(me.empty());
    BOOST_TEST(me.weight() == 0.);

    me += 1U;
    BOOST_CHECK(me == set<unsigned>({1U}));
    BOOST_CHECK(me != set<unsigned>({1U, 2U}));
    BOOST_CHECK( ! (me != set<unsigned>({1U})));
    BOOST_CHECK( ! me.empty());
    BOOST_CHECK(me.count() == 1ULL);
    BOOST_TEST(me.weight() == 1.);
    BOOST_CHECK_THROW(me += 1U, domain_error); // duplicate id
    BOOST_CHECK_THROW(me -= 2U, domain_error); // entity 2 wasn't in me

    me -= 1U;
    BOOST_CHECK(me.empty());
    BOOST_CHECK(me.count() == 0ULL);

    (me += 3U).clear();
    BOOST_CHECK(me.empty());
    BOOST_TEST(me.weight() == 0.);

    me = vector<unsigned>({1U, 2U});
    set<unsigned> expectedIds({1U,2U});
    BOOST_CHECK(me == expectedIds);
    BOOST_CHECK(me != set<unsigned>({1U, 2U, 3U}));
    BOOST_CHECK( ! (me != expectedIds));
    BOOST_CHECK(equal(CBOUNDS(me.ids()), CBOUNDS(expectedIds)));
    BOOST_CHECK(me.idsByTypes().size() == 2ULL); // t0 and t1
    BOOST_CHECK( ! me.anyRowCapableEnts(InitialSymbolsTable)); // CrossingIndex % 2 == 0
    BOOST_TEST(me.weight() == 3.);

    me += 4U; // Entity 4 rows for CrossingIndex % 2 == 0
    expectedIds = set<unsigned>({1U,2U,4U});
    BOOST_CHECK(me == expectedIds);
    BOOST_CHECK(me != set<unsigned>({1U, 2U}));
    BOOST_CHECK( ! (me != expectedIds));
    BOOST_CHECK(equal(CBOUNDS(me.ids()), CBOUNDS(expectedIds)));
    BOOST_CHECK(me.idsByTypes().size() == 3ULL); // t0, t1 and t2
    BOOST_CHECK(me.anyRowCapableEnts(InitialSymbolsTable)); // 4 can row
  });
}

BOOST_AUTO_TEST_CASE(bankEntities_usecases) {
  BOOST_CHECK_THROW(BankEntities(nullptr, {}), invalid_argument);

  AllEntities *pAe = new AllEntities;
  shared_ptr<const AllEntities> ae = shared_ptr<const AllEntities>(pAe),
    aeForeign = make_shared<const AllEntities>();

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
    *pAe += make_shared<const Entity>(9U, "e0", "t0", false, "true", 9.);
  });
  const size_t entsCount = pAe->count();

  MovingEntities me(ae);

  BOOST_CHECK_NO_THROW({
    BankEntities be(ae);
    BankEntities be1(ae);
    BankEntities beForeign(aeForeign);

    BOOST_CHECK_THROW(be = BankEntities(aeForeign), logic_error);
    BOOST_CHECK_THROW(be = beForeign, logic_error);
    BOOST_CHECK_THROW(be += me = vector<unsigned>({10U}), out_of_range); // no entity with id 10
    BOOST_CHECK(be.empty());
    BOOST_CHECK(be == set<unsigned>());
    BOOST_CHECK(be != set<unsigned>({1U, 2U}));
    BOOST_CHECK( ! (be != set<unsigned>()));
    BOOST_CHECK(~be == ae->ids());
    BOOST_CHECK(be.differencesCount(be) == 0ULL);
    BOOST_CHECK(be.differencesCount(be1) == 0ULL);

    be += me = vector<unsigned>({1U});
    BOOST_CHECK(be == set<unsigned>({1U}));
    BOOST_CHECK(be != set<unsigned>({1U, 2U}));
    BOOST_CHECK( ! (be != set<unsigned>({1U})));
    BOOST_CHECK( ! be.empty());
    BOOST_CHECK(be.count() == 1ULL);
    BOOST_CHECK((~be).count() == entsCount - 1ULL);
    BOOST_CHECK_THROW(be += me = vector<unsigned>({1U}), domain_error); // duplicate id
    BOOST_CHECK_THROW(be -= me = vector<unsigned>({2U}), domain_error); // entity 2 wasn't in be
    BOOST_CHECK(be.differencesCount(be) == 0ULL);
    BOOST_CHECK(be.differencesCount(be1) == 1ULL);

    be -= me = vector<unsigned>({1U});
    BOOST_CHECK(be.empty());
    BOOST_CHECK(be.count() == 0ULL);
    BOOST_CHECK(~be == ae->ids());

    (be += me = vector<unsigned>({3U})).clear();
    BOOST_CHECK(be.empty());
    BOOST_CHECK(~be == ae->ids());

    be = vector<unsigned>({1U, 2U});
    set<unsigned> expectedIds({1U,2U});
    BOOST_CHECK(be == expectedIds);
    BOOST_CHECK(be != set<unsigned>({1U, 2U, 3U}));
    BOOST_CHECK( ! (be != expectedIds));
    BOOST_CHECK((~be).count() == entsCount - 2ULL);
    BOOST_CHECK(equal(CBOUNDS(be.ids()), CBOUNDS(expectedIds)));
    BOOST_CHECK(be.idsByTypes().size() == 2ULL); // t0 and t1
    BOOST_CHECK( ! be.anyRowCapableEnts(InitialSymbolsTable)); // CrossingIndex % 2 == 0
    BOOST_CHECK(be.differencesCount(be) == 0ULL);
    BOOST_CHECK(be.differencesCount(be1) == 2ULL);

    be += me = vector<unsigned>({4U}); // Entity 4 rows for CrossingIndex % 2 == 0
    expectedIds = set<unsigned>({1U,2U,4U});
    BOOST_CHECK(be == expectedIds);
    BOOST_CHECK(be != set<unsigned>({1U, 2U}));
    BOOST_CHECK( ! (be != expectedIds));
    BOOST_CHECK((~be).count() == entsCount - 3ULL);
    BOOST_CHECK(equal(CBOUNDS(be.ids()), CBOUNDS(expectedIds)));
    BOOST_CHECK(be.idsByTypes().size() == 3ULL); // t0, t1 and t2
    BOOST_CHECK(be.anyRowCapableEnts(InitialSymbolsTable)); // 4 can row
    BOOST_CHECK(be.differencesCount(be) == 0ULL);
    BOOST_CHECK(be.differencesCount(be1) == 3ULL);

    be1 += me = vector<unsigned>({4U, 5U, 6U});
    BOOST_CHECK(be1.differencesCount(be1) == 0ULL);
    BOOST_CHECK(be.differencesCount(be1) == 4ULL); // 1,2,5,6
    BOOST_CHECK(be1.differencesCount(be) == 4ULL); // 1,2,5,6
  });
}

BOOST_AUTO_TEST_SUITE_END()

#endif // for ENTITIES_MANAGER_CPP and UNIT_TESTING
