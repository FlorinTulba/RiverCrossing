/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#if ! defined CONFIG_PARSER_CPP || ! defined UNIT_TESTING

  #error \
"Please include this file only at the end of `configParser.cpp` \
after a `#define CONFIG_PARSER_CPP` and surrounding the include and the define \
by `#ifdef UNIT_TESTING`!"

#else // for CONFIG_PARSER_CPP and UNIT_TESTING

#include "../src/mathRelated.h"

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <iomanip>

namespace bdata = boost::unit_test::data;
using namespace std;
using namespace rc;

BOOST_AUTO_TEST_SUITE(configGrammar, *boost::unit_test::tolerance(Eps))

// IdsGroup ::= \(<<unsigned>> (\s* \| \s* <<unsigned>>)+\)
BOOST_AUTO_TEST_CASE(idsGroupRule_correctInput) {
  using namespace grammar;

  bool b;
  {vector<unsigned> v; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
      "(  3|4  )", // group of 2 entities
      idsGroup, v)));
    if(b) BOOST_CHECK(v.size() == 2ULL);}
  {vector<unsigned> v; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "(3 | 4)", // group of 2 entities
    idsGroup, v)));
    if(b) BOOST_CHECK(v.size() == 2ULL);}
  {vector<unsigned> v; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "(3|4|5)", // group of 3 entities
    idsGroup, v)));
    if(b) BOOST_CHECK(v.size() == 3ULL);}
}
// IdsGroup ::= \(<<unsigned>> (\s* \| \s* <<unsigned>>)+\)
BOOST_DATA_TEST_CASE(idsGroupRule_wrongInput,
    bdata::make(vector<string>{"", "3", "(3)"}),
    wrongInput) {
  using namespace grammar;

  vector<unsigned> v;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, idsGroup, v)) );
}

/*
  IdsConfig ::= - | IdsTerm (\s+ IdsTerm)* \s+ (...)?

  IdsTerm ::= \* | !?<<unsigned>> | <<unsigned>>\? | IdsGroup\??
*/
BOOST_AUTO_TEST_CASE(idsConfigRule_correctInput) {
  using namespace grammar;

  bool b;
  {IdsConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "-", // empty set
    idsConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedIds.empty());
      BOOST_CHECK(c.mandatoryGroups.empty());
      BOOST_CHECK(c.optionalGroups.empty());
      BOOST_CHECK(c.avoidedIds.empty());
      BOOST_CHECK(c.expectedExtraIds == 0U);
      BOOST_CHECK(c.capacityLimit);
    }}
  {IdsConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "*", // one unspecified mandatory entity
    idsConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedIds.empty());
      BOOST_CHECK(c.mandatoryGroups.empty());
      BOOST_CHECK(c.optionalGroups.empty());
      BOOST_CHECK(c.avoidedIds.empty());
      BOOST_CHECK(c.expectedExtraIds == 1U);
      BOOST_CHECK(c.capacityLimit);
    }}
  {IdsConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "3", // one mandatory entity
    idsConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedIds.size() == 1ULL);
      BOOST_CHECK(b = (c.mandatoryGroups.size() == 1ULL));
      if(b) BOOST_CHECK(c.mandatoryGroups.front().size() == 1ULL);
      BOOST_CHECK(c.optionalGroups.empty());
      BOOST_CHECK(c.avoidedIds.empty());
      BOOST_CHECK(c.expectedExtraIds == 0U);
      BOOST_CHECK(c.capacityLimit);
    }}
  {IdsConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "!3", // one avoided entity
    idsConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedIds.size() == 1ULL);
      BOOST_CHECK(c.mandatoryGroups.empty());
      BOOST_CHECK(c.optionalGroups.empty());
      BOOST_CHECK(c.avoidedIds.size() == 1ULL);
      BOOST_CHECK(c.expectedExtraIds == 0U);
      BOOST_CHECK(c.capacityLimit);
    }}
  {IdsConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "3?", // one optional entity
    idsConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedIds.size() == 1ULL);
      BOOST_CHECK(c.mandatoryGroups.empty());
      BOOST_CHECK(b = (c.optionalGroups.size() == 1ULL));
      if(b) BOOST_CHECK(c.optionalGroups.front().size() == 1ULL);
      BOOST_CHECK(c.avoidedIds.empty());
      BOOST_CHECK(c.expectedExtraIds == 0U);
      BOOST_CHECK(c.capacityLimit);
    }}
  {IdsConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "3 ...", // one mandatory entity and any number from the remaining ones
    idsConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedIds.size() == 1ULL);
      BOOST_CHECK(b = (c.mandatoryGroups.size() == 1ULL));
      if(b) BOOST_CHECK(c.mandatoryGroups.front().size() == 1ULL);
      BOOST_CHECK(c.optionalGroups.empty());
      BOOST_CHECK(c.avoidedIds.empty());
      BOOST_CHECK(c.expectedExtraIds == 0U);
      BOOST_CHECK(! c.capacityLimit);
    }}
  {IdsConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "(3|4|5)", // a mandatory group of 3 entities
    idsConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedIds.size() == 3ULL);
      BOOST_CHECK(b = (c.mandatoryGroups.size() == 1ULL));
      if(b) BOOST_CHECK(c.mandatoryGroups.front().size() == 3ULL);
      BOOST_CHECK(c.optionalGroups.empty());
      BOOST_CHECK(c.avoidedIds.empty());
      BOOST_CHECK(c.expectedExtraIds == 0U);
      BOOST_CHECK(c.capacityLimit);
    }}
  {IdsConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "(3|4|5)?", // an optional group of 3 entities
    idsConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedIds.size() == 3ULL);
      BOOST_CHECK(c.mandatoryGroups.empty());
      BOOST_CHECK(b = (c.optionalGroups.size() == 1ULL));
      if(b) BOOST_CHECK(c.optionalGroups.front().size() == 3ULL);
      BOOST_CHECK(c.avoidedIds.empty());
      BOOST_CHECK(c.expectedExtraIds == 0U);
      BOOST_CHECK(c.capacityLimit);
    }}
  {IdsConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    // unbounded set of several mandatory/avoided/optional entities/groups
    "0  * 1?  * !2  * (3 |4|  5) *   ( 6| 7 )? *  !8   ...",
    idsConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedIds.size() == 9ULL);
      BOOST_CHECK(b = (c.mandatoryGroups.size() == 2ULL));
      if(b) {
        BOOST_CHECK(c.mandatoryGroups.front().size() == 1ULL);
        BOOST_CHECK(c.mandatoryGroups.back().size() == 3ULL);
      }
      BOOST_CHECK(b = (c.optionalGroups.size() == 2ULL));
      if(b) {
        BOOST_CHECK(c.optionalGroups.front().size() == 1ULL);
        BOOST_CHECK(c.optionalGroups.back().size() == 2ULL);
      }
      BOOST_CHECK(c.avoidedIds.size() == 2ULL);
      BOOST_CHECK(c.expectedExtraIds == 5U);
      BOOST_CHECK(! c.capacityLimit);
    }}
}
/*
  IdsConfig ::= - | IdsTerm (\s+ IdsTerm)* \s+ (...)?

  IdsTerm ::= \* | !?<<unsigned>> | <<unsigned>>\? | IdsGroup\??
*/
BOOST_DATA_TEST_CASE(idsConfigRule_wrongInput,
    bdata::make(vector<string>{
      "", "1...", "(1|2)...", "!1...", "1?...", "(1|2)?...", "...", "*...",
      "- ...", "! 0", "! (0|1)", "(0|1) ?", "*?", "!*", "*!0", "**", "0*",
      "*0", "1 ..", "1 1", "1 !2 3? (4|5) * 1 * ..."}),
    wrongInput) {
  using namespace grammar;

  IdsConstraint c;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, idsConfig, c)) );
}

