/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#if !defined CPP_ENTITY || !defined UNIT_TESTING

#error \
    "Please include this file only within `entity.cpp` \
after a `#define CPP_ENTITY` and surrounding the include \
and the define by `#ifdef UNIT_TESTING`!"

#else  // for CPP_ENTITY and UNIT_TESTING

#include "entity.h"
#include "mathRelated.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(entity, *boost::unit_test::tolerance(rc::Eps))

BOOST_AUTO_TEST_CASE(construction_fieldsAsParams) {
  using namespace std;
  using namespace rc;
  using namespace rc::ent;

  try {  // using all default params
    Entity e{123U, "abc"};
    BOOST_CHECK(e.id() == 123U);
    BOOST_CHECK(e.name() == "abc");
    BOOST_CHECK(e.type().empty());
    BOOST_CHECK(!e.startsFromRightBank());
    BOOST_TEST(!e.weight());
    BOOST_CHECK(!(bool)e.canRow());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!e.canRow({})));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // using no default params
    Entity e{123U, "abc", "def", true, "true", 123.};
    BOOST_CHECK(e.id() == 123U);
    BOOST_CHECK(e.name() == "abc");
    BOOST_CHECK(e.type() == "def");
    BOOST_CHECK(e.startsFromRightBank());
    BOOST_TEST(e.weight() == 123.);
    BOOST_CHECK((bool)e.canRow());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(e.canRow({})));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  BOOST_CHECK_THROW(Entity(123U, "abc", "def", true, "TrUe"),
                    domain_error);  // invalid canRowExpr
  BOOST_CHECK_THROW(Entity(123U, "abc", "def", true, "true", -123.),
                    invalid_argument);  // negative weight
}

BOOST_AUTO_TEST_CASE(construction_ptreeAsParam) {
  using namespace std;
  using namespace rc;
  using namespace rc::ent;
  using namespace boost::property_tree;

  ptree entTree;
  istringstream iss;

  // using all default params
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 10,
        "Name": "Father"
      }
    )"),
                                    iss),
                                   entTree));
  try {
    Entity e{entTree};
    BOOST_CHECK(e.id() == 10U);
    BOOST_CHECK(e.name() == "Father");
    BOOST_CHECK(e.type().empty());
    BOOST_CHECK(!e.startsFromRightBank());
    BOOST_TEST(!e.weight());
    BOOST_CHECK(!(bool)e.canRow());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!e.canRow({})));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // using no default params
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "Type": "def",
        "StartsFromRightBank": "true",
        "CanRow": "if (%CrossingIndex% mod 4) in {0, 3}",
        "Weight": 123
      }
    )"),
                                    iss),
                                   entTree));
  try {
    Entity e{entTree};
    BOOST_CHECK(e.id() == 123U);
    BOOST_CHECK(e.name() == "abc");
    BOOST_CHECK(e.type() == "def");
    BOOST_CHECK(e.startsFromRightBank());
    BOOST_TEST(e.weight() == 123.);
    BOOST_CHECK(boost::logic::indeterminate(e.canRow()));
    BOOST_CHECK(e.canRow({{"CrossingIndex", 4}}));
    BOOST_CHECK(e.canRow({{"CrossingIndex", 7}}));
    BOOST_CHECK(!e.canRow({{"CrossingIndex", 5}}));
    BOOST_CHECK(!e.canRow({{"CrossingIndex", 6}}));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // using the synonym of CanRow: CanTackleBridgeCrossing
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "CanTackleBridgeCrossing": "true"
      }
    )"),
                                    iss),
                                   entTree));
  try {
    Entity e{entTree};
    BOOST_CHECK(e.id() == 123U);
    BOOST_CHECK(e.name() == "abc");
    BOOST_CHECK(e.type().empty());
    BOOST_CHECK(!e.startsFromRightBank());
    BOOST_TEST(!e.weight());
    BOOST_CHECK((bool)e.canRow());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(e.canRow({})));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // using both synonyms CanRow and CanTackleBridgeCrossing
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "CanRow": "false",
        "CanTackleBridgeCrossing": "true"
      }
    )"),
                                    iss),
                                   entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // non-number Id
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": "id",
        "Name": "abc"
      }
    )"),
                                    iss),
                                   entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // negative Id
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": -123,
        "Name": "abc"
      }
    )"),
                                    iss),
                                   entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // incorrect canRowExpr
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "CanRow": "TrUe"
      }
    )"),
                                    iss),
                                   entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // incorrect StartsFromRightBank
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "StartsFromRightBank": "TrUe"
      }
    )"),
                                    iss),
                                   entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // non-number Weight
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "Weight": "weight"
      }
    )"),
                                    iss),
                                   entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // provided Weight was 0
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "Weight": 0
      }
    )"),
                                    iss),
                                   entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // negative Weight
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "Weight": -12.4
      }
    )"),
                                    iss),
                                   entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);
}

BOOST_AUTO_TEST_SUITE_END()

#endif  // for CPP_ENTITY and UNIT_TESTING
