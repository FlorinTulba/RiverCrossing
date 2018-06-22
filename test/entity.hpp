/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#if ! defined ENTITY_CPP || ! defined UNIT_TESTING

  #error \
Please include this file only at the end of `entity.cpp` \
after a `#define ENTITY_CPP` and surrounding the include and the define \
by `#ifdef UNIT_TESTING`!

#else // for ENTITY_CPP and UNIT_TESTING

#include "../src/mathRelated.h"

#include <boost/test/unit_test.hpp>

#include <boost/property_tree/json_parser.hpp>

using namespace rc;
using namespace rc::ent;

BOOST_AUTO_TEST_SUITE(entity, *boost::unit_test::tolerance(Eps))

BOOST_AUTO_TEST_CASE(construction_fieldsAsParams) {
  BOOST_CHECK_NO_THROW({ // using all default params
    Entity e(123U, "abc");
    BOOST_CHECK(e.id() == 123U);
    BOOST_CHECK(0 == e.name().compare("abc"));
    BOOST_CHECK(0 == e.type().compare(""));
    BOOST_CHECK( ! e.startsFromRightBank());
    BOOST_TEST(e.weight() == 0.);
    BOOST_CHECK( ! e.canRow());
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! e.canRow(SymbolsTable{})));
  });

  BOOST_CHECK_NO_THROW({ // using no default params
    Entity e(123U, "abc", "def", true, "true", 123.);
    BOOST_CHECK(e.id() == 123U);
    BOOST_CHECK(0 == e.name().compare("abc"));
    BOOST_CHECK(0 == e.type().compare("def"));
    BOOST_CHECK(e.startsFromRightBank());
    BOOST_TEST(e.weight() == 123.);
    BOOST_CHECK(e.canRow());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(e.canRow(SymbolsTable{})));
  });

  BOOST_CHECK_THROW(Entity(123U, "abc", "def", true, "TrUe"),
    domain_error); // invalid canRowExpr
  BOOST_CHECK_THROW(Entity(123U, "abc", "def", true, "true", -123.),
    invalid_argument); // negative weight
}

BOOST_AUTO_TEST_CASE(construction_ptreeAsParam) {
  ptree entTree;
  istringstream iss;

  // using all default params
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 10,
        "Name": "Father"
      }
    )"), iss), entTree));
  BOOST_CHECK_NO_THROW({
    Entity e(entTree);
    BOOST_CHECK(e.id() == 10U);
    BOOST_CHECK(0 == e.name().compare("Father"));
    BOOST_CHECK(0 == e.type().compare(""));
    BOOST_CHECK( ! e.startsFromRightBank());
    BOOST_TEST(e.weight() == 0.);
    BOOST_CHECK( ! e.canRow());
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! e.canRow(SymbolsTable{})));
  });

  // using no default params
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "Type": "def",
        "StartsFromRightBank": "true",
        "CanRow": "if (%CrossingIndex% mod 4) in {0, 3}",
        "Weight": 123
      }
    )"), iss), entTree));
  BOOST_CHECK_NO_THROW({
    Entity e(entTree);
    BOOST_CHECK(e.id() == 123U);
    BOOST_CHECK(0 == e.name().compare("abc"));
    BOOST_CHECK(0 == e.type().compare("def"));
    BOOST_CHECK(e.startsFromRightBank());
    BOOST_TEST(e.weight() == 123.);
    BOOST_CHECK(boost::logic::indeterminate(e.canRow()));
    BOOST_CHECK_NO_THROW(
      BOOST_CHECK(e.canRow(SymbolsTable{{"CrossingIndex", 4}})));
    BOOST_CHECK_NO_THROW(
      BOOST_CHECK(e.canRow(SymbolsTable{{"CrossingIndex", 7}})));
    BOOST_CHECK_NO_THROW(
      BOOST_CHECK( ! e.canRow(SymbolsTable{{"CrossingIndex", 5}})));
    BOOST_CHECK_NO_THROW(
      BOOST_CHECK( ! e.canRow(SymbolsTable{{"CrossingIndex", 6}})));
  });

  // using the synonym of CanRow: CanTackleBridgeCrossing
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "CanTackleBridgeCrossing": "true"
      }
    )"), iss), entTree));
  BOOST_CHECK_NO_THROW({
    Entity e(entTree);
    BOOST_CHECK(e.id() == 123U);
    BOOST_CHECK(0 == e.name().compare("abc"));
    BOOST_CHECK(0 == e.type().compare(""));
    BOOST_CHECK( ! e.startsFromRightBank());
    BOOST_TEST(e.weight() == 0.);
    BOOST_CHECK(e.canRow());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(e.canRow(SymbolsTable{})));
  });

  // using both synonyms CanRow and CanTackleBridgeCrossing
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "CanRow": "false",
        "CanTackleBridgeCrossing": "true"
      }
    )"), iss), entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // non-number Id
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": "id",
        "Name": "abc"
      }
    )"), iss), entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // negative Id
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": -123,
        "Name": "abc"
      }
    )"), iss), entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // incorrect canRowExpr
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "CanRow": "TrUe"
      }
    )"), iss), entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // incorrect StartsFromRightBank
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "StartsFromRightBank": "TrUe"
      }
    )"), iss), entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // non-number Weight
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "Weight": "weight"
      }
    )"), iss), entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // provided Weight was 0
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "Weight": 0
      }
    )"), iss), entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);

  // negative Weight
  BOOST_REQUIRE_NO_THROW(read_json((iss.str(R"(
      {"Id": 123,
        "Name": "abc",
        "Weight": -12.4
      }
    )"), iss), entTree));
  BOOST_CHECK_THROW(Entity e(entTree), domain_error);
}

BOOST_AUTO_TEST_SUITE_END()

#endif // for ENTITY_CPP and UNIT_TESTING