// TypeName ::= <<alpha>> (<<alnum>> | - | _)*
BOOST_AUTO_TEST_CASE(typeNameRule_correctInput) {
  using namespace grammar;
  using std::string;

  bool b;
  const string inputs[] = {
    "a", // typeName of exactly one alphabetic character
    "a1b-_" // typeName with all categories of accepted characters
  };
  for(const string &inp : inputs) {
    string s;
    BOOST_TEST_CONTEXT("for inp = `"<<inp<<'`') {
      b = false;
      BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(inp, typeName, s)) );
      if(b) BOOST_CHECK(s.compare(inp) == 0);
    }
  }
}
// TypeName ::= <<alpha>> (<<alnum>> | - | _)*
BOOST_DATA_TEST_CASE(typeNameRule_wrongInput,
    bdata::make(vector<std::string>{
      "", "1", "-", "_"}),
    wrongInput) {
  using namespace grammar;

  std::string s;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, typeName, s)) );
}

/*
  TypesConfig ::= TypeTerm ( \s+ \+ \s+ TypeTerm)*

  TypeTerm ::= (<<unsigned>>(\+|-)? \s+ x \s+)? TypeName
*/
BOOST_AUTO_TEST_CASE(typesConfigRule_correctInput) {
  using namespace grammar;

  bool b;
  {TypesConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "0  x   frog", // expecting exactly 0 frogs
    typesConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedTypes.size() == 1ULL);
      BOOST_CHECK(c.mandatoryTypes.empty());
      BOOST_CHECK(c.optionalTypes.empty());
    }}
  {TypesConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "frog", // expecting exactly 1 frog
    typesConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedTypes.size() == 1ULL);
      BOOST_CHECK(b = (c.mandatoryTypes.size() == 1ULL));
      if(b)
        BOOST_CHECK(c.mandatoryTypes.begin()->second == make_pair(1U, 1U));
      BOOST_CHECK(c.optionalTypes.empty());
    }}
  {TypesConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "2 x  frog", // expecting exactly 2 frogs
    typesConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedTypes.size() == 1ULL);
      BOOST_CHECK(b = (c.mandatoryTypes.size() == 1ULL));
      if(b)
        BOOST_CHECK(c.mandatoryTypes.begin()->second == make_pair(2U, 2U));
      BOOST_CHECK(c.optionalTypes.empty());
    }}
  {TypesConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "2+ x frog", // expecting at least 2 frogs
    typesConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedTypes.size() == 1ULL);
      BOOST_CHECK(b = (c.mandatoryTypes.size() == 1ULL));
      if(b) BOOST_CHECK(
        c.mandatoryTypes.begin()->second == make_pair(2U, UINT_MAX));
      BOOST_CHECK(c.optionalTypes.empty());
    }}
  {TypesConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "2- x frog", // expecting at most 2 frogs
    typesConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedTypes.size() == 1ULL);
      BOOST_CHECK(c.mandatoryTypes.empty());
      BOOST_CHECK(b = (c.optionalTypes.size() == 1ULL));
      if(b) BOOST_CHECK(c.optionalTypes.begin()->second == 2U);
    }}
  {TypesConstraint c; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    // expecting >=2 frogs, <=3 insects, 2 birds and 1 fish
    "2+ x frog  +   3- x insect  + 2 x bird + fish",
    typesConfig, c)));
    if(b) {
      BOOST_CHECK(c.mentionedTypes.size() == 4ULL);
      BOOST_CHECK(c.mandatoryTypes.size() == 3ULL);
      BOOST_CHECK_NO_THROW(
        BOOST_CHECK(c.mandatoryTypes.at("frog") == make_pair(2U, UINT_MAX)));
      BOOST_CHECK_NO_THROW(
        BOOST_CHECK(c.mandatoryTypes.at("bird") == make_pair(2U, 2U)));
      BOOST_CHECK_NO_THROW(
        BOOST_CHECK(c.mandatoryTypes.at("fish") == make_pair(1U, 1U)));
      BOOST_CHECK(b = (c.optionalTypes.size() == 1ULL));
      if(b) BOOST_CHECK(c.optionalTypes.begin()->second == 3U);
    }}
}
/*
  TypesConfig ::= TypeTerm ( \s+ \+ \s+ TypeTerm)*

  TypeTerm ::= (<<unsigned>>(\+|-)? \s+ x \s+)? TypeName
*/
BOOST_DATA_TEST_CASE(typesConfigRule_wrongInput,
    bdata::make(vector<string>{
      "x frog", "-1 x frog", "2x frog", "2+x frog", "2-x frog", "2 * frog",
      "2 xfrog", "frog+ insect", "frog + frog", "frog +insect",
      "frog + x insect", "frog +2 x insect", "frog +2+ x insect",
      "frog +2- x insect"}),
    wrongInput) {
  using namespace grammar;

  TypesConstraint c;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, typesConfig, c)) );
}

