/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#if !defined CPP_CONFIG_CONSTRAINT || !defined UNIT_TESTING

#error \
    "Please include this file only within `configConstraint.cpp` \
after a `#define CPP_CONFIG_CONSTRAINT` and surrounding the include \
and the define by `#ifdef UNIT_TESTING`!"

#else  // for CPP_CONFIG_CONSTRAINT and UNIT_TESTING

#include "entity.h"
#include "mathRelated.h"
#include "transferredLoadExt.h"

#include <cmath>
#include <tuple>

#include <boost/test/unit_test.hpp>

using std::ignore;

BOOST_AUTO_TEST_SUITE(configConstraint, *boost::unit_test::tolerance(rc::Eps))

BOOST_AUTO_TEST_CASE(numericConst_usecases) {
  bool b{};
  constexpr double d{4.5};

  const rc::cond::NumericConst c{d};
  BOOST_CHECK(!c.dependsOnVariable(""));
  BOOST_CHECK(b = (bool)c.constValue());
  if (b)
    BOOST_TEST(*c.constValue() == d);
  BOOST_CHECK_NO_THROW(BOOST_TEST(c.eval({}) == d));
}

BOOST_AUTO_TEST_CASE(numericVariable_usecases) {
  constexpr double d{4.5};
  const std::string var{"a"};
  const rc::SymbolsTable st{{var, d}};

  const rc::cond::NumericVariable v{var};
  BOOST_CHECK(!v.dependsOnVariable(""));
  BOOST_CHECK(v.dependsOnVariable(var));
  BOOST_CHECK(!v.constValue());
  BOOST_CHECK_THROW(ignore = v.eval({}), std::out_of_range);
  BOOST_CHECK_NO_THROW(BOOST_TEST(v.eval(st) == d));
}