/*
  Configurations ::= Configuration ( \s+ ; \s+ Configuration )*

  Configuration ::= TypesConfig | IdsConfig
*/
BOOST_AUTO_TEST_CASE(configurationsRule_correctInput) {
  using namespace grammar;

  // 1 type constraint followed by 2 id constraints
  const std::string input =
    "11+ x vi-sitor1_  +  30- x visitor + 4 x manager  + employee ;"
    " -  ;"
    "  !0 * 12 * (5 |  6)? * (1|2|3) 23?  ...";

  bool b;
  ConstraintsVec v;
  const TypesConstraint *tc = nullptr;
  const IdsConstraint *ic = nullptr;
  BOOST_REQUIRE_NO_THROW( BOOST_CHECK(tackle(input, configurations, v)) );
  BOOST_REQUIRE(v.size() == 3ULL);
  BOOST_CHECK(b = (nullptr != v[0]));
  if(b) {
    BOOST_CHECK(
      b = (nullptr != (tc = dynamic_cast<const TypesConstraint*>(v[0].get()))));
    if(b) {
      BOOST_CHECK(tc->mentionedTypes.size() == 4ULL);
      BOOST_CHECK(b = (tc->mandatoryTypes.size() == 3ULL));
      if(b) {
        BOOST_CHECK_NO_THROW( BOOST_CHECK(
          tc->mandatoryTypes.at("vi-sitor1_") == make_pair(11U, UINT_MAX)));
        BOOST_CHECK_NO_THROW( BOOST_CHECK(
          tc->mandatoryTypes.at("manager") == make_pair(4U, 4U)));
        BOOST_CHECK_NO_THROW( BOOST_CHECK(
          tc->mandatoryTypes.at("employee") == make_pair(1U, 1U)));
      }
      BOOST_CHECK(b = (tc->optionalTypes.size() == 1ULL));
      if(b) BOOST_CHECK(tc->optionalTypes.begin()->second == 30U);
    }
  }

  BOOST_CHECK(b = (nullptr != v[1]));
  if(b) {
    BOOST_CHECK(
      b = (nullptr != (ic = dynamic_cast<const IdsConstraint*>(v[1].get()))));
    if(b) {
      BOOST_CHECK(ic->mentionedIds.empty());
      BOOST_CHECK(ic->mandatoryGroups.empty());
      BOOST_CHECK(ic->optionalGroups.empty());
      BOOST_CHECK(ic->avoidedIds.empty());
      BOOST_CHECK(ic->expectedExtraIds == 0U);
      BOOST_CHECK(ic->capacityLimit);
    }
  }

  BOOST_CHECK(b = (nullptr != v[2]));
  if(b) {
    BOOST_CHECK(
      b = (nullptr != (ic = dynamic_cast<const IdsConstraint*>(v[2].get()))));
    if(b) {
      BOOST_CHECK(ic->mentionedIds.size() == 8ULL);
      BOOST_CHECK(b = (ic->mandatoryGroups.size() == 2ULL));
      if(b) {
        BOOST_CHECK(ic->mandatoryGroups.front().size() == 1ULL);
        BOOST_CHECK(ic->mandatoryGroups.back().size() == 3ULL);
      }
      BOOST_CHECK(b = (ic->optionalGroups.size() == 2ULL));
      if(b) {
        BOOST_CHECK(ic->optionalGroups.front().size() == 2ULL);
        BOOST_CHECK(ic->optionalGroups.back().size() == 1ULL);
      }
      BOOST_CHECK(ic->avoidedIds.size() == 1ULL);
      BOOST_CHECK(ic->expectedExtraIds == 3U);
      BOOST_CHECK(! ic->capacityLimit);
    }
  }
}
/*
  Configurations ::= Configuration ( \s+ ; \s+ Configuration )*

  Configuration ::= TypesConfig | IdsConfig
*/
BOOST_DATA_TEST_CASE(configurationsRule_wrongInput,
    bdata::make(vector<string>{"employee; 0", "- ;0", "!0 1;manager"}),
    wrongInput) {
  using namespace grammar;

  ConstraintsVec v;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, configurations, v)) );
}

// CrossingDurationForConfigurations ::= <<unsigned>> \s+ : \s+ Configurations
BOOST_AUTO_TEST_CASE(crossingDurationForConfigurationsRule_correctInput) {
  using namespace grammar;

  ConfigurationsTransferDurationInitType ctd;
  bool b = false;
  BOOST_CHECK(b = tackle(
    // Entity 0 or any employee cross the river in 2 time units
    "2  :  employee ; 0",
    crossingDurationForConfigurations, ctd));
  if(b) {
    BOOST_CHECK(ctd.duration() == 2U);
    BOOST_CHECK(ctd._constraints.size() == 2ULL);
  }
}
// CrossingDurationForConfigurations ::= <<unsigned>> \s+ : \s+ Configurations
BOOST_DATA_TEST_CASE(crossingDurationForConfigurationsRule_wrongInput,
    bdata::make(vector<string>{
      "employee ; 0", "2 employee ; 0", "-2 : employee ; 0", "0 : employee ; 0",
      "2: employee ; 0", "2 :employee ; 0", "2:employee ; 0", "2 : employee; 0"}),
    wrongInput) {
  using namespace grammar;

  ConfigurationsTransferDurationInitType ctd;
  BOOST_CHECK_NO_THROW(
    BOOST_CHECK( ! tackle(wrongInput, crossingDurationForConfigurations, ctd)) );
}

// Variable ::= % TypeName %
BOOST_AUTO_TEST_CASE(variableRule_correctInput) {
  using namespace grammar;
  using std::string;

  bool b;
  const string outputs[] = {
    "a", // variable of exactly one alphabetic character
    "a1b-_" // variable with all categories of accepted characters
  };
  for(const string &out : outputs) {
    ostringstream oss;
    oss<<quoted(out, '%');
    const string inp = oss.str();
    string s;
    BOOST_TEST_CONTEXT("for inp = `"<<inp<<'`') {
      b = false;
      BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(inp, variable, s)) );
      if(b) BOOST_CHECK(s.compare(out) == 0);
    }
  }
}
// Variable ::= % TypeName %
BOOST_DATA_TEST_CASE(variableRule_wrongInput,
    bdata::make(vector<std::string>{
      "", "a", "%a", "a%", "%%", "%1%", "%-%", "%_%"}),
    wrongInput) {
  using namespace grammar;

  std::string s;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, variable, s)) );
}

// Value ::= <<constant>> | Variable
BOOST_AUTO_TEST_CASE(valueRule_correctInput) {
  using namespace grammar;
  using std::string;

  bool b;

  // variables
  {const SymbolsTable st{{"abc", 10.}};
    shared_ptr<const NumericExpr> ne; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "%abc%", // valid variable
    value, ne)));
    if(b) {
      BOOST_CHECK(b = (nullptr != ne));
      if(b) {
        BOOST_CHECK(b = ! ne->constValue());
        if(b) BOOST_CHECK_NO_THROW( BOOST_TEST(ne->eval(st) == 10.) );
      }
    }}

  // valid double values inputs
  double d;
  SymbolsTable st;
  const string inputs[] = {"100.", ".1", "0.1", "1e2", "11E-2", "11.22e-2"};
  for(const string &inp : inputs) {
    shared_ptr<const NumericExpr> ne;
    istringstream iss(inp);
    BOOST_TEST_CONTEXT("for inp = `"<<inp<<'`') {
      BOOST_CHECK(b = (bool)(iss>>d));
      if(b) {
        b = false;
        BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(inp, value, ne)) );
        if(b) {
          BOOST_CHECK(b = (nullptr != ne));
          if(b) {
            BOOST_CHECK_NO_THROW( BOOST_TEST(ne->eval(st) == d) );
            BOOST_CHECK(b = (bool) ne->constValue());
            if(b) BOOST_CHECK_NO_THROW( BOOST_TEST(*ne->constValue() == d) );
          }
        }
      }
    }
  }
}
// Value ::= <<constant>> | Variable
BOOST_DATA_TEST_CASE(valueRule_wrongInput,
    bdata::make(vector<string>{"", "100..", "abc"}),
    wrongInput) {
  using namespace grammar;

  shared_ptr<const NumericExpr> ne;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, value, ne)) );
}