BOOST_AUTO_TEST_CASE(addition_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::cond;

  bool b{};
  constexpr double d1{4.5}, d2{2.5}, d{d1 + d2};
  const string var1{"a"}, var2{"b"};
  const SymbolsTable emptySt, st{{var1, d1}, {var2, d2}};

  const shared_ptr<const NumericExpr> c1{make_shared<const NumericConst>(d1)},
      c2{make_shared<const NumericConst>(d2)},
      v1{make_shared<const NumericVariable>(var1)},
      v2{make_shared<const NumericVariable>(var2)};

  // adding 2 constants
  try {
    const Addition a{c1, c2};
    BOOST_CHECK(!a.dependsOnVariable(var1));
    BOOST_CHECK(!a.dependsOnVariable(var2));
    BOOST_CHECK(b = (bool)a.constValue());
    if (b)
      BOOST_TEST(*a.constValue() == d);
    BOOST_CHECK_NO_THROW(BOOST_TEST(a.eval(emptySt) == d));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // adding a constant to a variable
  try {
    const Addition a{c1, v2};
    BOOST_CHECK(!a.dependsOnVariable(var1));
    BOOST_CHECK(a.dependsOnVariable(var2));
    BOOST_CHECK(!a.constValue());
    BOOST_CHECK_THROW(ignore = a.eval(emptySt), out_of_range);
    BOOST_CHECK_NO_THROW(BOOST_TEST(a.eval(st) == d));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {
    const Addition a{v1, c2};
    BOOST_CHECK(a.dependsOnVariable(var1));
    BOOST_CHECK(!a.dependsOnVariable(var2));
    BOOST_CHECK(!a.constValue());
    BOOST_CHECK_THROW(ignore = a.eval(emptySt), out_of_range);
    BOOST_CHECK_NO_THROW(BOOST_TEST(a.eval(st) == d));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // adding 2 variables
  try {
    const Addition a{v1, v2};
    BOOST_CHECK(a.dependsOnVariable(var1));
    BOOST_CHECK(a.dependsOnVariable(var2));
    BOOST_CHECK(!a.constValue());
    BOOST_CHECK_THROW(ignore = a.eval(emptySt), out_of_range);
    BOOST_CHECK_THROW(ignore = a.eval({{var1, d1}}), out_of_range);
    BOOST_CHECK_THROW(ignore = a.eval({{var2, d2}}), out_of_range);
    BOOST_CHECK_NO_THROW(BOOST_TEST(a.eval(st) == d));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(modulus_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::cond;

  bool b{};
  constexpr double d1{5.}, d2{3.}, d{(double)((long)d1 % (long)d2)},
      badVal{2.3};
  const string var1{"a"}, var2{"b"};
  const SymbolsTable emptySt, partialSt1{{var1, d1}}, partialSt2{{var2, d2}},
      st{{var1, d1}, {var2, d2}};

  const shared_ptr<const NumericExpr> c1{make_shared<const NumericConst>(d1)},
      c2{make_shared<const NumericConst>(d2)},
      badCt{make_shared<const NumericConst>(badVal)},
      zero{make_shared<const NumericConst>(0.)},
      one{make_shared<const NumericConst>(1.)},
      minusOne{make_shared<const NumericConst>(-1.)},
      v1{make_shared<const NumericVariable>(var1)},
      v2{make_shared<const NumericVariable>(var2)};

  // non-integer arguments
  BOOST_CHECK_THROW(Modulus(c1, badCt), logic_error);
  BOOST_CHECK_THROW(Modulus(badCt, v2), logic_error);
  BOOST_CHECK_THROW(Modulus(badCt, badCt), logic_error);
  try {
    const Modulus m{v1, v2};
    BOOST_CHECK(m.dependsOnVariable(var1));
    BOOST_CHECK(m.dependsOnVariable(var2));
    BOOST_CHECK_THROW(ignore = m.eval(SymbolsTable{{var1, badVal}, {var2, d2}}),
                      logic_error);
    BOOST_CHECK_THROW(ignore = m.eval(SymbolsTable{{var1, d1}, {var2, badVal}}),
                      logic_error);
    BOOST_CHECK_THROW(
        ignore = m.eval(SymbolsTable{{var1, badVal}, {var2, badVal}}),
        logic_error);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // denominator or both arguments are 0
  BOOST_CHECK_THROW(Modulus m(c1, zero), overflow_error);
  BOOST_CHECK_THROW(Modulus m(zero, zero), logic_error);
  try {
    const Modulus m{v1, v2};
    BOOST_CHECK(m.dependsOnVariable(var1));
    BOOST_CHECK(m.dependsOnVariable(var2));
    BOOST_CHECK_THROW(ignore = m.eval(SymbolsTable{{var1, d1}, {var2, 0.}}),
                      overflow_error);
    BOOST_CHECK_THROW(ignore = m.eval(SymbolsTable{{var1, 0.}, {var2, 0.}}),
                      logic_error);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // denominator 1 or -1 means result 0
  try {  // const numerator
    const Modulus m{c1, one};
    BOOST_CHECK(!m.dependsOnVariable(var1));
    BOOST_CHECK(!m.dependsOnVariable(var2));
    BOOST_CHECK(b = (bool)m.constValue());
    if (b)
      BOOST_TEST(!*m.constValue());
    BOOST_CHECK_NO_THROW(BOOST_TEST(!m.eval(emptySt)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // the numerator is a variable
    const Modulus m{v1, minusOne};
    BOOST_CHECK(!m.dependsOnVariable(var1));
    BOOST_CHECK(!m.dependsOnVariable(var2));
    BOOST_CHECK(b = (bool)m.constValue());
    if (b)
      BOOST_TEST(!*m.constValue());
    BOOST_CHECK_NO_THROW(BOOST_TEST(!m.eval(emptySt)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // modulus between 2 constants
  try {
    const Modulus m{c1, c2};
    BOOST_CHECK(!m.dependsOnVariable(var1));
    BOOST_CHECK(!m.dependsOnVariable(var2));
    BOOST_CHECK(b = (bool)m.constValue());
    if (b)
      BOOST_TEST(*m.constValue() == d);
    BOOST_CHECK_NO_THROW(BOOST_TEST(m.eval(emptySt) == d));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // modulus between a constant and a variable
  try {
    const Modulus m{c1, v2};
    BOOST_CHECK(!m.dependsOnVariable(var1));
    BOOST_CHECK(m.dependsOnVariable(var2));
    BOOST_CHECK(!m.constValue());
    BOOST_CHECK_THROW(ignore = m.eval(emptySt), out_of_range);
    BOOST_CHECK_NO_THROW(BOOST_TEST(m.eval(st) == d));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {
    const Modulus m{v1, c2};
    BOOST_CHECK(m.dependsOnVariable(var1));
    BOOST_CHECK(!m.dependsOnVariable(var2));
    BOOST_CHECK(!m.constValue());
    BOOST_CHECK_THROW(ignore = m.eval(emptySt), out_of_range);
    BOOST_CHECK_NO_THROW(BOOST_TEST(m.eval(st) == d));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // modulus between 2 variables
  try {
    const Modulus m{v1, v2};
    BOOST_CHECK(m.dependsOnVariable(var1));
    BOOST_CHECK(m.dependsOnVariable(var2));
    BOOST_CHECK(!m.constValue());
    BOOST_CHECK_THROW(ignore = m.eval(emptySt), out_of_range);
    BOOST_CHECK_THROW(ignore = m.eval(partialSt1), out_of_range);
    BOOST_CHECK_THROW(ignore = m.eval(partialSt2), out_of_range);
    BOOST_CHECK_NO_THROW(BOOST_TEST(m.eval(st) == d));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(valueOrRange_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::cond;

  constexpr auto nanD{numeric_limits<double>::quiet_NaN()};
  BOOST_REQUIRE(isnan(nanD));  // Fails if wrong compilation flags around
                               // '-ffast-math' and NaN handling. See:
                               // 'mathRelated.h'

  bool b{};
  constexpr double d{4.5}, dMin1{d - 1.};
  const string var{"a"};
  const SymbolsTable emptySt, st{{var, d}};
  const shared_ptr<const NumericExpr> c{make_shared<const NumericConst>(d)},
      cMin1{make_shared<const NumericConst>(dMin1)},
      cNaN{make_shared<const NumericConst>(nanD)},
      v{make_shared<const NumericVariable>(var)};

  BOOST_CHECK_THROW(ValueOrRange vor(cNaN), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(c, cNaN), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(v, cNaN), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(cNaN, c), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(cNaN, v), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(cNaN, cNaN), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(c, cMin1), logic_error);  // reverse order

  try {
    const ValueOrRange vor{v};  // variable var='a' from st pointing to value d
    BOOST_CHECK(vor.dependsOnVariable(var));
    BOOST_CHECK(!vor.isConst());
    BOOST_CHECK(b = !vor.isRange());
    BOOST_CHECK_THROW(ignore = vor.range(st), logic_error);
    if (b) {
      BOOST_CHECK_NO_THROW(BOOST_TEST(d == vor.value(st)));
      BOOST_CHECK_THROW(ignore = vor.value(emptySt),
                        out_of_range);  // 'a' not found
      BOOST_CHECK_THROW(ignore = vor.value(SymbolsTable{{var, nanD}}),
                        logic_error);
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {
    const ValueOrRange vor{c};  // constant c
    BOOST_CHECK(!vor.dependsOnVariable(var));
    BOOST_CHECK(vor.isConst());
    BOOST_CHECK(b = !vor.isRange());
    BOOST_CHECK_THROW(ignore = vor.range(st), logic_error);
    if (b)
      BOOST_CHECK_NO_THROW(BOOST_TEST(d == vor.value(emptySt)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {
    const ValueOrRange vor{c, c};  // range degenerated to a single constant d
    BOOST_CHECK(!vor.dependsOnVariable(var));
    BOOST_CHECK(vor.isConst());
    BOOST_CHECK(b = vor.isRange());
    BOOST_CHECK_THROW(ignore = vor.value(st), logic_error);
    if (b)
      try {
        const auto limits = vor.range(emptySt);
        BOOST_TEST(d == limits.first);
        BOOST_TEST(d == limits.second);
      } catch (...) {
        BOOST_CHECK(false);  // Unexpected exception
      }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // range between constants d-1 .. d
    const ValueOrRange vor{cMin1, c};
    BOOST_CHECK(!vor.dependsOnVariable(var));
    BOOST_CHECK(vor.isConst());
    BOOST_CHECK(b = vor.isRange());
    BOOST_CHECK_THROW(ignore = vor.value(st), logic_error);
    if (b)
      try {
        const auto limits = vor.range(emptySt);
        BOOST_TEST(dMin1 == limits.first);
        BOOST_TEST(d == limits.second);
      } catch (...) {
        BOOST_CHECK(false);  // Unexpected exception
      }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {
    // range degenerated to value pointed by variable var='a'
    const ValueOrRange vor{v, v};
    BOOST_CHECK(vor.dependsOnVariable(var));
    BOOST_CHECK(!vor.isConst());
    BOOST_CHECK(b = vor.isRange());
    BOOST_CHECK_THROW(ignore = vor.value(st), logic_error);
    if (b) {
      BOOST_CHECK_THROW(ignore = vor.range(emptySt),
                        out_of_range);  // 'a' not found
      try {
        const auto limits = vor.range(st);
        BOOST_TEST(d == limits.first);
        BOOST_TEST(d == limits.second);
      } catch (...) {
        BOOST_CHECK(false);  // Unexpected exception
      }
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {
    // range between constant d-1 and variable var='a'
    const ValueOrRange vor{cMin1, v};
    BOOST_CHECK(vor.dependsOnVariable(var));
    BOOST_CHECK(!vor.isConst());
    BOOST_CHECK(b = vor.isRange());
    BOOST_CHECK_THROW(ignore = vor.value(st), logic_error);
    if (b) {
      BOOST_CHECK_THROW(ignore = vor.range(emptySt),
                        out_of_range);  // 'a' not found
      BOOST_CHECK_THROW(ignore = vor.range(SymbolsTable{{var, nanD}}),
                        logic_error);
      try {
        const auto limits = vor.range(st);
        BOOST_TEST(dMin1 == limits.first);
        BOOST_TEST(d == limits.second);
      } catch (...) {
        BOOST_CHECK(false);  // Unexpected exception
      }
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // range between variable var='a' and constant d-1
    const ValueOrRange vor{v, cMin1};
    BOOST_CHECK(vor.dependsOnVariable(var));
    BOOST_CHECK(!vor.isConst());
    BOOST_CHECK(b = vor.isRange());
    BOOST_CHECK_THROW(ignore = vor.value(st), logic_error);
    if (b) {
      BOOST_CHECK_THROW(ignore = vor.range(emptySt),
                        out_of_range);  // 'a' not found
      BOOST_CHECK_THROW(ignore = vor.range(SymbolsTable{{var, nanD}}),
                        logic_error);
      BOOST_CHECK_THROW(ignore = vor.range(st),
                        logic_error);  // reverse order
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(valueSet_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::cond;

  constexpr double d{2.4}, d1{5.1}, d2{6.8}, missingD{8.1};
  const string var{"a"};
  const SymbolsTable emptySt, st{{var, d1}};  // a := d1
  const shared_ptr<const NumericExpr> c{make_shared<const NumericConst>(d)},
      c1{make_shared<const NumericConst>(d1)},
      c2{make_shared<const NumericConst>(d2)},
      v{make_shared<const NumericVariable>(var)};
  ValueSet vs;
  BOOST_CHECK(vs.empty());
  BOOST_CHECK(vs.constSet());
  BOOST_CHECK(!vs.dependsOnVariable(var));

  vs.add(ValueOrRange{c});  // hold value d
  BOOST_CHECK(!vs.empty());
  BOOST_CHECK(vs.constSet());
  BOOST_CHECK(!vs.dependsOnVariable(var));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(vs.contains(d, emptySt)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!vs.contains(d1, emptySt)));

  vs.add({v, c2});  // hold range 'a' .. d2
  BOOST_CHECK(!vs.constSet());
  BOOST_CHECK(vs.dependsOnVariable(var));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!vs.contains(missingD, st)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(vs.contains(d, st)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(vs.contains(d1, st)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(vs.contains(d2, st)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(vs.contains((d1 + d2) / 2., st)));
  BOOST_CHECK_THROW(ignore = vs.contains(missingD, emptySt), out_of_range);
  BOOST_CHECK_THROW(ignore = vs.contains(d1, emptySt), out_of_range);
  BOOST_CHECK_THROW(ignore = vs.contains(d2, emptySt), out_of_range);
  BOOST_CHECK_THROW(ignore = vs.contains((d1 + d2) / 2., emptySt),
                    out_of_range);
}

BOOST_AUTO_TEST_CASE(boolConst_usecases) {
  bool b{};
  const rc::SymbolsTable emptySt;

  {
    const rc::cond::BoolConst bc{true};
    BOOST_CHECK(!bc.dependsOnVariable(""));
    BOOST_CHECK_NO_THROW(BOOST_CHECK(bc.eval(emptySt)));
    BOOST_CHECK(b = (bool)bc.constValue());
    if (b)
      BOOST_CHECK(*bc.constValue());
  }
  {
    const rc::cond::BoolConst bc{false};
    BOOST_CHECK(!bc.dependsOnVariable(""));
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!bc.eval(emptySt)));
    BOOST_CHECK(b = (bool)bc.constValue());
    if (b)
      BOOST_CHECK(!*bc.constValue());
  }
}

BOOST_AUTO_TEST_CASE(belongToCondition_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::cond;

  constexpr double d{6.3}, d1{2.3}, d2{4.5}, d3{9.8}, dOut{3.1};
  const string var{"a"}, var1{"b"}, var2{"c"}, var3{"d"}, varOut{"e"};
  const SymbolsTable emptySt,
      st{{var, d}, {var1, d1}, {var2, d2}, {var3, d3}, {varOut, dOut}};

  const shared_ptr<const NumericExpr> c{make_shared<const NumericConst>(d)},
      cOut{make_shared<const NumericConst>(dOut)},
      c1{make_shared<const NumericConst>(d1)},
      c2{make_shared<const NumericConst>(d2)},
      c3{make_shared<const NumericConst>(d3)},
      v{make_shared<const NumericVariable>(var)},
      vOut{make_shared<const NumericVariable>(varOut)},
      v1{make_shared<const NumericVariable>(var1)},
      v2{make_shared<const NumericVariable>(var2)},
      v3{make_shared<const NumericVariable>(var3)};

  auto pSetOfConstants{make_unique<ValueSet>()};
  auto pSetWithVariables{make_unique<ValueSet>()};
  // {d1, d2..d3}
  pSetOfConstants->add(ValueOrRange{c1}).add({c2, c3});

  // {'b', 'c'..d3} = {d1, d2..d3}
  pSetWithVariables->add(ValueOrRange{v1}).add({v2, c3});
  const shared_ptr<const IValues<double> > emptySet{
      make_shared<const ValueSet>()};
  const shared_ptr<const IValues<double> > setOfConstants{
      shared_ptr<const ValueSet>(pSetOfConstants.release())};
  const shared_ptr<const IValues<double> > setWithVariables{
      shared_ptr<const ValueSet>(pSetWithVariables.release())};

  // membership test using only constants
  try {  // d in {}
    const BelongToCondition<double> btc{c, emptySet};
    BOOST_CHECK(!btc.dependsOnVariable(var));
    BOOST_CHECK(!btc.dependsOnVariable(varOut));
    BOOST_CHECK(!btc.dependsOnVariable(var1));
    BOOST_CHECK(!btc.dependsOnVariable(var2));
    BOOST_CHECK(!btc.dependsOnVariable(var3));
    BOOST_CHECK(btc.constValue());
    BOOST_CHECK(!*btc.constValue());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!btc.eval(emptySt)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // d in {d1, d2..d3}
    const BelongToCondition<double> btc{c, setOfConstants};
    BOOST_CHECK(!btc.dependsOnVariable(var));
    BOOST_CHECK(!btc.dependsOnVariable(varOut));
    BOOST_CHECK(!btc.dependsOnVariable(var1));
    BOOST_CHECK(!btc.dependsOnVariable(var2));
    BOOST_CHECK(!btc.dependsOnVariable(var3));
    BOOST_CHECK(btc.constValue());
    BOOST_CHECK(*btc.constValue());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(btc.eval(emptySt)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // dOut in {d1, d2..d3}
    const BelongToCondition<double> btc{cOut, setOfConstants};
    BOOST_CHECK(!btc.dependsOnVariable(var));
    BOOST_CHECK(!btc.dependsOnVariable(varOut));
    BOOST_CHECK(!btc.dependsOnVariable(var1));
    BOOST_CHECK(!btc.dependsOnVariable(var2));
    BOOST_CHECK(!btc.dependsOnVariable(var3));
    BOOST_CHECK(btc.constValue());
    BOOST_CHECK(!*btc.constValue());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!btc.eval(emptySt)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // membership test using variables
  try {  // 'a'(d) in {}
    const BelongToCondition<double> btc{v, emptySet};
    BOOST_CHECK(btc.dependsOnVariable(var));
    BOOST_CHECK(!btc.dependsOnVariable(varOut));
    BOOST_CHECK(!btc.dependsOnVariable(var1));
    BOOST_CHECK(!btc.dependsOnVariable(var2));
    BOOST_CHECK(!btc.dependsOnVariable(var3));
    BOOST_CHECK(!btc.constValue());
    BOOST_CHECK_THROW(ignore = btc.eval(emptySt),
                      out_of_range);  // 'a' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!btc.eval(st)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // 'a'(d) in {d1, d2..d3}
    const BelongToCondition<double> btc{v, setOfConstants};
    BOOST_CHECK(btc.dependsOnVariable(var));
    BOOST_CHECK(!btc.dependsOnVariable(varOut));
    BOOST_CHECK(!btc.dependsOnVariable(var1));
    BOOST_CHECK(!btc.dependsOnVariable(var2));
    BOOST_CHECK(!btc.dependsOnVariable(var3));
    BOOST_CHECK(!btc.constValue());
    BOOST_CHECK_THROW(ignore = btc.eval(emptySt),
                      out_of_range);  // 'a' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(btc.eval(st)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // 'e'(dOut) in {d1, d2..d3}
    const BelongToCondition<double> btc{vOut, setOfConstants};
    BOOST_CHECK(!btc.dependsOnVariable(var));
    BOOST_CHECK(btc.dependsOnVariable(varOut));
    BOOST_CHECK(!btc.dependsOnVariable(var1));
    BOOST_CHECK(!btc.dependsOnVariable(var2));
    BOOST_CHECK(!btc.dependsOnVariable(var3));
    BOOST_CHECK(!btc.constValue());
    BOOST_CHECK_THROW(ignore = btc.eval(emptySt),
                      out_of_range);  // 'e' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!btc.eval(st)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // d in {'b', 'c'..d3} = {d1, d2..d3}
    const BelongToCondition<double> btc{c, setWithVariables};
    BOOST_CHECK(!btc.dependsOnVariable(var));
    BOOST_CHECK(!btc.dependsOnVariable(varOut));
    BOOST_CHECK(btc.dependsOnVariable(var1));
    BOOST_CHECK(btc.dependsOnVariable(var2));
    BOOST_CHECK(!btc.dependsOnVariable(var3));
    BOOST_CHECK(!btc.constValue());
    BOOST_CHECK_THROW(ignore = btc.eval(emptySt),
                      out_of_range);  // 'a' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(btc.eval(st)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // dOut in {'b', 'c'..d3} = {d1, d2..d3}
    const BelongToCondition<double> btc{cOut, setWithVariables};
    BOOST_CHECK(!btc.dependsOnVariable(var));
    BOOST_CHECK(!btc.dependsOnVariable(varOut));
    BOOST_CHECK(btc.dependsOnVariable(var1));
    BOOST_CHECK(btc.dependsOnVariable(var2));
    BOOST_CHECK(!btc.dependsOnVariable(var3));
    BOOST_CHECK(!btc.constValue());
    BOOST_CHECK_THROW(ignore = btc.eval(emptySt),
                      out_of_range);  // 'e' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!btc.eval(st)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // 'a'(d) in {'b', 'c'..d3} = {d1, d2..d3}
    const BelongToCondition<double> btc{v, setWithVariables};
    BOOST_CHECK(btc.dependsOnVariable(var));
    BOOST_CHECK(!btc.dependsOnVariable(varOut));
    BOOST_CHECK(btc.dependsOnVariable(var1));
    BOOST_CHECK(btc.dependsOnVariable(var2));
    BOOST_CHECK(!btc.dependsOnVariable(var3));
    BOOST_CHECK(!btc.constValue());
    BOOST_CHECK_THROW(ignore = btc.eval(emptySt),
                      out_of_range);  // 'a' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(btc.eval(st)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // 'e'(dOut) in {'b', 'c'..d3} = {d1, d2..d3}
    const BelongToCondition<double> btc{vOut, setWithVariables};
    BOOST_CHECK(!btc.constValue());
    BOOST_CHECK(!btc.dependsOnVariable(var));
    BOOST_CHECK(btc.dependsOnVariable(varOut));
    BOOST_CHECK(btc.dependsOnVariable(var1));
    BOOST_CHECK(btc.dependsOnVariable(var2));
    BOOST_CHECK(!btc.dependsOnVariable(var3));
    BOOST_CHECK_THROW(ignore = btc.eval(emptySt),
                      out_of_range);  // 'e' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!btc.eval(st)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(notPredicate_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::cond;

  bool b{};
  const string var{"a"}, varOut{"b"};
  constexpr double d{6.3}, d1{2.3}, d2{4.5}, d3{9.8}, dOut{3.1};
  const SymbolsTable emptySt, st{{var, d}, {varOut, dOut}};

  // Not(bool_constant)
  {
    const Not notFalse{make_shared<const BoolConst>(false)};
    BOOST_CHECK_NO_THROW(BOOST_CHECK(notFalse.eval(emptySt)));
    BOOST_CHECK(!notFalse.dependsOnVariable(var));
    BOOST_CHECK(!notFalse.dependsOnVariable(varOut));
    BOOST_CHECK(b = (bool)notFalse.constValue());
    if (b)
      BOOST_CHECK(*notFalse.constValue());
  }
  {
    const Not notTrue{make_shared<const BoolConst>(true)};
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!notTrue.eval(emptySt)));
    BOOST_CHECK(!notTrue.dependsOnVariable(var));
    BOOST_CHECK(!notTrue.dependsOnVariable(varOut));
    BOOST_CHECK(b = (bool)notTrue.constValue());
    if (b)
      BOOST_CHECK(!*notTrue.constValue());
  }

  // Not(BelongToCondition)
  const shared_ptr<const NumericExpr> c{make_shared<const NumericConst>(d)},
      cOut{make_shared<const NumericConst>(dOut)},
      c1{make_shared<const NumericConst>(d1)},
      c2{make_shared<const NumericConst>(d2)},
      c3{make_shared<const NumericConst>(d3)},
      v{make_shared<const NumericVariable>(var)},
      vOut{make_shared<const NumericVariable>(varOut)};

  ValueSet* const pSetOfConstants{new ValueSet};
  // {d1, d2..d3}
  pSetOfConstants->add(ValueOrRange{c1}).add({c2, c3});
  const shared_ptr<const IValues<double> > setOfConstants{
      shared_ptr<const ValueSet>(pSetOfConstants)};

  // membership test using only constants
  try {  // not(dOut in {d1, d2..d3})
    const Not notFalse{
        make_shared<const BelongToCondition<double> >(cOut, setOfConstants)};
    BOOST_CHECK(!notFalse.dependsOnVariable(var));
    BOOST_CHECK(!notFalse.dependsOnVariable(varOut));
    BOOST_CHECK(notFalse.constValue());
    BOOST_CHECK(*notFalse.constValue());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(notFalse.eval(emptySt)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // not(d in {d1, d2..d3})
    const Not notTrue{
        make_shared<const BelongToCondition<double> >(c, setOfConstants)};
    BOOST_CHECK(!notTrue.dependsOnVariable(var));
    BOOST_CHECK(!notTrue.dependsOnVariable(varOut));
    BOOST_CHECK(notTrue.constValue());
    BOOST_CHECK(!*notTrue.constValue());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!notTrue.eval(emptySt)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // membership test using variables as well
  try {  // not('b'(dOut) in {d1, d2..d3})
    const Not notFalse{
        make_shared<const BelongToCondition<double> >(vOut, setOfConstants)};
    BOOST_CHECK(!notFalse.dependsOnVariable(var));
    BOOST_CHECK(notFalse.dependsOnVariable(varOut));
    BOOST_CHECK(!notFalse.constValue());
    BOOST_CHECK_THROW(ignore = notFalse.eval(emptySt),
                      out_of_range);  // 'b' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(notFalse.eval(st)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // not('a'(d) in {d1, d2..d3})
    const Not notTrue{
        make_shared<const BelongToCondition<double> >(v, setOfConstants)};
    BOOST_CHECK(notTrue.dependsOnVariable(var));
    BOOST_CHECK(!notTrue.dependsOnVariable(varOut));
    BOOST_CHECK(!notTrue.constValue());
    BOOST_CHECK_THROW(ignore = notTrue.eval(emptySt),
                      out_of_range);  // 'a' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!notTrue.eval(st)));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(typesConstraint_usecases) {
  using namespace std;
  using namespace rc::cond;
  using namespace rc::ent;

  bool b{};
  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  shared_ptr<const AllEntities> spAe{pAe.release()};
  MovingEntities me{spAe};
  BankEntities be{spAe};
  TypesConstraint tc;
  BOOST_CHECK(!tc.longestMatchLength());
  BOOST_CHECK(tc.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(tc.validate(ae));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(me)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be)));

  try {  // Available: 2 x t0 + 2 x t1 + 2 x t2 + 2 x t3
    ae += make_shared<const Entity>(0U, "e0", "t0", false, "true", 1.);
    ae += make_shared<const Entity>(1U, "e1", "t0", false, "false", 2.);
    ae += make_shared<const Entity>(2U, "e2", "t1", false, "true", 3.);
    ae += make_shared<const Entity>(3U, "e3", "t1", false, "false", 4.);
    ae += make_shared<const Entity>(4U, "e4", "t2", false, "true", 5.);
    ae += make_shared<const Entity>(5U, "e5", "t2", false, "false", 6.);
    ae += make_shared<const Entity>(6U, "e6", "t3", false, "true", 7.);
    ae += make_shared<const Entity>(7U, "e7", "t3", false, "true", 1.);
  } catch (...) {
    BOOST_REQUIRE(false);  // Unexpected exception
  }

  // Creating tc1 as t0 + t1 + t2 + t3, which has min weight 10. and min
  // capacity 4
  try {
    TypesConstraint tc1;
    tc1.addTypeRange("t0", 1U, 1U)
        .addTypeRange("t1", 1U, 1U)
        .addTypeRange("t2", 1U, 1U)
        .addTypeRange("t3", 1U, 1U);
    const MaxLoadValidatorExt maxLoad10{10.};
    tc1.validate(ae, 4U, maxLoad10);
    BOOST_CHECK_THROW(tc1.validate(ae, 3U, maxLoad10),
                      logic_error);  // capacity = 3 < 4
    BOOST_CHECK_THROW(tc1.validate(ae, 4U, MaxLoadValidatorExt{9.}),
                      logic_error);  // min load = 10 > 9
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // Below working only on tc

  BOOST_CHECK_NO_THROW(tc.validate(ae));

  BOOST_CHECK_THROW(tc.addTypeRange("t0", 5U, 3U),
                    logic_error);  // reversed range limits

  // exactly 1 't1' is required
  BOOST_CHECK_NO_THROW(tc.addTypeRange("t1", 1U, 1U));
  BOOST_CHECK(tc.longestMatchLength() == 1U);
  BOOST_CHECK(tc.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(tc.validate(ae));

  BOOST_CHECK_THROW(tc.validate(ae, 0U),
                    logic_error);  // asking for 1 't1' when capacity is 0
  BOOST_CHECK_THROW(
      tc.validate(ae, 1U, MaxLoadValidatorExt{2.}),
      logic_error);  // asking for 1 't1' (weighing >= 3) when maxLoad is 2

  BOOST_CHECK_THROW(tc.addTypeRange("t1"),
                    logic_error);  // duplicate type

  // Empty configurations don't match any longer
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!tc.matches(me)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!tc.matches(be)));

  // Entities 2 and 3 (of type 't1') match. The rest don't
  BOOST_REQUIRE_NO_THROW(me = {2U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(me)));
  BOOST_REQUIRE_NO_THROW(be = {3U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be)));
  BOOST_REQUIRE_NO_THROW(me = {0U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!tc.matches(me)));
  BOOST_REQUIRE_NO_THROW(be = {1U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!tc.matches(be)));
  BOOST_REQUIRE_NO_THROW(me = {4U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!tc.matches(me)));
  BOOST_REQUIRE_NO_THROW(be = {5U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!tc.matches(be)));
  BOOST_REQUIRE_NO_THROW(me = {6U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!tc.matches(me)));

  // There's no match when there are more than 1 't1' entities
  BOOST_REQUIRE_NO_THROW(be = vector({2U, 3U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!tc.matches(be)));

  try {  // apart from the 't1', expect also an _unknown_ 't' type
    const auto clonedC = tc.clone();
    TypesConstraint* clonedTc{dynamic_cast<TypesConstraint*>(
        const_cast<IConfigConstraint*>(clonedC.get()))};
    b = false;
    BOOST_CHECK(b = (bool)clonedTc);
    if (b) {
      BOOST_CHECK_NO_THROW(clonedTc->addTypeRange("t"));
      BOOST_CHECK(clonedTc->longestMatchLength() == UINT_MAX);
      BOOST_CHECK(clonedTc->longestMismatchLength() == UINT_MAX);
      BOOST_CHECK_THROW(clonedTc->validate(ae), logic_error);
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // apart from the 't1', expect also 3 't0's (only 2 are available)
    const auto clonedC = tc.clone();
    TypesConstraint* clonedTc{dynamic_cast<TypesConstraint*>(
        const_cast<IConfigConstraint*>(clonedC.get()))};
    b = false;
    BOOST_CHECK(b = (bool)clonedTc);
    if (b) {
      BOOST_CHECK_NO_THROW(clonedTc->addTypeRange("t0", 3U, 3U));
      BOOST_CHECK(clonedTc->longestMatchLength() == 4U);
      BOOST_CHECK(clonedTc->longestMismatchLength() == UINT_MAX);
      BOOST_CHECK_THROW(clonedTc->validate(ae), logic_error);
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // at least 2 't0' are required (and the 't1' from before) and 1 't2' is
  // optional
  BOOST_CHECK_NO_THROW(tc.addTypeRange("t2", 0U, 1U).validate(ae));
  BOOST_CHECK(tc.longestMatchLength() == 2U);
  BOOST_CHECK(tc.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(tc.addTypeRange("t0", 2U).validate(ae));
  BOOST_CHECK(tc.longestMatchLength() == UINT_MAX);
  BOOST_CHECK(tc.longestMismatchLength() == UINT_MAX);

  // There's a match for the following sets
  BOOST_REQUIRE_NO_THROW(be = vector({0U, 1U, 2U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be)));  // 2 x t0 + t1
  BOOST_REQUIRE_NO_THROW(be = vector({0U, 1U, 3U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be)));  // 2 x t0 + t1
  BOOST_REQUIRE_NO_THROW(be = vector({0U, 1U, 2U, 4U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be)));  // 2 x t0 + t1 + t2
  BOOST_REQUIRE_NO_THROW(be = vector({0U, 1U, 3U, 4U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be)));  // 2 x t0 + t1 + t2
  BOOST_REQUIRE_NO_THROW(be = vector({0U, 1U, 2U, 5U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be)));  // 2 x t0 + t1 + t2
  BOOST_REQUIRE_NO_THROW(be = vector({0U, 1U, 3U, 5U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be)));  // 2 x t0 + t1 + t2

  // Other sets produce no match
  BOOST_REQUIRE_NO_THROW(be = vector({0U, 2U}));          // t0 + t1
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!tc.matches(be)));     // missing 't0'
  BOOST_REQUIRE_NO_THROW(be = vector({0U, 1U, 2U, 3U}));  // 2 x t0 + 2 x t1
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!tc.matches(be)));     // extra 't1'
  BOOST_REQUIRE_NO_THROW(
      be = vector({0U, 1U, 2U, 4U, 5U}));              // 2 x t0 + t1 + 2 x t2
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!tc.matches(be)));  // extra 't2'
  BOOST_REQUIRE_NO_THROW(be = vector({0U, 1U, 2U, 6U}));  // 2 x t0 + t1 + t3
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!tc.matches(be)));     // unwanted type 't3'
}

BOOST_AUTO_TEST_CASE(idsConstraint_usecases) {
  using namespace std;
  using namespace rc::cond;
  using namespace rc::ent;

  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  shared_ptr<const AllEntities> spAe{pAe.release()};
  MovingEntities me{spAe};
  BankEntities be{spAe};
  IdsConstraint ic;
  BOOST_CHECK(!ic.longestMatchLength());
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(ae));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(be)));

  try {  // Available: 2 x t0 + 2 x t1 + 2 x t2 + t3 + t4 + t5
    ae += make_shared<const Entity>(0U, "e0", "t0", false, "true");
    ae += make_shared<const Entity>(1U, "e1", "t0", false, "false");
    ae += make_shared<const Entity>(2U, "e2", "t1", false, "true");
    ae += make_shared<const Entity>(3U, "e3", "t1", false, "false");
    ae += make_shared<const Entity>(4U, "e4", "t2", false, "true");
    ae += make_shared<const Entity>(5U, "e5", "t2", false, "false");
    ae += make_shared<const Entity>(6U, "e6", "t3", false, "true");
    ae += make_shared<const Entity>(7U, "e7", "t4", false, "true");
    ae += make_shared<const Entity>(8U, "e8", "t5", false, "true");
  } catch (...) {
    BOOST_REQUIRE(false);  // Unexpected exception
  }

  // the mandatory ids exceed the available ones
  {
    IdsConstraint ic1;
    for (unsigned i{}; i < 50U; ++i)
      ic1.addUnspecifiedMandatory();
    BOOST_CHECK(ic1.longestMatchLength() == 50U);
    BOOST_CHECK(ic1.longestMismatchLength() == UINT_MAX);
    BOOST_CHECK_THROW(ic1.validate(ae), logic_error);

    ic1.setUnbounded();
    BOOST_CHECK(ic1.longestMatchLength() == UINT_MAX);
    BOOST_CHECK(ic1.longestMismatchLength() == 49U);
  }

  // A single unspecified mandatory id matches any id, but not empty sets nor
  // more than 1 id
  {
    MovingEntities me1{spAe};
    IdsConstraint ic1;
    ic1.addUnspecifiedMandatory();
    BOOST_CHECK_NO_THROW(ic1.validate(ae));
    BOOST_CHECK(ic1.longestMatchLength() == 1U);
    BOOST_CHECK(ic1.longestMismatchLength() == UINT_MAX);
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!ic1.matches(me)));
    const size_t lim{ae.count()};
    for (size_t i{}; i < lim; ++i)
      BOOST_TEST_CONTEXT("for id = `" << i << '`') {
        BOOST_REQUIRE_NO_THROW(me1 = {(unsigned)i});
        BOOST_CHECK_NO_THROW(BOOST_CHECK(ic1.matches(me1)));
      }
    BOOST_REQUIRE_NO_THROW(me1 = vector({0U, 1U}));
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!ic1.matches(me1)));
  }

  BOOST_CHECK_NO_THROW(ic.validate(ae));  // no id constraints yet

  // Setting id 0 as optional
  BOOST_CHECK_NO_THROW(ic.addOptionalId(0U));
  BOOST_CHECK(ic.longestMatchLength() == 1U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(ae));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));  // empty config matches
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(be)));  // empty config matches
  BOOST_REQUIRE_NO_THROW(me = {0U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));  // id 0 matches
  BOOST_REQUIRE_NO_THROW(me = {1U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!ic.matches(me)));  // 1 doesn't match

  // cannot reuse id 0
  BOOST_CHECK_THROW(ic.addOptionalId(0U), logic_error);
  BOOST_CHECK_THROW(ic.addMandatoryId(0U), logic_error);
  BOOST_CHECK_THROW(ic.addAvoidedId(0U), logic_error);
  BOOST_CHECK_THROW(ic.addMandatoryGroup(vector{0U, 1U}), logic_error);
  BOOST_CHECK_THROW(ic.addOptionalGroup(vector{0U, 2U}), logic_error);

  // Setting id 1 as mandatory
  BOOST_CHECK_NO_THROW(ic.addMandatoryId(1U));
  BOOST_CHECK(ic.longestMatchLength() == 2U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(ae));
  BOOST_CHECK_THROW(ic.validate(ae, 0U),
                    logic_error);  // the mandatory id 1 exceeds the capacity 0
  BOOST_REQUIRE_NO_THROW(me = {1U});  // id 1 only
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({0U, 1U}));  // ids 0 and 1
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = {});  // empty config
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = {0U});                   // id 0 only
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!ic.matches(me)));  // id 1 is missing
  BOOST_REQUIRE_NO_THROW(me = vector({0U, 1U, 2U}));   // ids 0, 1 and 2
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!ic.matches(me)));  // id 2 wasn't expected

  // Setting id 2 as avoided
  BOOST_CHECK_NO_THROW(ic.addAvoidedId(2U));
  BOOST_CHECK(ic.longestMatchLength() == 2U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(ae));
  BOOST_REQUIRE_NO_THROW(me = {1U});  // id 1 only
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({0U, 1U}));  // ids 0 and 1
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 2U}));       // ids 1 and 2
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!ic.matches(me)));  // id 2 is avoided
  BOOST_REQUIRE_NO_THROW(me = vector({0U, 1U, 2U}));   // ids 0, 1 and 2
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!ic.matches(me)));  // id 2 is avoided
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 3U}));       // ids 1 and 3
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!ic.matches(me)));  // id 3 wasn't expected

  // Allowing one of the group 3 and 4
  BOOST_CHECK_NO_THROW(ic.addOptionalGroup(vector{3U, 4U}));
  BOOST_CHECK(ic.longestMatchLength() == 3U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(ae));
  BOOST_REQUIRE_NO_THROW(me = {1U});  // id 1 only
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 3U}));  // ids 1 and 3
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 4U}));  // ids 1 and 4
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({0U, 1U}));  // ids 0 and 1
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({0U, 1U, 3U}));  // ids 0, 1 and 3
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({0U, 1U, 4U}));  // ids 0, 1 and 4
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 2U}));       // ids 1 and 2
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!ic.matches(me)));  // id 2 is avoided
  BOOST_REQUIRE_NO_THROW(me = vector({0U, 1U, 2U}));   // ids 0, 1 and 2
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!ic.matches(me)));  // id 2 is avoided
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 3U, 4U}));   // ids 1, 3 and 4
  BOOST_CHECK_NO_THROW(
      BOOST_CHECK(!ic.matches(me)));  // only one of 3 and 4 can appear
  BOOST_REQUIRE_NO_THROW(me = vector({0U, 1U, 3U, 4U}));  // ids 0, 1, 3 and 4
  BOOST_CHECK_NO_THROW(
      BOOST_CHECK(!ic.matches(me)));  // only one of 3 and 4 can appear

  // Expecting one of the group 5 and 6
  BOOST_CHECK_NO_THROW(ic.addMandatoryGroup(vector{5U, 6U}));
  BOOST_CHECK(ic.longestMatchLength() == 4U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(ae));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 5U}));  // id 1 and 5
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 6U}));  // id 1 and 6
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 5U, 6U}));  // id 1, 5 and 6
  BOOST_CHECK_NO_THROW(
      BOOST_CHECK(!ic.matches(me)));  // only one of 5 and 6 can appear
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 5U, 7U}));   // id 1, 5 and 7
  BOOST_CHECK_NO_THROW(BOOST_CHECK(!ic.matches(me)));  // unexpected id 7

  // Expecting 1 unspecified mandatory id from the rest of the id-s
  BOOST_CHECK_NO_THROW(ic.addUnspecifiedMandatory());
  BOOST_CHECK(ic.longestMatchLength() == 5U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(ae));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 5U, 7U}));  // id 1, 5 and 7
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 6U, 8U}));  // id 1, 6 and 8
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 6U, 7U, 8U}));  // id 1, 6, 7 and 8
  BOOST_CHECK_NO_THROW(BOOST_CHECK(
      !ic.matches(me)));  // 7 and 8 are 2 extra id-s,  but expecting only one

  // Removing the capacity limit condition
  BOOST_CHECK_NO_THROW(ic.setUnbounded());
  BOOST_CHECK(ic.longestMatchLength() == UINT_MAX);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(ae));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 5U, 7U}));  // id 1, 5 and 7
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 6U, 8U}));  // id 1, 6 and 8
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector({1U, 6U, 7U, 8U}));  // id 1, 6, 7 and 8
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));

  // Cannot use invalid id 50
  BOOST_CHECK_NO_THROW(ic.addMandatoryId(50U));
  BOOST_CHECK(ic.longestMatchLength() == UINT_MAX);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_THROW(ic.validate(ae), logic_error);
}