// UnfencedOperand ::= Value | add \s* \( \s* MathExpr \s* , \s* MathExpr \s* \)
BOOST_AUTO_TEST_CASE(unfencedOperandRule_correctInput) {
  using namespace grammar;
  using std::string;

  bool b;
  SymbolsTable st;

  // expressions evaluated as constants
  const unordered_map<string, double> inpPlusExpected {
    {"0.1", .1}, // constant
    {"add(10, 3)", 13.}, // sum of constants
    {"add(10, 3 mod 10)", 13.}, // 10 + mod(3, 10) = 13
    {"add(10, (3 mod 10) mod 2)", 11.} // 10 + mod(mod(3,10) , 2) = 11
  };
  for(const auto &ioPair : inpPlusExpected) {
    shared_ptr<const NumericExpr> ne;
    const string &inp = ioPair.first;
    const double expected = ioPair.second;
    BOOST_TEST_CONTEXT("for inp = `"<<inp<<"` and expected = "<<expected) {
      b = false;
      BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(inp, unfencedOperand, ne)) );
      if(b) {
        BOOST_CHECK(b = (nullptr != ne));
        if(b) {
          BOOST_CHECK_NO_THROW( BOOST_TEST(ne->eval(st) == expected) );
          BOOST_CHECK(b = (bool) ne->constValue());
          if(b) BOOST_CHECK_NO_THROW( BOOST_TEST(*ne->constValue() == expected) );
        }
      }
    }
  }

  // expressions containing variables
  {st = SymbolsTable{{"abc", 10.}};
    shared_ptr<const NumericExpr> ne; b = false;
    BOOST_CHECK(b = tackle(
    "%abc%",
    unfencedOperand, ne));
    if(b) {
      BOOST_CHECK(b = (nullptr != ne));
      if(b) {
        BOOST_CHECK_NO_THROW( BOOST_TEST(ne->eval(st) == 10.) );
        BOOST_CHECK(! ne->constValue());
      }
    }}
  {st = SymbolsTable{{"abc", 10.}};
    shared_ptr<const NumericExpr> ne; b = false;
    BOOST_CHECK(b = tackle(
    "add(%abc%, 3)",
    unfencedOperand, ne));
    if(b) {
      BOOST_CHECK(b = (nullptr != ne));
      if(b) {
        BOOST_CHECK_NO_THROW( BOOST_TEST(ne->eval(st) == 13.) );
        BOOST_CHECK(! ne->constValue());
      }
    }}
  {st = SymbolsTable{{"a", 10}, {"b", 3}, {"c", 2}};
    shared_ptr<const NumericExpr> ne; b = false;
    BOOST_CHECK(b = tackle(
    "add(%a%, (%b% mod %a%) mod %c%)",
    unfencedOperand, ne));
    if(b) {
      BOOST_CHECK(b = (nullptr != ne));
      if(b) {
        BOOST_CHECK_NO_THROW( BOOST_TEST(ne->eval(st) == 11.) );
        BOOST_CHECK(! ne->constValue());
      }
    }}
}
// UnfencedOperand ::= Value | add \s* \( \s* MathExpr \s* , \s* MathExpr \s* \)
BOOST_DATA_TEST_CASE(unfencedOperandRule_wrongInput,
    bdata::make(vector<string>{
      "(0.1)", "add(0)", "add 0 1", "(add(0,1))", "(add(0,1,2))", "(add(0 1))",
      "0 + 1", "(%abc%)", "(%abc% mod %abc%)"}),
    wrongInput) {
  using namespace grammar;

  shared_ptr<const NumericExpr> ne;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, unfencedOperand, ne)) );
}

// Operand ::= UnfencedOperand | \( \s* LeftRecursiveMathExpr \s* \)
BOOST_AUTO_TEST_CASE(operandRule_correctInput) {
  using namespace grammar;
  using std::string;

  bool b;
  SymbolsTable st;

  // expressions evaluated as constants
  const unordered_map<string, double> inpPlusExpected {
    {"0.1", .1}, // constant
    {"add(10, 3)", 13.}, // sum of constants
    {"(add(10, 3) mod 10)", 3.}, // mod(10+3, 10) = 3
    {"((add(10, 3) mod 10) mod 2)", 1.} // mod( mod(10+3, 10), 2) = 1
  };
  for(const auto &ioPair : inpPlusExpected) {
    shared_ptr<const NumericExpr> ne;
    const string &inp = ioPair.first;
    const double expected = ioPair.second;
    BOOST_TEST_CONTEXT("for inp = `"<<inp<<"` and expected = "<<expected) {
      b = false;
      BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(inp, operand, ne)) );
      if(b) {
        BOOST_CHECK(b = (nullptr != ne));
        if(b) {
          BOOST_TEST(ne->eval(st) == expected);
          BOOST_CHECK(b = (bool) ne->constValue());
          if(b) BOOST_TEST(*ne->constValue() == expected);
        }
      }
    }
  }

  // expressions containing variables
  {st = SymbolsTable{{"abc", 10.}};
    shared_ptr<const NumericExpr> ne; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "%abc%", // value of abc
    operand, ne)));
    if(b) {
      BOOST_CHECK(b = (nullptr != ne));
      if(b) {
        BOOST_TEST(ne->eval(st) == 10.);
        BOOST_CHECK(! ne->constValue());
      }
    }}
  {st = SymbolsTable{{"abc", 10.}};
    shared_ptr<const NumericExpr> ne; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "add(%abc%, 3)", // abc + 3
    operand, ne)));
    if(b) {
      BOOST_CHECK(b = (nullptr != ne));
      if(b) {
        BOOST_TEST(ne->eval(st) == 13.);
        BOOST_CHECK(! ne->constValue());
      }
    }}
  {st = SymbolsTable{{"a", 10}, {"b", 3}, {"c", 2}};
    shared_ptr<const NumericExpr> ne; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "((add(%a%, %b%) mod %a%) mod %c%)", // mod(mod(a+b, a), c)
    operand, ne)));
    if(b) {
      BOOST_CHECK(b = (nullptr != ne));
      if(b) {
        BOOST_TEST(ne->eval(st) == 1.);
        BOOST_CHECK(! ne->constValue());
      }
    }}
}
// Operand ::= UnfencedOperand | \( \s* LeftRecursiveMathExpr \s* \)
BOOST_DATA_TEST_CASE(operandRule_wrongInput,
    bdata::make(vector<string>{
      "(0.1", "add(0,1))", "(%abc%", "%abc% mod %abc%)"}),
    wrongInput) {
  using namespace grammar;

  shared_ptr<const NumericExpr> ne;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, operand, ne)) );
}

// LeftRecursiveMathExpr ::= Operand \s+ mod \s+ Operand
BOOST_AUTO_TEST_CASE(leftRecursiveMathExprRule_correctInput) {
  using namespace grammar;
  using std::string;

  bool b;
  SymbolsTable st;

  // expressions evaluated as constants
  const unordered_map<string, double> inpPlusExpected {
    {"add(10, 3) mod 10", 3.}, // mod(10+3, 10) = 3
    {"(add(10, 3) mod 10) mod 2", 1.} // mod( mod(10+3, 10), 2) = 1
  };
  for(const auto &ioPair : inpPlusExpected) {
    shared_ptr<const NumericExpr> ne;
    const string &inp = ioPair.first;
    const double expected = ioPair.second;
    BOOST_TEST_CONTEXT("for inp = `"<<inp<<"` and expected = "<<expected) {
      b = false;
      BOOST_CHECK_NO_THROW(
        BOOST_CHECK(b = tackle(inp, leftRecursiveMathExpr, ne)) );
      if(b) {
        BOOST_CHECK(b = (nullptr != ne));
        if(b) {
          BOOST_TEST(ne->eval(st) == expected);
          BOOST_CHECK(b = (bool) ne->constValue());
          if(b) BOOST_TEST(*ne->constValue() == expected);
        }
      }
    }
  }

  // expressions containing variables
  {st = SymbolsTable{{"a", 10}, {"b", 3}, {"c", 2}};
    shared_ptr<const NumericExpr> ne; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "(add(%a%, %b%) mod %a%) mod %c%", // mod(mod(a+b, a), c)
    leftRecursiveMathExpr, ne)));
    if(b) {
      BOOST_CHECK(b = (nullptr != ne));
      if(b) {
        BOOST_TEST(ne->eval(st) == 1.);
        BOOST_CHECK(! ne->constValue());
      }
    }}
}
// LeftRecursiveMathExpr ::= Operand \s+ mod \s+ Operand
BOOST_DATA_TEST_CASE(leftRecursiveMathExprRule_wrongInput,
    bdata::make(vector<string>{
      "0.1", "add(0,1)", "mod(0,1)", "mod 10", "5 mod", "%abc%",
      "%abc% mod %abc%)", "3.2 mod 3", "5 mod 3.1", "5 % 3", "mod(5, 3)",
      "5 mod 0", "0 mod 0", "10 mod3", "(10) mod 3", "10 mod (3)", "10mod 3",
      "10mod3", "10 mod%a%", "%a%mod 3", "%a%mod%a%", "(%a%) mod 3",
      "10 mod (%a%)"}),
    wrongInput) {
  using namespace grammar;

  shared_ptr<const NumericExpr> ne;
  BOOST_CHECK_NO_THROW(
    BOOST_CHECK( ! tackle(wrongInput, leftRecursiveMathExpr, ne)) );
}

// MathExpr ::= LeftRecursiveMathExpr | UnfencedOperand
BOOST_AUTO_TEST_CASE(mathExprRule_correctInput) {
  using namespace grammar;

  bool b;
  SymbolsTable st;

  // expressions evaluated as constants
  {shared_ptr<const NumericExpr> ne; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "(add(10, 3) mod 10) mod 2", // mod(mod(10+3, 10), 2) = 1
    mathExpr, ne)));
    if(b) {
      BOOST_CHECK(b = (nullptr != ne));
      if(b) {
        BOOST_TEST(ne->eval(st) == 1.);
        BOOST_CHECK(b = (bool) ne->constValue());
        if(b) BOOST_TEST(*ne->constValue() == 1.);
      }
    }}

  // expressions containing variables
  {st = SymbolsTable{{"a", 10}, {"b", 3}, {"c", 2}};
    shared_ptr<const NumericExpr> ne; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "(add(%a%, %b%) mod %a%) mod %c%", // mod(mod(a+b, a), c)
    mathExpr, ne)));
    if(b) {
      BOOST_CHECK(b = (nullptr != ne));
      if(b) {
        BOOST_TEST(ne->eval(st) == 1.);
        BOOST_CHECK(! ne->constValue());
      }
    }}
}
// MathExpr ::= LeftRecursiveMathExpr | UnfencedOperand
BOOST_DATA_TEST_CASE(mathExprRule_wrongInput,
    bdata::make(vector<string>{
      "(1)", "(%a%)", "(add(1,2))", "40 mod 17 mod 3", "40 (mod 17 mod 3)",
      "%a% mod %b% mod %c%"}),
    wrongInput) {
  using namespace grammar;

  shared_ptr<const NumericExpr> ne;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, mathExpr, ne)) );
}