BOOST_AUTO_TEST_CASE(configConstraints_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::cond;
  using namespace rc::ent;

  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  shared_ptr<const AllEntities> spAe{pAe.release()};
  try {  // Available: 2 x t0 + 2 x t1 + 2 x t2 + t3 + t4 + t5
    ae += make_shared<const Entity>(0U, "e0", "t0", false, "true");
    ae += make_shared<const Entity>(1U, "e1", "t0", false, "false");
    ae += make_shared<const Entity>(2U, "e2", "t1", false, "true");
    ae += make_shared<const Entity>(3U, "e3", "t1", false, "false");
    ae += make_shared<const Entity>(4U, "e4", "t2", false, "true");
    ae += make_shared<const Entity>(5U, "e5", "t2", false, "false");
    ae += make_shared<const Entity>(6U, "e6", "t3", false, "true");
    ae += make_shared<const Entity>(7U, "e7", "t4", false, "true");
    ae += make_shared<const Entity>(8U, "e8", "t5", false, "true");
  } catch (...) {
    BOOST_REQUIRE(false);  // Unexpected exception
  }
  const size_t countAvailEnts{ae.count()};
  BankEntities be{spAe};

  try {  // allow nothing
    ConfigConstraints cc{{}, ae};
    BOOST_CHECK(cc.empty());
    BOOST_CHECK(cc.allowed());
    BOOST_CHECK(!cc.check(be = {}));
    BOOST_CHECK(!cc.check(be = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // allow anything
    ConfigConstraints cc{{}, ae, false};
    BOOST_CHECK(cc.empty());
    BOOST_CHECK(!cc.allowed());
    BOOST_CHECK(cc.check(be = {}));
    BOOST_CHECK(cc.check(be = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // required more entities than available
    shared_ptr<const IdsConstraint> ic{make_shared<const IdsConstraint>()};
    IdsConstraint* pIc{const_cast<IdsConstraint*>(ic.get())};

    // asking for all available entities
    for (size_t i{}; i < countAvailEnts; ++i)
      pIc->addUnspecifiedMandatory();
    BOOST_CHECK_NO_THROW(ConfigConstraints({ic}, ae));
    BOOST_CHECK_NO_THROW(ConfigConstraints({ic}, ae, false));

    // asking for an extra entity
    pIc->addUnspecifiedMandatory();
    BOOST_CHECK_THROW(ConfigConstraints({ic}, ae), logic_error);
    BOOST_CHECK_THROW(ConfigConstraints({ic}, ae, false), logic_error);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // invalid id 50
    shared_ptr<const IdsConstraint> ic{make_shared<const IdsConstraint>()};
    IdsConstraint* pIc{const_cast<IdsConstraint*>(ic.get())};
    pIc->addAvoidedId(50U);  // not(50)

    BOOST_CHECK_THROW(ConfigConstraints({ic}, ae), logic_error);
    BOOST_CHECK_THROW(ConfigConstraints({ic}, ae, false), logic_error);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // invalid type 't'
    shared_ptr<const TypesConstraint> tc{make_shared<const TypesConstraint>()};
    TypesConstraint* pTc{const_cast<TypesConstraint*>(tc.get())};
    pTc->addTypeRange("t", 2U, 2U);  // 't'{2}

    BOOST_CHECK_THROW(ConfigConstraints({tc}, ae), logic_error);
    BOOST_CHECK_THROW(ConfigConstraints({tc}, ae, false), logic_error);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // invalid id 50 and  type 't', but postponed validation
    shared_ptr<const IdsConstraint> ic{make_shared<const IdsConstraint>()};
    IdsConstraint* pIc{const_cast<IdsConstraint*>(ic.get())};
    pIc->addAvoidedId(50U);  // not(50)

    shared_ptr<const TypesConstraint> tc{make_shared<const TypesConstraint>()};
    TypesConstraint* pTc{const_cast<TypesConstraint*>(tc.get())};
    pTc->addTypeRange("t", 2U, 2U);  // 't'{2}

    ConfigConstraints({ic, tc}, ae, true, true);
    ConfigConstraints({ic, tc}, ae, false, true);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // 2 alternative constraints
    shared_ptr<const IdsConstraint> ic{make_shared<const IdsConstraint>()};
    IdsConstraint* pIc{const_cast<IdsConstraint*>(ic.get())};
    // not(1) (2|3)? (6|7|8) .{1}
    pIc->addAvoidedId(1U)
        .addOptionalGroup(vector{2U, 3U})
        .addMandatoryGroup(vector{6U, 7U, 8U})
        .addUnspecifiedMandatory();

    shared_ptr<const TypesConstraint> tc{make_shared<const TypesConstraint>()};
    TypesConstraint* pTc{const_cast<TypesConstraint*>(tc.get())};
    // 't0'{1,2} 't1'{,2} 't5'{1,}
    pTc->addTypeRange("t0", 1U, 2U)  // 0 | 1 | (0 1)
        .addTypeRange("t1", 0U, 2U)  // (not(2) not(3)) | 2 | 3 | (2 3)
        .addTypeRange("t5", 1U);     // 8

    ConfigConstraints cc{{ic, tc}, ae};
    BOOST_CHECK(!cc.empty());
    BOOST_CHECK(cc.allowed());

    ConfigConstraints notCc{{ic, tc}, ae, false};
    BOOST_CHECK(!notCc.empty());
    BOOST_CHECK(!notCc.allowed());

    // These are some of the configurations which satisfy 1st constraint
    BOOST_CHECK(cc.check(be = {0U, 6U}));
    BOOST_CHECK(!notCc.check(be));
    BOOST_CHECK(cc.check(be = {2U, 4U, 7U}));
    BOOST_CHECK(!notCc.check(be));
    BOOST_CHECK(cc.check(be = {3U, 5U, 8U}));
    BOOST_CHECK(!notCc.check(be));
    BOOST_CHECK(cc.check(be = {0U, 3U, 8U}));
    BOOST_CHECK(!notCc.check(be));

    // These are some of the configurations which satisfy 2nd constraint
    BOOST_CHECK(cc.check(be = {0U, 8U}));
    BOOST_CHECK(!notCc.check(be));
    BOOST_CHECK(cc.check(be = {1U, 2U, 8U}));
    BOOST_CHECK(!notCc.check(be));
    BOOST_CHECK(cc.check(be = {0U, 1U, 3U, 8U}));
    BOOST_CHECK(!notCc.check(be));
    BOOST_CHECK(cc.check(be = {0U, 1U, 2U, 3U, 8U}));
    BOOST_CHECK(!notCc.check(be));

    // Several configurations which should not satisfy neither of the
    // constraints
    BOOST_CHECK(!cc.check(  // both constraints require at least 2 id-s
        be = {}));
    BOOST_CHECK(notCc.check(be));
    BOOST_CHECK(!cc.check(  // both constraints require at least 2 id-s
        be = {0U}));
    BOOST_CHECK(notCc.check(be));
    BOOST_CHECK(!cc.check(
        // id 7 cannot appear together with 8; id 7 has an unsuitable type 't4'
        be = {0U, 7U, 8U}));
    BOOST_CHECK(notCc.check(be));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_CASE(transferConstraints_usecases) {
  using namespace std;
  using namespace rc;
  using namespace rc::cond;
  using namespace rc::ent;

  auto pAe{make_unique<AllEntities>()};
  AllEntities& ae{*pAe};
  shared_ptr<const AllEntities> spAe{pAe.release()};
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

  const size_t countAvailEnts{ae.count()};
  constexpr unsigned cap{UINT_MAX};
  MovingEntities me{spAe};

  try {  // allow nothing
    TransferConstraints cc{{}, ae, cap};
    BOOST_CHECK(cc.empty());
    BOOST_CHECK(cc.allowed());
    BOOST_CHECK(!cc.minRequiredCapacity());
    BOOST_CHECK(!cc.check(me = {}));
    BOOST_CHECK(!cc.check(me = {0U, 8U}));
    BOOST_CHECK_THROW(ignore = cc.check(BankEntities(spAe)),
                      bad_cast);  // needs a MovingEntities parameter
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // allow anything in groups not larger than 2 entities
    constexpr unsigned cap2{2U};
    TransferConstraints notCc{{}, ae, cap2, false};
    BOOST_CHECK(notCc.empty());
    BOOST_CHECK(!notCc.allowed());
    BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    BOOST_CHECK(notCc.check(me = {}));
    BOOST_CHECK(notCc.check(me = {0U, 8U}));
    BOOST_CHECK(!notCc.check(me = {0U, 2U, 8U}));
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // allow anything in groups not heavier than 12
    constexpr double maxLoad{12.};
    const MaxLoadTransferConstraintsExt maxLoadExt{maxLoad};
    TransferConstraints notCc{{}, ae, cap, false, maxLoadExt};
    MovingEntities meMaxLoad12{
        spAe, {}, make_unique<TotalLoadExt>(spAe, maxLoad)};

    BOOST_CHECK(notCc.empty());
    BOOST_CHECK(!notCc.allowed());
    BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    BOOST_CHECK(notCc.check(meMaxLoad12 = {}));             // 0 < 12
    BOOST_CHECK(notCc.check(meMaxLoad12 = {2U, 8U}));       // 3+9=12 <= 12
    BOOST_CHECK(!notCc.check(meMaxLoad12 = {0U, 2U, 8U}));  // 1+3+9=13 > 12
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // required more entities than available
    shared_ptr<const IdsConstraint> ic{make_shared<const IdsConstraint>()};
    IdsConstraint* pIc{const_cast<IdsConstraint*>(ic.get())};

    // empty ids constraint
    BOOST_CHECK_NO_THROW({
      TransferConstraints cc({ic}, ae, cap);
      BOOST_CHECK(!cc.minRequiredCapacity());
    });
    BOOST_CHECK_NO_THROW({
      TransferConstraints notCc({ic}, ae, cap, false);
      BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    });

    // asking for all available entities but one
    for (size_t i{1ULL}; i < countAvailEnts; ++i)  // all except one for now
      pIc->addUnspecifiedMandatory();
    BOOST_CHECK_NO_THROW({
      TransferConstraints cc({ic}, ae, cap);
      BOOST_CHECK(cc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    });
    BOOST_CHECK_NO_THROW({
      TransferConstraints notCc({ic}, ae, cap, false);
      BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    });

    // adding the last one here
    pIc->addUnspecifiedMandatory();
    BOOST_CHECK_NO_THROW({
      TransferConstraints cc({ic}, ae, cap);
      BOOST_CHECK(cc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    });
    BOOST_CHECK_NO_THROW({
      TransferConstraints notCc({ic}, ae, cap, false);
      BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    });

    // asking for an extra entity
    pIc->addUnspecifiedMandatory();
    BOOST_CHECK_THROW(TransferConstraints({ic}, ae, cap), logic_error);
    BOOST_CHECK_THROW(TransferConstraints({ic}, ae, cap, false), logic_error);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // required more entities than the capacity=2
    constexpr unsigned cap2{2U};

    shared_ptr<const IdsConstraint> ic{make_shared<const IdsConstraint>()};
    IdsConstraint* pIc{const_cast<IdsConstraint*>(ic.get())};
    for (unsigned i{}; i < cap2; ++i)  // filling the capacity
      pIc->addUnspecifiedMandatory();
    BOOST_CHECK_NO_THROW(TransferConstraints({ic}, ae, cap2));
    BOOST_CHECK_NO_THROW(TransferConstraints({ic}, ae, cap2, false));

    // asking for an extra entity
    pIc->addUnspecifiedMandatory();
    BOOST_CHECK_THROW(TransferConstraints({ic}, ae, cap2), logic_error);
    BOOST_CHECK_THROW(TransferConstraints({ic}, ae, cap2, false), logic_error);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // invalid id 50
    shared_ptr<const IdsConstraint> ic{make_shared<const IdsConstraint>()};
    IdsConstraint* pIc{const_cast<IdsConstraint*>(ic.get())};
    pIc->addAvoidedId(50U);  // not(50)

    BOOST_CHECK_THROW(TransferConstraints({ic}, ae, cap), logic_error);
    BOOST_CHECK_THROW(TransferConstraints({ic}, ae, cap, false), logic_error);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // invalid type 't'
    shared_ptr<const TypesConstraint> tc{make_shared<const TypesConstraint>()};
    TypesConstraint* pTc{const_cast<TypesConstraint*>(tc.get())};
    pTc->addTypeRange("t", 2U, 2U);  // 't'{2}

    BOOST_CHECK_THROW(TransferConstraints({tc}, ae, cap), logic_error);
    BOOST_CHECK_THROW(TransferConstraints({tc}, ae, cap, false), logic_error);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // more required entities ('t0'{2} + 't1'{1,2}) than the capacity=2
  try {
    constexpr unsigned cap2{2U};

    shared_ptr<const TypesConstraint> tc{make_shared<const TypesConstraint>()};
    TypesConstraint* pTc{const_cast<TypesConstraint*>(tc.get())};
    pTc->addTypeRange("t0", 2U, 2U)   // 't0'{2}
        .addTypeRange("t1", 1U, 2U);  // 't1'{1, 2}

    BOOST_CHECK_THROW(TransferConstraints({tc}, ae, cap2), logic_error);
    BOOST_CHECK_THROW(TransferConstraints({tc}, ae, cap2, false), logic_error);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // heavier entities ('t4' + 't5' => 17) than the max_load=5
  try {
    constexpr double maxLoad{5.};
    const MaxLoadTransferConstraintsExt maxLoadExt{maxLoad};

    shared_ptr<const TypesConstraint> tc{make_shared<const TypesConstraint>()};
    TypesConstraint* pTc{const_cast<TypesConstraint*>(tc.get())};
    pTc->addTypeRange("t4", 1U, 1U)   // 't4'{1}
        .addTypeRange("t5", 1U, 1U);  // 't5'{1}

    BOOST_CHECK_THROW(TransferConstraints({tc}, ae, cap, true, maxLoadExt),
                      logic_error);
    BOOST_CHECK_THROW(TransferConstraints({tc}, ae, cap, false, maxLoadExt),
                      logic_error);
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {  // 2 alternative constraints
    constexpr unsigned cap2{2U};
    constexpr double maxLoad{12.};
    const MaxLoadTransferConstraintsExt maxLoadExt{maxLoad};

    shared_ptr<const IdsConstraint> ic{make_shared<const IdsConstraint>()};
    IdsConstraint* pIc{const_cast<IdsConstraint*>(ic.get())};
    // not(1) (2|3)? (6|7|8) .{1}
    pIc->addAvoidedId(1U)
        .addOptionalGroup(vector{2U, 3U})
        .addMandatoryGroup(vector{6U, 7U, 8U})
        .addUnspecifiedMandatory();

    shared_ptr<const TypesConstraint> tc{make_shared<const TypesConstraint>()};
    TypesConstraint* pTc{const_cast<TypesConstraint*>(tc.get())};
    // 't0'{1,2} 't1'{,2} 't5'{1,}
    pTc->addTypeRange("t0", 1U, 2U)  // 0 | 1 | (0 1)
        .addTypeRange("t1", 0U, 2U)  // (not(2) not(3)) | 2 | 3 | (2 3)
        .addTypeRange("t5", 1U);     // 8

    TransferConstraints cc{{ic, tc}, ae, cap2, true, maxLoadExt};
    BOOST_CHECK(!cc.empty());
    BOOST_CHECK(cc.allowed());
    BOOST_CHECK(cc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);

    TransferConstraints notCc{{ic, tc}, ae, cap2, false, maxLoadExt};
    BOOST_CHECK(!notCc.empty());
    BOOST_CHECK(!notCc.allowed());
    BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);

    MovingEntities meMaxLoad12{
        spAe, {}, make_unique<TotalLoadExt>(spAe, maxLoad)};

    // These are some of the configurations which satisfy 1st constraint
    BOOST_CHECK(cc.check(meMaxLoad12 = {0U, 6U}));  // weight: 1+7 < 12
    BOOST_CHECK(!notCc.check(meMaxLoad12));
    BOOST_CHECK(cc.check(meMaxLoad12 = {0U, 8U}));  // weight: 1+9 < 12
    BOOST_CHECK(!notCc.check(meMaxLoad12));
    BOOST_CHECK(cc.check(meMaxLoad12 = {4U, 6U}));  // weight: 5+7 <= 12
    BOOST_CHECK(!notCc.check(meMaxLoad12));

    // These are some of the configurations which satisfy 2nd constraint
    BOOST_CHECK(cc.check(meMaxLoad12 = {1U, 8U}));  // weight: 2+9 < 12
    BOOST_CHECK(!notCc.check(meMaxLoad12));

    // Several configurations which should not satisfy neither of the
    // constraints
    BOOST_CHECK(!cc.check(  // both constraints require at least 2 id-s
        meMaxLoad12 = {}));
    BOOST_CHECK(notCc.check(meMaxLoad12));
    BOOST_CHECK(!cc.check(  // both constraints require at least 2 id-s
        meMaxLoad12 = {0U}));
    BOOST_CHECK(notCc.check(meMaxLoad12));
    BOOST_CHECK(!cc.check(  // missing unspecified mandatory id
        meMaxLoad12 = {2U, 7U}));
    BOOST_CHECK(notCc.check(meMaxLoad12));
    BOOST_CHECK(!cc.check(  // capacity overflow: 3 > 2
        meMaxLoad12 = {0U, 2U, 6U}));
    BOOST_CHECK(!notCc.check(
        meMaxLoad12));  // negated constraints still need to respect capacity
    BOOST_CHECK(!cc.check(  // beyond max load: 6+8 = 14 > 12
        meMaxLoad12 = {5U, 7U}));
    BOOST_CHECK(!notCc.check(
        meMaxLoad12));  // negated constraints still need to respect maxLoad
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  try {
    shared_ptr<const IdsConstraint> ic1{make_shared<const IdsConstraint>()};
    IdsConstraint* pIc1{const_cast<IdsConstraint*>(ic1.get())};
    // not(1) (2|3)? (6|7|8) .{1}
    pIc1->addAvoidedId(1U)
        .addOptionalGroup(vector{2U, 3U})
        .addMandatoryGroup(vector{6U, 7U, 8U})
        .addUnspecifiedMandatory();

    shared_ptr<const IdsConstraint> ic2{make_shared<const IdsConstraint>()};
    IdsConstraint* pIc2{const_cast<IdsConstraint*>(ic2.get())};
    // .{4,} not(8)
    pIc2->addUnspecifiedMandatory()
        .addUnspecifiedMandatory()
        .addUnspecifiedMandatory()
        .addUnspecifiedMandatory()
        .addAvoidedId(8U)
        .setUnbounded();

    shared_ptr<const TypesConstraint> tc{make_shared<const TypesConstraint>()};
    TypesConstraint* pTc{const_cast<TypesConstraint*>(tc.get())};
    // 't0'{1,2} 't1'{,2} 't5'{1}
    pTc->addTypeRange("t0", 1U, 2U)   // 0 | 1 | (0 1)
        .addTypeRange("t1", 0U, 2U)   // (not(2) not(3)) | 2 | 3 | (2 3)
        .addTypeRange("t5", 1U, 1U);  // 8

    {
      /*
       Using only the first Ids constraint, which can be accommodated
       by a raft/bridge with a capacity of 3:
       - 1 for the optionals `(2|3)?`
       - 1 for the mandatories `(6|7|8)`
       - 1 for the unspecified mandatory `.{1}`
      */
      TransferConstraints cc{{ic1}, ae, cap};
      BOOST_CHECK(!cc.empty());
      BOOST_CHECK(cc.allowed());
      BOOST_CHECK(cc.minRequiredCapacity() == 3U);

      TransferConstraints notCc{{ic1}, ae, cap, false};
      BOOST_CHECK(!notCc.empty());
      BOOST_CHECK(!notCc.allowed());
      BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    }

    {
      /*
       Using only the first Types constraint, which can be accommodated
       by a raft/bridge with a capacity of 5:
       - 2 for the mandatories `'t0'{1,2}`
       - 2 for the optionals `'t1'{,2}`
       - 1 for the mandatory `'t5'{1}`
      */
      TransferConstraints cc{{tc}, ae, cap};
      BOOST_CHECK(!cc.empty());
      BOOST_CHECK(cc.allowed());
      BOOST_CHECK(cc.minRequiredCapacity() == 5U);

      TransferConstraints notCc{{tc}, ae, cap, false};
      BOOST_CHECK(!notCc.empty());
      BOOST_CHECK(!notCc.allowed());
      BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    }

    {
      /*
       Using the first Ids constraint and the Types constraint,
       which can be accommodated by a raft/bridge with a capacity of 5,
       value dictated by the first Types constraint, which is spacier.
      */
      TransferConstraints cc{{ic1, tc}, ae, cap};
      BOOST_CHECK(!cc.empty());
      BOOST_CHECK(cc.allowed());
      BOOST_CHECK(cc.minRequiredCapacity() == 5U);

      TransferConstraints notCc{{ic1, tc}, ae, cap, false};
      BOOST_CHECK(!notCc.empty());
      BOOST_CHECK(!notCc.allowed());
      BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    }

    {
      /*
       Using only the second Ids constraint, which can be accommodated
       by a raft/bridge with a capacity of 3 when negated:
       - anything that is less than 4 entities and doesn't contain entity 8
      */
      TransferConstraints cc{{ic2}, ae, cap};
      BOOST_CHECK(!cc.empty());
      BOOST_CHECK(cc.allowed());
      BOOST_CHECK(cc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);

      TransferConstraints notCc{{ic2}, ae, cap, false};
      BOOST_CHECK(!notCc.empty());
      BOOST_CHECK(!notCc.allowed());
      BOOST_CHECK(notCc.minRequiredCapacity() == 3U);
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }
}

BOOST_AUTO_TEST_SUITE_END()

#endif  // for CPP_CONFIG_CONSTRAINT and UNIT_TESTING