// ValueOrRange ::= MathExpr (?!(\s+ \.\.)) | MathExpr \s+ \.\. \s+ MathExpr
BOOST_AUTO_TEST_CASE(valueOrRangeRule_correctInput) {
  using namespace grammar;

  bool b;
  SymbolsTable st;

  {shared_ptr<const ValueOrRange> vr; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "add(1,1) .. add(0, 5 mod 3)", // range: 1+1 up to 0+mod(5,3) -> from 2 to 2
    valueOrRange, vr)));
    if(b) {
      BOOST_CHECK(b = (nullptr != vr));
      if(b) {
        BOOST_CHECK(b = vr->isRange());
        if(b) BOOST_CHECK_NO_THROW({
                const auto range = vr->range(st);
                BOOST_TEST(range.first == 2.);
                BOOST_TEST(range.second == 2.);
              });

      }
    }}
  {shared_ptr<const ValueOrRange> vr; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "add(0, 5 mod 3)", // value: 0+mod(5,3) = 2
    valueOrRange, vr)));
    if(b) {
      BOOST_CHECK(b = (nullptr != vr));
      if(b) {
        BOOST_CHECK(b = ! vr->isRange());
        if(b) BOOST_CHECK_NO_THROW( BOOST_TEST(vr->value(st) == 2.) );
      }
    }}

  {st = SymbolsTable{{"a", 10}, {"b", 3}, {"c", 2}};
    shared_ptr<const ValueOrRange> vr; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "5 .. add(%a%, %b% mod %c%)", // range from 5 up to a+mod(b,c)
    valueOrRange, vr)));
    if(b) {
      BOOST_CHECK(b = (nullptr != vr));
      if(b) {
        BOOST_CHECK(b = vr->isRange());
        if(b) BOOST_CHECK_NO_THROW({
                const auto range = vr->range(st);
                BOOST_TEST(range.first == 5.);
                BOOST_TEST(range.second == 11.);
              });
      }
    }}
}
// ValueOrRange ::= MathExpr (?!(\s+ \.\.)) | MathExpr \s+ \.\. \s+ MathExpr
BOOST_DATA_TEST_CASE(valueOrRangeRule_wrongInput,
    bdata::make(vector<string>{
      "%a", "(%a%)", "a", "2..3", "2 ..3", "2.. 3", "2. .3", "2 .. 1",
      "(%a%) .. 1", "%a% .. (5 mod 2)"}),
    wrongInput) {
  using namespace grammar;

  shared_ptr<const ValueOrRange> vr;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, valueOrRange, vr)) );
}

// ValueSet ::= \s* (ValueOrRange (\s* , \s* ValueOrRange)* \s* | eps)
BOOST_AUTO_TEST_CASE(valueSetRule_correctInput) {
  using namespace grammar;

  bool b;
  SymbolsTable st;

  {ValueSet vs; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "    ", // empty value set
    valueSet, vs)));
    if(b) BOOST_CHECK(vs.empty());}
  {ValueSet vs; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    // values: {14, from 1+1 to 0+mod(5,3), from 9 to 12} = {14, 2..2, 9..12}
    "  14  , add(1,1) .. add(0, 5 mod 3), 9 .. 12  ",
    valueSet, vs)));
    if(b) {
      BOOST_CHECK(b = (vs.vors.size() == 3ULL));
      if(b) {
        BOOST_CHECK(b = ! vs.vors[0].isRange());
        if(b) BOOST_CHECK_NO_THROW( BOOST_TEST(vs.vors[0].value(st) == 14.) );

        BOOST_CHECK(b = vs.vors[1].isRange());
        if(b) BOOST_CHECK_NO_THROW({
          const auto range = vs.vors[1].range(st);
          BOOST_TEST(range.first == 2.);
          BOOST_TEST(range.second == 2.);
        });

        BOOST_CHECK(b = vs.vors[2].isRange());
        if(b) BOOST_CHECK_NO_THROW({
          const auto range = vs.vors[2].range(st);
          BOOST_TEST(range.first == 9.);
          BOOST_TEST(range.second == 12.);
        });
      }
    }}

  {st = SymbolsTable{{"a", 10}, {"b", 3}, {"c", 2}};
    ValueSet vs; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    // Values: {b+c, from 5 to a+mod(b,c)} = {b+c, 5..a+mod(b,c)} = {5, 5..11}
    " add(%b%, %c%),5 .. add(%a%, %b% mod %c%)  ",
    valueSet, vs)));
    if(b) {
      BOOST_CHECK(b = (vs.vors.size() == 2ULL));
      if(b) {
        BOOST_CHECK(b = ! vs.vors[0].isRange());
        if(b) BOOST_CHECK_NO_THROW( BOOST_TEST(vs.vors[0].value(st) == 5.) );

        BOOST_CHECK(b = vs.vors[1].isRange());
        if(b) BOOST_CHECK_NO_THROW({
          const auto range = vs.vors[1].range(st);
          BOOST_TEST(range.first == 5.);
          BOOST_TEST(range.second == 11.);
        });
      }
    }}
}
// ValueSet ::= \s* (ValueOrRange (\s* , \s* ValueOrRange)* \s* | eps)
BOOST_DATA_TEST_CASE(valueSetRule_wrongInput,
    bdata::make(vector<string>{
      "4 5", "4, 5,", "4, 5 ..", "%a% .. 5,"}),
    wrongInput) {
  using namespace grammar;

  ValueSet vs;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, valueSet, vs)) );
}

/*
  Condition ::= true | false |
    (MathExpr | \( \s* MathExpr \s* \)) \s+ (not \s+)? in \s* { ValueSet }
*/
BOOST_AUTO_TEST_CASE(conditionRule_correctInput) {
  using namespace grammar;

  bool b;

  {const SymbolsTable st;
    shared_ptr<const LogicalExpr> le; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "true",
    condition, le)));
    if(b) {
      BOOST_CHECK(b = (nullptr != le));
      if(b) {
        BOOST_CHECK_NO_THROW( BOOST_CHECK(le->eval(st)) );
        BOOST_CHECK(b = (bool) le->constValue());
        if(b) BOOST_CHECK(*le->constValue());
      }
    }}
  {const SymbolsTable st;
    shared_ptr<const LogicalExpr> le; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    "false",
    condition, le)));
    if(b) {
      BOOST_CHECK(b = (nullptr != le));
      if(b) {
        BOOST_CHECK_NO_THROW( BOOST_CHECK( ! le->eval(st)) );
        BOOST_CHECK(b = (bool) le->constValue());
        if(b) BOOST_CHECK( ! *le->constValue());
      }
    }}

  {const SymbolsTable st{{"a", 10}, {"b", 13}, {"c", 2}};
    shared_ptr<const LogicalExpr> le; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    // (a-6) outside {b+c, 5..a+mod(b,c)} = 4 outside {15, 5..11} = true
    "add(%a%, -6) not in{ add(%b%, %c%),5 .. add(%a%, %b% mod %c%)  }",
    condition, le)));
    if(b) {
      BOOST_CHECK(b = (nullptr != le));
      if(b) {
        BOOST_CHECK_NO_THROW( BOOST_CHECK(le->eval(st)) );
        BOOST_CHECK( ! le->constValue());
      }
    }}
}
/*
  Condition ::= true | false |
    (MathExpr | \( \s* MathExpr \s* \)) \s+ (not \s+)? in \s* { ValueSet }
*/
BOOST_DATA_TEST_CASE(conditionRule_wrongInput,
    bdata::make(vector<string>{
      "True", "False", "5in  {5}", "5 in 4 .. 6", "%a%not in {5}",
      "add(%a%,1)in {5}"}),
    wrongInput) {
  using namespace grammar;

  shared_ptr<const LogicalExpr> le;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, condition, le)) );
}

// LogicalExpr ::= (if \s+)? (Condition | not \s* \( \s* Condition \s* \))
BOOST_AUTO_TEST_CASE(logicalExprRule_correctInput) {
  using namespace grammar;
  using std::string;

  bool b;
  SymbolsTable st;

  // expressions evaluated as logical constants
  const unordered_map<string, bool> inpPlusExpected{
    {"if true", true},
    {"if  not(true)", false},
    {"not ( false )", true}
  };
  for(const auto &ioPair : inpPlusExpected) {
    shared_ptr<const LogicalExpr> le;
    const string &inp = ioPair.first;
    const bool expected = ioPair.second;
    BOOST_TEST_CONTEXT(
        "for inp = `"<<inp<<"` and expected = "<<boolalpha<<expected) {
      b = false;
      BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(inp, logicalExpr, le)) );
      if(b) {
        BOOST_CHECK(b = (nullptr != le));
        if(b) {
          BOOST_CHECK_NO_THROW( BOOST_CHECK(le->eval(st) == expected) );
          BOOST_CHECK(b = (bool) le->constValue());
          if(b) BOOST_CHECK(*le->constValue() == expected);
        }
      }
    }
  }

  // logical expressions depending on variables
  {st = SymbolsTable{{"a", 10}, {"b", 13}, {"c", 2}};
    shared_ptr<const LogicalExpr> le; b = false;
    BOOST_CHECK_NO_THROW( BOOST_CHECK(b = tackle(
    // (a-6) inside {b+c, 5..a+mod(b,c)} = 4 inside {15, 5..11} = false
    "if not(add(%a%, -6) not in{ add(%b%, %c%),5 .. add(%a%, %b% mod %c%)  })",
    logicalExpr, le)));
    if(b) {
      BOOST_CHECK(b = (nullptr != le));
      if(b) {
        BOOST_CHECK_NO_THROW( BOOST_CHECK( ! le->eval(st)) );
        BOOST_CHECK( ! le->constValue());
      }
    }}
}
// LogicalExpr ::= (if \s+)? (Condition | not \s* \( \s* Condition \s* \))
BOOST_DATA_TEST_CASE(logicalExprRule_wrongInput,
    bdata::make(vector<string>{
      "if(true)", "if(not(true))", "not false"}),
    wrongInput) {
  using namespace grammar;

  shared_ptr<const LogicalExpr> le;
  BOOST_CHECK_NO_THROW( BOOST_CHECK( ! tackle(wrongInput, logicalExpr, le)) );
}

BOOST_AUTO_TEST_SUITE_END()

#endif // for CONFIG_PARSER_CPP and UNIT_TESTING
