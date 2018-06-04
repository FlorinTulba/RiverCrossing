/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#if ! defined CONFIG_CONSTRAINT_CPP || ! defined UNIT_TESTING

  #error \
"Please include this file only at the end of `configConstraint.cpp` \
after a `#define CONFIG_CONSTRAINT_CPP` and surrounding the include and the define \
by `#ifdef UNIT_TESTING`!"

#else // for CONFIG_CONSTRAINT_CPP and UNIT_TESTING

#include <boost/test/unit_test.hpp>

#include "../src/entity.h"

using namespace rc;
using namespace rc::cond;
using namespace rc::ent;

BOOST_AUTO_TEST_SUITE(configConstraint, *boost::unit_test::tolerance(Eps))

BOOST_AUTO_TEST_CASE(numericConst_usecases) {
  bool b;
  constexpr double d = 4.5;

  const NumericConst c(d);
  BOOST_CHECK( ! c.dependsOnVariable(""));
  BOOST_CHECK(b = (bool) c.constValue());
  if(b) BOOST_TEST(*c.constValue() == d);
  BOOST_CHECK_NO_THROW(BOOST_TEST(c.eval(SymbolsTable{}) == d));
}

BOOST_AUTO_TEST_CASE(numericVariable_usecases) {
  constexpr double d = 4.5;
  const string var = "a";
  const SymbolsTable st{{var, d}};

  const NumericVariable v(var);
  BOOST_CHECK( ! v.dependsOnVariable(""));
  BOOST_CHECK(v.dependsOnVariable(var));
  BOOST_CHECK( ! v.constValue());
  BOOST_CHECK_THROW(v.eval(SymbolsTable{}), out_of_range);
  BOOST_CHECK_NO_THROW(BOOST_TEST(v.eval(st) == d));
}

BOOST_AUTO_TEST_CASE(addition_usecases) {
  bool b;
  constexpr double d1 = 4.5, d2 = 2.5, d = d1 + d2;
  const string var1 = "a", var2 = "b";
  const SymbolsTable emptySt,
    st{{var1, d1}, {var2, d2}};

  const shared_ptr<const NumericExpr>
    c1 = make_shared<const NumericConst>(d1),
    c2 = make_shared<const NumericConst>(d2),
    v1 = make_shared<const NumericVariable>(var1),
    v2 = make_shared<const NumericVariable>(var2);

  // nullptr arguments
  BOOST_CHECK_THROW(Addition(nullptr, nullptr), invalid_argument);
  BOOST_CHECK_THROW(Addition(c1, nullptr), invalid_argument);
  BOOST_CHECK_THROW(Addition(nullptr, c1), invalid_argument);

  // adding 2 constants
  BOOST_CHECK_NO_THROW({
    const Addition a(c1, c2);
    BOOST_CHECK( ! a.dependsOnVariable(var1));
    BOOST_CHECK( ! a.dependsOnVariable(var2));
    BOOST_CHECK(b = (bool) a.constValue());
    if(b) BOOST_TEST(*a.constValue() == d);
    BOOST_CHECK_NO_THROW(BOOST_TEST(a.eval(emptySt) == d));
  });

   // adding a constant to a variable
  BOOST_CHECK_NO_THROW({
    const Addition a(c1, v2);
    BOOST_CHECK( ! a.dependsOnVariable(var1));
    BOOST_CHECK(a.dependsOnVariable(var2));
    BOOST_CHECK( ! a.constValue());
    BOOST_CHECK_THROW(a.eval(emptySt), out_of_range);
    BOOST_CHECK_NO_THROW(BOOST_TEST(a.eval(st) == d));
  });
  BOOST_CHECK_NO_THROW({
    const Addition a(v1, c2);
    BOOST_CHECK(a.dependsOnVariable(var1));
    BOOST_CHECK( ! a.dependsOnVariable(var2));
    BOOST_CHECK( ! a.constValue());
    BOOST_CHECK_THROW(a.eval(emptySt), out_of_range);
    BOOST_CHECK_NO_THROW(BOOST_TEST(a.eval(st) == d));
  });

  // adding 2 variables
  BOOST_CHECK_NO_THROW({
    const Addition a(v1, v2);
    BOOST_CHECK(a.dependsOnVariable(var1));
    BOOST_CHECK(a.dependsOnVariable(var2));
    BOOST_CHECK( ! a.constValue());
    BOOST_CHECK_THROW(a.eval(emptySt), out_of_range);
    BOOST_CHECK_THROW(a.eval(SymbolsTable{{var1, d1}}), out_of_range);
    BOOST_CHECK_THROW(a.eval(SymbolsTable{{var2, d2}}), out_of_range);
    BOOST_CHECK_NO_THROW(BOOST_TEST(a.eval(st) == d));
  });
}

BOOST_AUTO_TEST_CASE(modulus_usecases) {
  bool b;
  constexpr double d1 = 5., d2 = 3., d = (double)((long)d1 % (long)d2),
    badVal = 2.3;
  const string var1 = "a", var2 = "b";
  const SymbolsTable emptySt,
    partialSt1{{var1, d1}},
    partialSt2{{var2, d2}},
    st{{var1, d1}, {var2, d2}};

  const shared_ptr<const NumericExpr>
    c1 = make_shared<const NumericConst>(d1),
    c2 = make_shared<const NumericConst>(d2),
    badCt = make_shared<const NumericConst>(badVal),
    zero = make_shared<const NumericConst>(0.),
    one = make_shared<const NumericConst>(1.),
    minusOne = make_shared<const NumericConst>(-1.),
    v1 = make_shared<const NumericVariable>(var1),
    v2 = make_shared<const NumericVariable>(var2);

  // nullptr arguments
  BOOST_CHECK_THROW(Modulus(nullptr, nullptr), invalid_argument);
  BOOST_CHECK_THROW(Modulus(c1, nullptr), invalid_argument);
  BOOST_CHECK_THROW(Modulus(nullptr, c1), invalid_argument);

  // non-integer arguments
  BOOST_CHECK_THROW(Modulus(c1, badCt), logic_error);
  BOOST_CHECK_THROW(Modulus(badCt, v2), logic_error);
  BOOST_CHECK_THROW(Modulus(badCt, badCt), logic_error);
  BOOST_CHECK_NO_THROW({
    const Modulus m(v1, v2);
    BOOST_CHECK(m.dependsOnVariable(var1));
    BOOST_CHECK(m.dependsOnVariable(var2));
    BOOST_CHECK_THROW(m.eval(SymbolsTable{{var1, badVal}, {var2, d2}}),
      logic_error);
    BOOST_CHECK_THROW(m.eval(SymbolsTable{{var1, d1}, {var2, badVal}}),
      logic_error);
    BOOST_CHECK_THROW(m.eval(SymbolsTable{{var1, badVal}, {var2, badVal}}),
      logic_error);
  });

  // denominator or both arguments are 0
  BOOST_CHECK_THROW(Modulus m(c1, zero), overflow_error);
  BOOST_CHECK_THROW(Modulus m(zero, zero), logic_error);
  BOOST_CHECK_NO_THROW({
    const Modulus m(v1, v2);
    BOOST_CHECK(m.dependsOnVariable(var1));
    BOOST_CHECK(m.dependsOnVariable(var2));
    BOOST_CHECK_THROW(m.eval(SymbolsTable{{var1, d1}, {var2, 0.}}),
      overflow_error);
    BOOST_CHECK_THROW(m.eval(SymbolsTable{{var1, 0.}, {var2, 0.}}),
      logic_error);
  });

  // denominator 1 or -1 means result 0
  BOOST_CHECK_NO_THROW({ // const numerator
    const Modulus m(c1, one);
    BOOST_CHECK( ! m.dependsOnVariable(var1));
    BOOST_CHECK( ! m.dependsOnVariable(var2));
    BOOST_CHECK(b = (bool) m.constValue());
    if(b) BOOST_TEST(*m.constValue() == 0.);
    BOOST_CHECK_NO_THROW(BOOST_TEST(m.eval(emptySt) == 0.));
  });
  BOOST_CHECK_NO_THROW({ // the numerator is a variable
    const Modulus m(v1, minusOne);
    BOOST_CHECK( ! m.dependsOnVariable(var1));
    BOOST_CHECK( ! m.dependsOnVariable(var2));
    BOOST_CHECK(b = (bool) m.constValue());
    if(b) BOOST_TEST(*m.constValue() == 0.);
    BOOST_CHECK_NO_THROW(BOOST_TEST(m.eval(emptySt) == 0.));
  });

  // modulus between 2 constants
  BOOST_CHECK_NO_THROW({
    const Modulus m(c1, c2);
    BOOST_CHECK( ! m.dependsOnVariable(var1));
    BOOST_CHECK( ! m.dependsOnVariable(var2));
    BOOST_CHECK(b = (bool) m.constValue());
    if(b) BOOST_TEST(*m.constValue() == d);
    BOOST_CHECK_NO_THROW(BOOST_TEST(m.eval(emptySt) == d));
  });

   // modulus between a constant and a variable
  BOOST_CHECK_NO_THROW({
    const Modulus m(c1, v2);
    BOOST_CHECK( ! m.dependsOnVariable(var1));
    BOOST_CHECK(m.dependsOnVariable(var2));
    BOOST_CHECK( ! m.constValue());
    BOOST_CHECK_THROW(m.eval(emptySt), out_of_range);
    BOOST_CHECK_NO_THROW(BOOST_TEST(m.eval(st) == d));
  });
  BOOST_CHECK_NO_THROW({
    const Modulus m(v1, c2);
    BOOST_CHECK(m.dependsOnVariable(var1));
    BOOST_CHECK( ! m.dependsOnVariable(var2));
    BOOST_CHECK( ! m.constValue());
    BOOST_CHECK_THROW(m.eval(emptySt), out_of_range);
    BOOST_CHECK_NO_THROW(BOOST_TEST(m.eval(st) == d));
  });

  // modulus between 2 variables
  BOOST_CHECK_NO_THROW({
    const Modulus m(v1, v2);
    BOOST_CHECK(m.dependsOnVariable(var1));
    BOOST_CHECK(m.dependsOnVariable(var2));
    BOOST_CHECK( ! m.constValue());
    BOOST_CHECK_THROW(m.eval(emptySt), out_of_range);
    BOOST_CHECK_THROW(m.eval(partialSt1), out_of_range);
    BOOST_CHECK_THROW(m.eval(partialSt2), out_of_range);
    BOOST_CHECK_NO_THROW(BOOST_TEST(m.eval(st) == d));
  });
}

BOOST_AUTO_TEST_CASE(valueOrRange_usecases) {
  bool b;
  constexpr double d = 4.5,
    dMin1 = d - 1.,
    nanD = numeric_limits<double>::quiet_NaN();
  const string var = "a";
  const SymbolsTable emptySt,
    st{{var, d}};
  const shared_ptr<const NumericExpr>
    c = make_shared<const NumericConst>(d),
    cMin1 = make_shared<const NumericConst>(dMin1),
    cNaN = make_shared<const NumericConst>(nanD),
    v = make_shared<const NumericVariable>(var);

  BOOST_CHECK_THROW(ValueOrRange(nullptr), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(c, nullptr), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(v, nullptr), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(nullptr, c), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(nullptr, v), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(nullptr, nullptr), logic_error);
  BOOST_CHECK_THROW(ValueOrRange vor(cNaN), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(c, cNaN), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(v, cNaN), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(cNaN, c), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(cNaN, v), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(cNaN, cNaN), logic_error);
  BOOST_CHECK_THROW(ValueOrRange(c, cMin1), logic_error); // reverse order

  BOOST_CHECK_NO_THROW({
    const ValueOrRange vor(v); // variable var='a' from st pointing to value d
    BOOST_CHECK(vor.dependsOnVariable(var));
    BOOST_CHECK( ! vor.isConst());
    BOOST_CHECK(b = ! vor.isRange());
    BOOST_CHECK_THROW(vor.range(st), logic_error);
    if(b) {
      BOOST_CHECK_NO_THROW(BOOST_TEST(d == vor.value(st)));
      BOOST_CHECK_THROW(vor.value(emptySt), out_of_range); // 'a' not found
      BOOST_CHECK_THROW(vor.value(SymbolsTable{{var, nanD}}), logic_error);
    }
  });
  BOOST_CHECK_NO_THROW({
    const ValueOrRange vor(c); // constant c
    BOOST_CHECK( ! vor.dependsOnVariable(var));
    BOOST_CHECK(vor.isConst());
    BOOST_CHECK(b = ! vor.isRange());
    BOOST_CHECK_THROW(vor.range(st), logic_error);
    if(b) BOOST_CHECK_NO_THROW(BOOST_TEST(d == vor.value(emptySt)));
  });
  BOOST_CHECK_NO_THROW({
    const ValueOrRange vor(c, c); // range degenerated to a single constant d
    BOOST_CHECK( ! vor.dependsOnVariable(var));
    BOOST_CHECK(vor.isConst());
    BOOST_CHECK(b = vor.isRange());
    BOOST_CHECK_THROW(vor.value(st), logic_error);
    if(b) BOOST_CHECK_NO_THROW({
      const auto limits = vor.range(emptySt);
      BOOST_TEST(d == limits.first);
      BOOST_TEST(d == limits.second);
    });
  });
  BOOST_CHECK_NO_THROW({ // range between constants d-1 .. d
    const ValueOrRange vor(cMin1, c);
    BOOST_CHECK( ! vor.dependsOnVariable(var));
    BOOST_CHECK(vor.isConst());
    BOOST_CHECK(b = vor.isRange());
    BOOST_CHECK_THROW(vor.value(st), logic_error);
    if(b) BOOST_CHECK_NO_THROW({
      const auto limits = vor.range(emptySt);
      BOOST_TEST(dMin1 == limits.first);
      BOOST_TEST(d == limits.second);
    });
  });
  BOOST_CHECK_NO_THROW({
    const ValueOrRange vor(v, v); // range degenerated to value pointed by variable var='a'
    BOOST_CHECK(vor.dependsOnVariable(var));
    BOOST_CHECK( ! vor.isConst());
    BOOST_CHECK(b = vor.isRange());
    BOOST_CHECK_THROW(vor.value(st), logic_error);
    if(b) {
      BOOST_CHECK_THROW(vor.range(emptySt), out_of_range); // 'a' not found
      BOOST_CHECK_NO_THROW({
        const auto limits = vor.range(st);
        BOOST_TEST(d == limits.first);
        BOOST_TEST(d == limits.second);
      });
    }
  });
  BOOST_CHECK_NO_THROW({
    const ValueOrRange vor(cMin1, v); // range between constant d-1 and variable var='a'
    BOOST_CHECK(vor.dependsOnVariable(var));
    BOOST_CHECK( ! vor.isConst());
    BOOST_CHECK(b = vor.isRange());
    BOOST_CHECK_THROW(vor.value(st), logic_error);
    if(b) {
      BOOST_CHECK_THROW(vor.range(emptySt), out_of_range); // 'a' not found
      BOOST_CHECK_THROW(vor.range(SymbolsTable{{var, nanD}}), logic_error);
      BOOST_CHECK_NO_THROW({
        const auto limits = vor.range(st);
        BOOST_TEST(dMin1 == limits.first);
        BOOST_TEST(d == limits.second);
      });
    }
  });
  BOOST_CHECK_NO_THROW({ // range between variable var='a' and constant d-1
    const ValueOrRange vor(v, cMin1);
    BOOST_CHECK(vor.dependsOnVariable(var));
    BOOST_CHECK( ! vor.isConst());
    BOOST_CHECK(b = vor.isRange());
    BOOST_CHECK_THROW(vor.value(st), logic_error);
    if(b) {
      BOOST_CHECK_THROW(vor.range(emptySt), out_of_range); // 'a' not found
      BOOST_CHECK_THROW(vor.range(SymbolsTable{{var, nanD}}), logic_error);
      BOOST_CHECK_THROW(vor.range(st), logic_error); // reverse order
    }
  });
}

BOOST_AUTO_TEST_CASE(valueSet_usecases) {
  constexpr double d = 2.4, d1 = 5.1, d2 = 6.8, missingD = 8.1;
  const string var = "a";
  const SymbolsTable emptySt,
    st{{var, d1}}; // a := d1
  const shared_ptr<const NumericExpr>
    c = make_shared<const NumericConst>(d),
    c1 = make_shared<const NumericConst>(d1),
    c2 = make_shared<const NumericConst>(d2),
    v = make_shared<const NumericVariable>(var);
  ValueSet vs;
  BOOST_CHECK(vs.empty());
  BOOST_CHECK(vs.constSet());
  BOOST_CHECK( ! vs.dependsOnVariable(var));

  vs.add(ValueOrRange(c)); // hold value d
  BOOST_CHECK( ! vs.empty());
  BOOST_CHECK(vs.constSet());
  BOOST_CHECK( ! vs.dependsOnVariable(var));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(vs.contains(d, emptySt)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! vs.contains(d1, emptySt)));

  vs.add(ValueOrRange(v, c2)); // hold range 'a' .. d2
  BOOST_CHECK( ! vs.constSet());
  BOOST_CHECK(vs.dependsOnVariable(var));
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! vs.contains(missingD, st)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(vs.contains(d, st)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(vs.contains(d1, st)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(vs.contains(d2, st)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(vs.contains((d1+d2)/2., st)));
  BOOST_CHECK_THROW(vs.contains(missingD, emptySt), out_of_range);
  BOOST_CHECK_THROW(vs.contains(d1, emptySt), out_of_range);
  BOOST_CHECK_THROW(vs.contains(d2, emptySt), out_of_range);
  BOOST_CHECK_THROW(vs.contains((d1+d2)/2., emptySt), out_of_range);
}

BOOST_AUTO_TEST_CASE(boolConst_usecases) {
  bool b;
  const SymbolsTable emptySt;

  {const BoolConst bc(true);
    BOOST_CHECK( ! bc.dependsOnVariable(""));
    BOOST_CHECK_NO_THROW(BOOST_CHECK(bc.eval(emptySt)));
    BOOST_CHECK(b = (bool) bc.constValue());
    if(b) BOOST_CHECK(*bc.constValue());}
  {const BoolConst bc(false);
    BOOST_CHECK( ! bc.dependsOnVariable(""));
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! bc.eval(emptySt)));
    BOOST_CHECK(b = (bool) bc.constValue());
    if(b) BOOST_CHECK( ! *bc.constValue());}
}

BOOST_AUTO_TEST_CASE(belongToCondition_usecases) {
  constexpr double d = 6.3, d1 = 2.3, d2 = 4.5, d3 = 9.8, dOut = 3.1;
  const string var = "a", var1 = "b", var2 = "c", var3 = "d", varOut = "e";
  const SymbolsTable emptySt,
    st{{var, d}, {var1, d1}, {var2, d2}, {var3, d3}, {varOut, dOut}};

  const shared_ptr<const NumericExpr>
    c = make_shared<const NumericConst>(d),
    cOut = make_shared<const NumericConst>(dOut),
    c1 = make_shared<const NumericConst>(d1),
    c2 = make_shared<const NumericConst>(d2),
    c3 = make_shared<const NumericConst>(d3),
    v = make_shared<const NumericVariable>(var),
    vOut = make_shared<const NumericVariable>(varOut),
    v1 = make_shared<const NumericVariable>(var1),
    v2 = make_shared<const NumericVariable>(var2),
    v3 = make_shared<const NumericVariable>(var3);

  ValueSet *pSetOfConstants = new ValueSet,
    *pSetWithVariables = new ValueSet;
  pSetOfConstants-> // {d1, d2..d3}
    add(ValueOrRange(c1)).
    add(ValueOrRange(c2, c3));
  pSetWithVariables-> // {'b', 'c'..d3} = {d1, d2..d3}
    add(ValueOrRange(v1)).
    add(ValueOrRange(v2, c3));
  const shared_ptr<const IValues<double>>
    emptySet = make_shared<const ValueSet>(),
    setOfConstants = shared_ptr<const ValueSet>(pSetOfConstants),
    setWithVariables = shared_ptr<const ValueSet>(pSetWithVariables);


  // NULL arguments
  BOOST_CHECK_THROW(BelongToCondition<bool>(nullptr, nullptr),
    invalid_argument);
  BOOST_CHECK_THROW(BelongToCondition<double>(nullptr, emptySet),
    invalid_argument);
  BOOST_CHECK_THROW(BelongToCondition<double>(c1, nullptr),
    invalid_argument);

  // membership test using only constants
  BOOST_CHECK_NO_THROW({ // d in {}
    const BelongToCondition<double> btc(c, emptySet);
    BOOST_CHECK( ! btc.dependsOnVariable(var));
    BOOST_CHECK( ! btc.dependsOnVariable(varOut));
    BOOST_CHECK( ! btc.dependsOnVariable(var1));
    BOOST_CHECK( ! btc.dependsOnVariable(var2));
    BOOST_CHECK( ! btc.dependsOnVariable(var3));
    BOOST_CHECK(btc.constValue());
    BOOST_CHECK( ! *btc.constValue());
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! btc.eval(emptySt)));
  });
  BOOST_CHECK_NO_THROW({ // d in {d1, d2..d3}
    const BelongToCondition<double> btc(c, setOfConstants);
    BOOST_CHECK( ! btc.dependsOnVariable(var));
    BOOST_CHECK( ! btc.dependsOnVariable(varOut));
    BOOST_CHECK( ! btc.dependsOnVariable(var1));
    BOOST_CHECK( ! btc.dependsOnVariable(var2));
    BOOST_CHECK( ! btc.dependsOnVariable(var3));
    BOOST_CHECK(btc.constValue());
    BOOST_CHECK(*btc.constValue());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(btc.eval(emptySt)));
  });
  BOOST_CHECK_NO_THROW({ // dOut in {d1, d2..d3}
    const BelongToCondition<double> btc(cOut, setOfConstants);
    BOOST_CHECK( ! btc.dependsOnVariable(var));
    BOOST_CHECK( ! btc.dependsOnVariable(varOut));
    BOOST_CHECK( ! btc.dependsOnVariable(var1));
    BOOST_CHECK( ! btc.dependsOnVariable(var2));
    BOOST_CHECK( ! btc.dependsOnVariable(var3));
    BOOST_CHECK(btc.constValue());
    BOOST_CHECK( ! *btc.constValue());
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! btc.eval(emptySt)));
  });

  // membership test using variables
  BOOST_CHECK_NO_THROW({ // 'a'(d) in {}
    const BelongToCondition<double> btc(v, emptySet);
    BOOST_CHECK(btc.dependsOnVariable(var));
    BOOST_CHECK( ! btc.dependsOnVariable(varOut));
    BOOST_CHECK( ! btc.dependsOnVariable(var1));
    BOOST_CHECK( ! btc.dependsOnVariable(var2));
    BOOST_CHECK( ! btc.dependsOnVariable(var3));
    BOOST_CHECK( ! btc.constValue());
    BOOST_CHECK_THROW(btc.eval(emptySt), out_of_range); // 'a' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! btc.eval(st)));
  });
  BOOST_CHECK_NO_THROW({ // 'a'(d) in {d1, d2..d3}
    const BelongToCondition<double> btc(v, setOfConstants);
    BOOST_CHECK(btc.dependsOnVariable(var));
    BOOST_CHECK( ! btc.dependsOnVariable(varOut));
    BOOST_CHECK( ! btc.dependsOnVariable(var1));
    BOOST_CHECK( ! btc.dependsOnVariable(var2));
    BOOST_CHECK( ! btc.dependsOnVariable(var3));
    BOOST_CHECK( ! btc.constValue());
    BOOST_CHECK_THROW(btc.eval(emptySt), out_of_range); // 'a' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(btc.eval(st)));
  });
  BOOST_CHECK_NO_THROW({ // 'e'(dOut) in {d1, d2..d3}
    const BelongToCondition<double> btc(vOut, setOfConstants);
    BOOST_CHECK( ! btc.dependsOnVariable(var));
    BOOST_CHECK(btc.dependsOnVariable(varOut));
    BOOST_CHECK( ! btc.dependsOnVariable(var1));
    BOOST_CHECK( ! btc.dependsOnVariable(var2));
    BOOST_CHECK( ! btc.dependsOnVariable(var3));
    BOOST_CHECK( ! btc.constValue());
    BOOST_CHECK_THROW(btc.eval(emptySt), out_of_range); // 'e' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! btc.eval(st)));
  });
  BOOST_CHECK_NO_THROW({ // d in {'b', 'c'..d3} = {d1, d2..d3}
    const BelongToCondition<double> btc(c, setWithVariables);
    BOOST_CHECK( ! btc.dependsOnVariable(var));
    BOOST_CHECK( ! btc.dependsOnVariable(varOut));
    BOOST_CHECK(btc.dependsOnVariable(var1));
    BOOST_CHECK(btc.dependsOnVariable(var2));
    BOOST_CHECK( ! btc.dependsOnVariable(var3));
    BOOST_CHECK( ! btc.constValue());
    BOOST_CHECK_THROW(btc.eval(emptySt), out_of_range); // 'a' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(btc.eval(st)));
  });
  BOOST_CHECK_NO_THROW({ // dOut in {'b', 'c'..d3} = {d1, d2..d3}
    const BelongToCondition<double> btc(cOut, setWithVariables);
    BOOST_CHECK( ! btc.dependsOnVariable(var));
    BOOST_CHECK( ! btc.dependsOnVariable(varOut));
    BOOST_CHECK(btc.dependsOnVariable(var1));
    BOOST_CHECK(btc.dependsOnVariable(var2));
    BOOST_CHECK( ! btc.dependsOnVariable(var3));
    BOOST_CHECK( ! btc.constValue());
    BOOST_CHECK_THROW(btc.eval(emptySt), out_of_range); // 'e' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! btc.eval(st)));
  });
  BOOST_CHECK_NO_THROW({ // 'a'(d) in {'b', 'c'..d3} = {d1, d2..d3}
    const BelongToCondition<double> btc(v, setWithVariables);
    BOOST_CHECK(btc.dependsOnVariable(var));
    BOOST_CHECK( ! btc.dependsOnVariable(varOut));
    BOOST_CHECK(btc.dependsOnVariable(var1));
    BOOST_CHECK(btc.dependsOnVariable(var2));
    BOOST_CHECK( ! btc.dependsOnVariable(var3));
    BOOST_CHECK( ! btc.constValue());
    BOOST_CHECK_THROW(btc.eval(emptySt), out_of_range); // 'a' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(btc.eval(st)));
  });
  BOOST_CHECK_NO_THROW({ // 'e'(dOut) in {'b', 'c'..d3} = {d1, d2..d3}
    const BelongToCondition<double> btc(vOut, setWithVariables);
    BOOST_CHECK( ! btc.constValue());
    BOOST_CHECK( ! btc.dependsOnVariable(var));
    BOOST_CHECK(btc.dependsOnVariable(varOut));
    BOOST_CHECK(btc.dependsOnVariable(var1));
    BOOST_CHECK(btc.dependsOnVariable(var2));
    BOOST_CHECK( ! btc.dependsOnVariable(var3));
    BOOST_CHECK_THROW(btc.eval(emptySt), out_of_range); // 'e' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! btc.eval(st)));
  });
}

BOOST_AUTO_TEST_CASE(notPredicate_usecases) {
  bool b;
  const string var = "a", varOut = "b";
  constexpr double d = 6.3, d1 = 2.3, d2 = 4.5, d3 = 9.8, dOut = 3.1;
  const SymbolsTable emptySt,
    st{{var, d}, {varOut, dOut}};

  // Not(bool_constant)
  {const Not notFalse(make_shared<const BoolConst>(false));
    BOOST_CHECK_NO_THROW(BOOST_CHECK(notFalse.eval(emptySt)));
    BOOST_CHECK( ! notFalse.dependsOnVariable(var));
    BOOST_CHECK( ! notFalse.dependsOnVariable(varOut));
    BOOST_CHECK(b = (bool) notFalse.constValue());
    if(b) BOOST_CHECK(*notFalse.constValue());}
  {const Not notTrue(make_shared<const BoolConst>(true));
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! notTrue.eval(emptySt)));
    BOOST_CHECK( ! notTrue.dependsOnVariable(var));
    BOOST_CHECK( ! notTrue.dependsOnVariable(varOut));
    BOOST_CHECK(b = (bool) notTrue.constValue());
    if(b) BOOST_CHECK( ! *notTrue.constValue());}

  // Not(BelongToCondition)
  const shared_ptr<const NumericExpr>
    c = make_shared<const NumericConst>(d),
    cOut = make_shared<const NumericConst>(dOut),
    c1 = make_shared<const NumericConst>(d1),
    c2 = make_shared<const NumericConst>(d2),
    c3 = make_shared<const NumericConst>(d3),
    v = make_shared<const NumericVariable>(var),
    vOut = make_shared<const NumericVariable>(varOut);

  ValueSet * const pSetOfConstants = new ValueSet;
  pSetOfConstants-> // {d1, d2..d3}
    add(ValueOrRange(c1)).
    add(ValueOrRange(c2, c3));
  const shared_ptr<const IValues<double>>
    setOfConstants = shared_ptr<const ValueSet>(pSetOfConstants);

  // membership test using only constants
  BOOST_CHECK_NO_THROW({ // not(dOut in {d1, d2..d3})
    const Not notFalse(
      make_shared<const BelongToCondition<double>>(cOut, setOfConstants));
    BOOST_CHECK( ! notFalse.dependsOnVariable(var));
    BOOST_CHECK( ! notFalse.dependsOnVariable(varOut));
    BOOST_CHECK(notFalse.constValue());
    BOOST_CHECK(*notFalse.constValue());
    BOOST_CHECK_NO_THROW(BOOST_CHECK(notFalse.eval(emptySt)));
  });
  BOOST_CHECK_NO_THROW({ // not(d in {d1, d2..d3})
    const Not notTrue(
      make_shared<const BelongToCondition<double>>(c, setOfConstants));
    BOOST_CHECK( ! notTrue.dependsOnVariable(var));
    BOOST_CHECK( ! notTrue.dependsOnVariable(varOut));
    BOOST_CHECK(notTrue.constValue());
    BOOST_CHECK( ! *notTrue.constValue());
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! notTrue.eval(emptySt)));
  });

  // membership test using variables as well
  BOOST_CHECK_NO_THROW({ // not('b'(dOut) in {d1, d2..d3})
    const Not notFalse(
      make_shared<const BelongToCondition<double>>(vOut, setOfConstants));
    BOOST_CHECK( ! notFalse.dependsOnVariable(var));
    BOOST_CHECK(notFalse.dependsOnVariable(varOut));
    BOOST_CHECK( ! notFalse.constValue());
    BOOST_CHECK_THROW(notFalse.eval(emptySt), out_of_range); // 'b' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK(notFalse.eval(st)));
  });
  BOOST_CHECK_NO_THROW({ // not('a'(d) in {d1, d2..d3})
    const Not notTrue(
      make_shared<const BelongToCondition<double>>(v, setOfConstants));
    BOOST_CHECK(notTrue.dependsOnVariable(var));
    BOOST_CHECK( ! notTrue.dependsOnVariable(varOut));
    BOOST_CHECK( ! notTrue.constValue());
    BOOST_CHECK_THROW(notTrue.eval(emptySt), out_of_range); // 'a' not found
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! notTrue.eval(st)));
  });
}

BOOST_AUTO_TEST_CASE(typesConstraint_usecases) {
  bool b;
  AllEntities *pAe = new AllEntities;
  AllEntities &ae = *pAe;
  shared_ptr<const AllEntities> spAe(pAe);
  MovingEntities me(spAe);
  BankEntities be(spAe);
  TypesConstraint tc;
  BOOST_CHECK(tc.longestMatchLength() == 0U);
  BOOST_CHECK(tc.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(tc.validate(spAe));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(me)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be)));

  BOOST_REQUIRE_NO_THROW({ // Available: 2 x t0 + 2 x t1 + 2 x t2 + t3
    ae += make_shared<const Entity>(0U, "e0", "t0", false, "true", 1.);
    ae += make_shared<const Entity>(1U, "e1", "t0", false, "false", 2.);
    ae += make_shared<const Entity>(2U, "e2", "t1", false, "true", 3.);
    ae += make_shared<const Entity>(3U, "e3", "t1", false, "false", 4.);
    ae += make_shared<const Entity>(4U, "e4", "t2", false, "true", 5.);
    ae += make_shared<const Entity>(5U, "e5", "t2", false, "false", 6.);
    ae += make_shared<const Entity>(6U, "e6", "t3", false, "true", 7.);
  });

  BOOST_CHECK_NO_THROW(tc.validate(spAe));

  BOOST_CHECK_THROW(tc.addTypeRange("t0", 5U, 3U),
    logic_error); // reversed range limits

  // exactly 1 't1' is required
  BOOST_CHECK_NO_THROW(tc.addTypeRange("t1", 1U, 1U));
  BOOST_CHECK(tc.longestMatchLength() == 1U);
  BOOST_CHECK(tc.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(tc.validate(spAe));

  BOOST_CHECK_THROW(tc.validate(spAe, 0U),
    logic_error); // asking for 1 't1' when capacity is 0
  BOOST_CHECK_THROW(tc.validate(spAe, 1U, 2.),
    logic_error); // asking for 1 't1' (weighing >= 3) when maxLoad is 2

  BOOST_CHECK_THROW(tc.addTypeRange("t1"),
    logic_error); // duplicate type

  // Empty configurations don't match anylonger
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! tc.matches(me)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! tc.matches(be)));

  // Entities 2 and 3 (of type 't1') match. The rest don't
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>{2U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(me)));
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>{3U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>{0U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! tc.matches(me)));
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>{1U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! tc.matches(be)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>{4U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! tc.matches(me)));
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>{5U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! tc.matches(be)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>{6U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! tc.matches(me)));

  // There's no match when there are more than 1 't1' entities
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>({2U, 3U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! tc.matches(be)));

  BOOST_CHECK_NO_THROW({ // apart from the 't1', expect also an _unknown_ 't' type
    const auto clonedC = tc.clone();
    TypesConstraint* clonedTc =
      dynamic_cast<TypesConstraint*>(const_cast<IConfigConstraint*>(
        clonedC.get()));
    b = false;
    BOOST_CHECK(b = (nullptr != clonedTc));
    if(b) {
      BOOST_CHECK_NO_THROW(clonedTc->addTypeRange("t"));
      BOOST_CHECK(clonedTc->longestMatchLength() == UINT_MAX);
      BOOST_CHECK(clonedTc->longestMismatchLength() == UINT_MAX);
      BOOST_CHECK_THROW(clonedTc->validate(spAe), logic_error);
    }
  });

  BOOST_CHECK_NO_THROW({ // apart from the 't1', expect also 3 't0's (only 2 are available)
    const auto clonedC = tc.clone();
    TypesConstraint* clonedTc =
      dynamic_cast<TypesConstraint*>(const_cast<IConfigConstraint*>(
        clonedC.get()));
    b = false;
    BOOST_CHECK(b = (nullptr != clonedTc));
    if(b) {
      BOOST_CHECK_NO_THROW(clonedTc->addTypeRange("t0", 3U, 3U));
      BOOST_CHECK(clonedTc->longestMatchLength() == 4U);
      BOOST_CHECK(clonedTc->longestMismatchLength() == UINT_MAX);
      BOOST_CHECK_THROW(clonedTc->validate(spAe), logic_error);
    }
  });

  // at least 2 't0' are required (and the 't1' from before) and 1 't2' is optional
  BOOST_CHECK_NO_THROW(tc.addTypeRange("t2", 0U, 1U).validate(spAe));
  BOOST_CHECK(tc.longestMatchLength() == 2U);
  BOOST_CHECK(tc.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(tc.addTypeRange("t0", 2U).validate(spAe));
  BOOST_CHECK(tc.longestMatchLength() == UINT_MAX);
  BOOST_CHECK(tc.longestMismatchLength() == UINT_MAX);

  // There's a match for the following sets
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>({0U, 1U, 2U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be))); // 2 x t0 + t1
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>({0U, 1U, 3U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be))); // 2 x t0 + t1
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>({0U, 1U, 2U, 4U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be))); // 2 x t0 + t1 + t2
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>({0U, 1U, 3U, 4U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be))); // 2 x t0 + t1 + t2
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>({0U, 1U, 2U, 5U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be))); // 2 x t0 + t1 + t2
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>({0U, 1U, 3U, 5U}));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(tc.matches(be))); // 2 x t0 + t1 + t2

  // Other sets produce no match
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>({0U, 2U})); // t0 + t1
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! tc.matches(be))); // missing 't0'
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>({0U, 1U, 2U, 3U})); // 2 x t0 + 2 x t1
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! tc.matches(be))); // extra 't1'
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>({0U, 1U, 2U, 4U, 5U})); // 2 x t0 + t1 + 2 x t2
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! tc.matches(be))); // extra 't2'
  BOOST_REQUIRE_NO_THROW(be = vector<unsigned>({0U, 1U, 2U, 6U})); // 2 x t0 + t1 + t3
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! tc.matches(be))); // unwanted type 't3'
}

BOOST_AUTO_TEST_CASE(idsConstraint_usecases) {
  AllEntities *pAe = new AllEntities;
  AllEntities &ae = *pAe;
  shared_ptr<const AllEntities> spAe(pAe);
  MovingEntities me(spAe);
  BankEntities be(spAe);
  IdsConstraint ic;
  BOOST_CHECK(ic.longestMatchLength() == 0U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(spAe));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(be)));

  BOOST_REQUIRE_NO_THROW({ // Available: 2 x t0 + 2 x t1 + 2 x t2 + t3 + t4 + t5
    ae += make_shared<const Entity>(0U, "e0", "t0", false, "true");
    ae += make_shared<const Entity>(1U, "e1", "t0", false, "false");
    ae += make_shared<const Entity>(2U, "e2", "t1", false, "true");
    ae += make_shared<const Entity>(3U, "e3", "t1", false, "false");
    ae += make_shared<const Entity>(4U, "e4", "t2", false, "true");
    ae += make_shared<const Entity>(5U, "e5", "t2", false, "false");
    ae += make_shared<const Entity>(6U, "e6", "t3", false, "true");
    ae += make_shared<const Entity>(7U, "e7", "t4", false, "true");
    ae += make_shared<const Entity>(8U, "e8", "t5", false, "true");
  });

  // the mandatory ids exceed the available ones
  {IdsConstraint ic1;
    for(unsigned i = 0U; i < 50U; ++i)
      ic1.addUnspecifiedMandatory();
    BOOST_CHECK(ic1.longestMatchLength() == 50U);
    BOOST_CHECK(ic1.longestMismatchLength() == UINT_MAX);
    BOOST_CHECK_THROW(ic1.validate(spAe), logic_error);

    ic1.setUnbounded();
    BOOST_CHECK(ic1.longestMatchLength() == UINT_MAX);
    BOOST_CHECK(ic1.longestMismatchLength() == 49U);}

  // A single unspecified mandatory id matches any id, but not empty sets nor more than 1 id
  {MovingEntities me1(spAe);
    IdsConstraint ic1;
    ic1.addUnspecifiedMandatory();
    BOOST_CHECK_NO_THROW(ic1.validate(spAe));
    BOOST_CHECK(ic1.longestMatchLength() == 1U);
    BOOST_CHECK(ic1.longestMismatchLength() == UINT_MAX);
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic1.matches(me)));
    for(size_t i = 0ULL, lim = ae.count(); i < lim; ++i)
      BOOST_TEST_CONTEXT("for id = `"<<i<<'`') {
        BOOST_REQUIRE_NO_THROW(me1 = vector<unsigned>{(unsigned)i});
        BOOST_CHECK_NO_THROW(BOOST_CHECK(ic1.matches(me1)));
      }
    BOOST_REQUIRE_NO_THROW(me1 = vector<unsigned>({0U, 1U}));
    BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic1.matches(me1)));}

  BOOST_CHECK_NO_THROW(ic.validate(spAe)); // no id constraints yet

  // Setting id 0 as optional
  BOOST_CHECK_NO_THROW(ic.addOptionalId(0U));
  BOOST_CHECK(ic.longestMatchLength() == 1U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(spAe));
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me))); // empty config matches
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(be))); // empty config matches
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>{0U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me))); // id 0 matches
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>{1U});
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // 1 doesn't match

  // cannot reuse id 0
  BOOST_CHECK_THROW(ic.addOptionalId(0U), logic_error);
  BOOST_CHECK_THROW(ic.addMandatoryId(0U), logic_error);
  BOOST_CHECK_THROW(ic.addAvoidedId(0U), logic_error);
  BOOST_CHECK_THROW(ic.addMandatoryGroup(vector<unsigned>({0U, 1U})),
    logic_error);
  BOOST_CHECK_THROW(ic.addOptionalGroup(vector<unsigned>({0U, 2U})),
    logic_error);

  // Setting id 1 as mandatory
  BOOST_CHECK_NO_THROW(ic.addMandatoryId(1U));
  BOOST_CHECK(ic.longestMatchLength() == 2U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(spAe));
  BOOST_CHECK_THROW(ic.validate(spAe, 0U), logic_error); // the mandatory id 1 exceeds the capacity 0
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>{1U}); // id 1 only
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({0U, 1U})); // ids 0 and 1
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>{}); // empty config
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>{0U}); // id 0 only
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // id 1 is missing
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({0U, 1U, 2U})); // ids 0, 1 and 2
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // id 2 wasn't expected

  // Setting id 2 as avoided
  BOOST_CHECK_NO_THROW(ic.addAvoidedId(2U));
  BOOST_CHECK(ic.longestMatchLength() == 2U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(spAe));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>{1U}); // id 1 only
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({0U, 1U})); // ids 0 and 1
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 2U})); // ids 1 and 2
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // id 2 is avoided
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({0U, 1U, 2U})); // ids 0, 1 and 2
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // id 2 is avoided
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 3U})); // ids 1 and 3
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // id 3 wasn't expected

  // Allowing one of the group 3 and 4
  BOOST_CHECK_NO_THROW(ic.addOptionalGroup(vector<unsigned>({3U, 4U})));
  BOOST_CHECK(ic.longestMatchLength() == 3U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(spAe));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>{1U}); // id 1 only
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 3U})); // ids 1 and 3
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 4U})); // ids 1 and 4
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({0U, 1U})); // ids 0 and 1
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({0U, 1U, 3U})); // ids 0, 1 and 3
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({0U, 1U, 4U})); // ids 0, 1 and 4
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 2U})); // ids 1 and 2
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // id 2 is avoided
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({0U, 1U, 2U})); // ids 0, 1 and 2
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // id 2 is avoided
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 3U, 4U})); // ids 1, 3 and 4
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // only one of 3 and 4 can appear
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({0U, 1U, 3U, 4U})); // ids 0, 1, 3 and 4
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // only one of 3 and 4 can appear

  // Expecting one of the group 5 and 6
  BOOST_CHECK_NO_THROW(ic.addMandatoryGroup(vector<unsigned>({5U, 6U})));
  BOOST_CHECK(ic.longestMatchLength() == 4U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(spAe));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 5U})); // id 1 and 5
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 6U})); // id 1 and 6
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 5U, 6U})); // id 1, 5 and 6
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // only one of 5 and 6 can appear
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 5U, 7U})); // id 1, 5 and 7
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // unexpected id 7

  // Expecting 1 unspecified mandatory id from the rest of the id-s
  BOOST_CHECK_NO_THROW(ic.addUnspecifiedMandatory());
  BOOST_CHECK(ic.longestMatchLength() == 5U);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(spAe));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 5U, 7U})); // id 1, 5 and 7
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 6U, 8U})); // id 1, 6 and 8
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 6U, 7U, 8U})); // id 1, 6, 7 and 8
  BOOST_CHECK_NO_THROW(BOOST_CHECK( ! ic.matches(me))); // 7 and 8 are 2 extra id-s,  but expecting only one

  // Removing the capacity limit condition
  BOOST_CHECK_NO_THROW(ic.setUnbounded());
  BOOST_CHECK(ic.longestMatchLength() == UINT_MAX);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_NO_THROW(ic.validate(spAe));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 5U, 7U})); // id 1, 5 and 7
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 6U, 8U})); // id 1, 6 and 8
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));
  BOOST_REQUIRE_NO_THROW(me = vector<unsigned>({1U, 6U, 7U, 8U})); // id 1, 6, 7 and 8
  BOOST_CHECK_NO_THROW(BOOST_CHECK(ic.matches(me)));

  // Cannot use invalid id 50
  BOOST_CHECK_NO_THROW(ic.addMandatoryId(50U));
  BOOST_CHECK(ic.longestMatchLength() == UINT_MAX);
  BOOST_CHECK(ic.longestMismatchLength() == UINT_MAX);
  BOOST_CHECK_THROW(ic.validate(spAe), logic_error);
}

BOOST_AUTO_TEST_CASE(configConstraints_usecases) {
  AllEntities *pAe = new AllEntities;
  AllEntities &ae = *pAe;
  shared_ptr<const AllEntities> spAe(pAe);
  BOOST_REQUIRE_NO_THROW({ // Available: 2 x t0 + 2 x t1 + 2 x t2 + t3 + t4 + t5
    ae += make_shared<const Entity>(0U, "e0", "t0", false, "true");
    ae += make_shared<const Entity>(1U, "e1", "t0", false, "false");
    ae += make_shared<const Entity>(2U, "e2", "t1", false, "true");
    ae += make_shared<const Entity>(3U, "e3", "t1", false, "false");
    ae += make_shared<const Entity>(4U, "e4", "t2", false, "true");
    ae += make_shared<const Entity>(5U, "e5", "t2", false, "false");
    ae += make_shared<const Entity>(6U, "e6", "t3", false, "true");
    ae += make_shared<const Entity>(7U, "e7", "t4", false, "true");
    ae += make_shared<const Entity>(8U, "e8", "t5", false, "true");
  });
  const size_t countAvailEnts = ae.count();
  BankEntities be(spAe);

  BOOST_CHECK_NO_THROW({ // allow nothing
    ConfigConstraints cc(grammar::ConstraintsVec {}, spAe);
    BOOST_CHECK(cc.empty());
    BOOST_CHECK(cc.allowed());
    BOOST_CHECK( ! cc.check(be = vector<unsigned>{}));
    BOOST_CHECK( ! cc.check(
      be = vector<unsigned>({0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U})));
  });

  BOOST_CHECK_NO_THROW({ // allow anything
    ConfigConstraints cc(grammar::ConstraintsVec {}, spAe, false);
    BOOST_CHECK(cc.empty());
    BOOST_CHECK( ! cc.allowed());
    BOOST_CHECK(cc.check(be = vector<unsigned>{}));
    BOOST_CHECK(cc.check(
      be = vector<unsigned>({0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U})));
  });

  BOOST_CHECK_NO_THROW({ // required more entities than available
    shared_ptr<const IdsConstraint> ic = make_shared<const IdsConstraint>();
    IdsConstraint *pIc = const_cast<IdsConstraint*>(ic.get());

    // asking for all available entities
    for(size_t i = 0ULL; i < countAvailEnts; ++i)
      pIc->addUnspecifiedMandatory();
    BOOST_CHECK_NO_THROW(ConfigConstraints(grammar::ConstraintsVec{ic}, spAe));
    BOOST_CHECK_NO_THROW(ConfigConstraints(grammar::ConstraintsVec{ic}, spAe, false));

    // asking for an extra entity
    pIc->addUnspecifiedMandatory();
    BOOST_CHECK_THROW(ConfigConstraints(grammar::ConstraintsVec{ic}, spAe),
      logic_error);
    BOOST_CHECK_THROW(ConfigConstraints(grammar::ConstraintsVec{ic}, spAe, false),
      logic_error);
  });

  BOOST_CHECK_NO_THROW({ // invalid id 50
    shared_ptr<const IdsConstraint> ic = make_shared<const IdsConstraint>();
    IdsConstraint *pIc = const_cast<IdsConstraint*>(ic.get());
    pIc->addAvoidedId(50U); // not(50)

    BOOST_CHECK_THROW(ConfigConstraints(grammar::ConstraintsVec{ic}, spAe),
      logic_error);
    BOOST_CHECK_THROW(ConfigConstraints(grammar::ConstraintsVec{ic}, spAe, false),
      logic_error);
  });

  BOOST_CHECK_NO_THROW({ // invalid type 't'
    shared_ptr<const TypesConstraint> tc = make_shared<const TypesConstraint>();
    TypesConstraint *pTc = const_cast<TypesConstraint*>(tc.get());
    pTc->addTypeRange("t", 2U, 2U); // 't'{2}

    BOOST_CHECK_THROW(ConfigConstraints(grammar::ConstraintsVec{tc}, spAe),
      logic_error);
    BOOST_CHECK_THROW(ConfigConstraints(grammar::ConstraintsVec{tc}, spAe, false),
      logic_error);
  });

  BOOST_CHECK_NO_THROW({ // invalid id 50 and  type 't', but postponed validation
    shared_ptr<const IdsConstraint> ic = make_shared<const IdsConstraint>();
    IdsConstraint *pIc = const_cast<IdsConstraint*>(ic.get());
    pIc->addAvoidedId(50U); // not(50)

    shared_ptr<const TypesConstraint> tc = make_shared<const TypesConstraint>();
    TypesConstraint *pTc = const_cast<TypesConstraint*>(tc.get());
    pTc->addTypeRange("t", 2U, 2U); // 't'{2}

    ConfigConstraints(grammar::ConstraintsVec{ic, tc}, spAe, true, true);
    ConfigConstraints(grammar::ConstraintsVec{ic, tc}, spAe, false, true);
  });

  BOOST_CHECK_NO_THROW({ // 2 alternative constraints
    shared_ptr<const IdsConstraint> ic = make_shared<const IdsConstraint>();
    IdsConstraint *pIc = const_cast<IdsConstraint*>(ic.get());
    pIc-> // not(1) (2|3)? (6|7|8) .{1}
      addAvoidedId(1U).
      addOptionalGroup(vector<unsigned>{2U, 3U}).
      addMandatoryGroup(vector<unsigned>{6U, 7U, 8U}).
      addUnspecifiedMandatory();

    shared_ptr<const TypesConstraint> tc = make_shared<const TypesConstraint>();
    TypesConstraint *pTc = const_cast<TypesConstraint*>(tc.get());
    pTc-> // 't0'{1,2} 't1'{,2} 't5'{1,}
      addTypeRange("t0", 1U, 2U). // 0 | 1 | (0 1)
      addTypeRange("t1", 0U, 2U). // (not(2) not(3)) | 2 | 3 | (2 3)
      addTypeRange("t5", 1U);     // 8

    ConfigConstraints cc(grammar::ConstraintsVec{ic, tc}, spAe);
    BOOST_CHECK( ! cc.empty());
    BOOST_CHECK(cc.allowed());

    ConfigConstraints notCc(grammar::ConstraintsVec{ic, tc}, spAe, false);
    BOOST_CHECK( ! notCc.empty());
    BOOST_CHECK( ! notCc.allowed());

    // These are some of the configurations which satisfy 1st constraint
    BOOST_CHECK(cc.check(be = vector<unsigned>({0U, 6U})));
    BOOST_CHECK( ! notCc.check(be));
    BOOST_CHECK(cc.check(be = vector<unsigned>({2U, 4U, 7U})));
    BOOST_CHECK( ! notCc.check(be));
    BOOST_CHECK(cc.check(be = vector<unsigned>({3U, 5U, 8U})));
    BOOST_CHECK( ! notCc.check(be));
    BOOST_CHECK(cc.check(be = vector<unsigned>({0U, 3U, 8U})));
    BOOST_CHECK( ! notCc.check(be));

    // These are some of the configurations which satisfy 2nd constraint
    BOOST_CHECK(cc.check(be = vector<unsigned>({0U, 8U})));
    BOOST_CHECK( ! notCc.check(be));
    BOOST_CHECK(cc.check(be = vector<unsigned>({1U, 2U, 8U})));
    BOOST_CHECK( ! notCc.check(be));
    BOOST_CHECK(cc.check(be = vector<unsigned>({0U, 1U, 3U, 8U})));
    BOOST_CHECK( ! notCc.check(be));
    BOOST_CHECK(cc.check(be = vector<unsigned>({0U, 1U, 2U, 3U, 8U})));
    BOOST_CHECK( ! notCc.check(be));

    // Several configurations which should not satisfy neither of the constraints
    BOOST_CHECK( ! cc.check( // both constraints require at least 2 id-s
      be = vector<unsigned>{}));
    BOOST_CHECK(notCc.check(be));
    BOOST_CHECK( ! cc.check( // both constraints require at least 2 id-s
      be = vector<unsigned>{0U}));
    BOOST_CHECK(notCc.check(be));
    BOOST_CHECK( ! cc.check( // id 7 cannot appear together with 8; id 7 has an unsuitable type 't4'
      be = vector<unsigned>({0U, 7U, 8U})));
    BOOST_CHECK(notCc.check(be));
  });
}

BOOST_AUTO_TEST_CASE(transferConstraints_usecases) {
  AllEntities *pAe = new AllEntities;
  AllEntities &ae = *pAe;
  shared_ptr<const AllEntities> spAe(pAe);
  BOOST_REQUIRE_NO_THROW({ // Available: 2 x t0 + 2 x t1 + 2 x t2 + t3 + t4 + t5
    ae += make_shared<const Entity>(0U, "e0", "t0", false, "true", 1.);
    ae += make_shared<const Entity>(1U, "e1", "t0", false, "false", 2.);
    ae += make_shared<const Entity>(2U, "e2", "t1", false, "true", 3.);
    ae += make_shared<const Entity>(3U, "e3", "t1", false, "false", 4.);
    ae += make_shared<const Entity>(4U, "e4", "t2", false, "true", 5.);
    ae += make_shared<const Entity>(5U, "e5", "t2", false, "false", 6.);
    ae += make_shared<const Entity>(6U, "e6", "t3", false, "true", 7.);
    ae += make_shared<const Entity>(7U, "e7", "t4", false, "true", 8.);
    ae += make_shared<const Entity>(8U, "e8", "t5", false, "true", 9.);
  });
  const size_t countAvailEnts = ae.count();
  const unsigned cap = UINT_MAX;
  const double maxLoad = DBL_MAX;
  BankEntities be(spAe);

  BOOST_CHECK_NO_THROW({ // allow nothing
    TransferConstraints cc(grammar::ConstraintsVec {}, spAe, cap, maxLoad);
    BOOST_CHECK(cc.empty());
    BOOST_CHECK(cc.allowed());
    BOOST_CHECK(cc.minRequiredCapacity() == 0U);
    BOOST_CHECK( ! cc.check(be = vector<unsigned>{}));
    BOOST_CHECK( ! cc.check(be = vector<unsigned>({0U, 8U})));
  });

  BOOST_CHECK_NO_THROW({ // allow anything in groups not larger than 2 entities
    const unsigned cap = 2U;
    TransferConstraints notCc(grammar::ConstraintsVec {}, spAe, cap, maxLoad,
                              false);
    BOOST_CHECK(notCc.empty());
    BOOST_CHECK( ! notCc.allowed());
    BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    BOOST_CHECK(notCc.check(be = vector<unsigned>{}));
    BOOST_CHECK(notCc.check(be = vector<unsigned>({0U, 8U})));
    BOOST_CHECK( ! notCc.check(be = vector<unsigned>({0U, 2U, 8U})));
  });

  BOOST_CHECK_NO_THROW({ // allow anything in groups not heavier than 12
    const double maxLoad = 12.;
    TransferConstraints notCc(grammar::ConstraintsVec {}, spAe, cap, maxLoad,
                              false);
    BOOST_CHECK(notCc.empty());
    BOOST_CHECK( ! notCc.allowed());
    BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    BOOST_CHECK(notCc.check(be = vector<unsigned>{})); // 0 < 12
    BOOST_CHECK(notCc.check(
                be = vector<unsigned>({2U, 8U}))); // 3+9=12 <= 12
    BOOST_CHECK( ! notCc.check(
                be = vector<unsigned>({0U, 2U, 8U}))); // 1+3+9=13 > 12
  });

  BOOST_CHECK_NO_THROW({ // required more entities than available
    shared_ptr<const IdsConstraint> ic = make_shared<const IdsConstraint>();
    IdsConstraint *pIc = const_cast<IdsConstraint*>(ic.get());

    // empty ids constraint
    BOOST_CHECK_NO_THROW({
      TransferConstraints cc(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad);
      BOOST_CHECK(cc.minRequiredCapacity() == 0U);});
    BOOST_CHECK_NO_THROW({
      TransferConstraints notCc(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad,
                             false);
      BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);});

    // asking for all available entities but one
    for(size_t i = 1ULL; i < countAvailEnts; ++i) // all except one for now
      pIc->addUnspecifiedMandatory();
    BOOST_CHECK_NO_THROW({
      TransferConstraints cc(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad);
      BOOST_CHECK(cc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);});
    BOOST_CHECK_NO_THROW({
      TransferConstraints notCc(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad,
                             false);
      BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);});

    // adding the last one here
    pIc->addUnspecifiedMandatory();
    BOOST_CHECK_NO_THROW({
      TransferConstraints cc(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad);
      BOOST_CHECK(cc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);});
    BOOST_CHECK_NO_THROW({
      TransferConstraints notCc(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad,
                             false);
      BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);});

    // asking for an extra entity
    pIc->addUnspecifiedMandatory();
    BOOST_CHECK_THROW(
      TransferConstraints(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad),
      logic_error);
    BOOST_CHECK_THROW(
      TransferConstraints(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad,
                          false),
      logic_error);
  });

  BOOST_CHECK_NO_THROW({ // required more entities than the capacity=2
    constexpr unsigned cap = 2U;

    shared_ptr<const IdsConstraint> ic = make_shared<const IdsConstraint>();
    IdsConstraint *pIc = const_cast<IdsConstraint*>(ic.get());
    for(unsigned i = 0U; i < cap; ++i) // filling the capacity
      pIc->addUnspecifiedMandatory();
    BOOST_CHECK_NO_THROW(
      TransferConstraints(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad));
    BOOST_CHECK_NO_THROW(
      TransferConstraints(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad,
                          false));

    // asking for an extra entity
    pIc->addUnspecifiedMandatory();
    BOOST_CHECK_THROW(
      TransferConstraints(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad),
      logic_error);
    BOOST_CHECK_THROW(
      TransferConstraints(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad,
                          false),
      logic_error);
  });

  BOOST_CHECK_NO_THROW({ // invalid id 50
    shared_ptr<const IdsConstraint> ic = make_shared<const IdsConstraint>();
    IdsConstraint *pIc = const_cast<IdsConstraint*>(ic.get());
    pIc->addAvoidedId(50U); // not(50)

    BOOST_CHECK_THROW(
      TransferConstraints(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad),
      logic_error);
    BOOST_CHECK_THROW(
      TransferConstraints(grammar::ConstraintsVec{ic}, spAe, cap, maxLoad,
                          false),
      logic_error);
  });

  BOOST_CHECK_NO_THROW({ // invalid type 't'
    shared_ptr<const TypesConstraint> tc = make_shared<const TypesConstraint>();
    TypesConstraint *pTc = const_cast<TypesConstraint*>(tc.get());
    pTc->addTypeRange("t", 2U, 2U); // 't'{2}

    BOOST_CHECK_THROW(
      TransferConstraints(grammar::ConstraintsVec{tc}, spAe, cap, maxLoad),
      logic_error);
    BOOST_CHECK_THROW(
      TransferConstraints(grammar::ConstraintsVec{tc}, spAe, cap, maxLoad,
                          false),
      logic_error);
  });

  // more required entities ('t0'{2} + 't1'{1,2}) than the capacity=2
  BOOST_CHECK_NO_THROW({
    const unsigned cap = 2U;

    shared_ptr<const TypesConstraint> tc = make_shared<const TypesConstraint>();
    TypesConstraint *pTc = const_cast<TypesConstraint*>(tc.get());
    pTc->
      addTypeRange("t0", 2U, 2U). // 't0'{2}
      addTypeRange("t1", 1U, 2U); // 't1'{1, 2}

    BOOST_CHECK_THROW(
      TransferConstraints(grammar::ConstraintsVec{tc}, spAe, cap, maxLoad),
      logic_error);
    BOOST_CHECK_THROW(
      TransferConstraints(grammar::ConstraintsVec{tc}, spAe, cap, maxLoad,
                          false),
      logic_error);
  });

  // heavier entities ('t4' + 't5' => 17) than the max_load=5
  BOOST_CHECK_NO_THROW({
    const double maxLoad = 5.;

    shared_ptr<const TypesConstraint> tc = make_shared<const TypesConstraint>();
    TypesConstraint *pTc = const_cast<TypesConstraint*>(tc.get());
    pTc->
      addTypeRange("t4", 1U, 1U). // 't4'{1}
      addTypeRange("t5", 1U, 1U); // 't5'{1}

    BOOST_CHECK_THROW(
      TransferConstraints(grammar::ConstraintsVec{tc}, spAe, cap, maxLoad),
      logic_error);
    BOOST_CHECK_THROW(
      TransferConstraints(grammar::ConstraintsVec{tc}, spAe, cap, maxLoad,
                          false),
      logic_error);
  });

  BOOST_CHECK_NO_THROW({ // 2 alternative constraints
    const unsigned cap = 2U;
    const double maxLoad = 12.;

    shared_ptr<const IdsConstraint> ic =
      make_shared<const IdsConstraint>();
    IdsConstraint *pIc = const_cast<IdsConstraint*>(ic.get());
    pIc-> // not(1) (2|3)? (6|7|8) .{1}
      addAvoidedId(1U).
      addOptionalGroup(vector<unsigned>{2U, 3U}).
      addMandatoryGroup(vector<unsigned>{6U, 7U, 8U}).
      addUnspecifiedMandatory();

    shared_ptr<const TypesConstraint> tc =
      make_shared<const TypesConstraint>();
    TypesConstraint *pTc = const_cast<TypesConstraint*>(tc.get());
    pTc-> // 't0'{1,2} 't1'{,2} 't5'{1,}
      addTypeRange("t0", 1U, 2U). // 0 | 1 | (0 1)
      addTypeRange("t1", 0U, 2U). // (not(2) not(3)) | 2 | 3 | (2 3)
      addTypeRange("t5", 1U);     // 8

    TransferConstraints cc(grammar::ConstraintsVec{ic, tc}, spAe, cap, maxLoad);
    BOOST_CHECK( ! cc.empty());
    BOOST_CHECK(cc.allowed());
    BOOST_CHECK(cc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);

    TransferConstraints notCc(grammar::ConstraintsVec{ic, tc}, spAe, cap, maxLoad,
                              false);
    BOOST_CHECK( ! notCc.empty());
    BOOST_CHECK( ! notCc.allowed());
    BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);

    // These are some of the configurations which satisfy 1st constraint
    BOOST_CHECK(cc.check(be = vector<unsigned>({0U, 6U}))); // weight: 1+7 < 12
    BOOST_CHECK( ! notCc.check(be));
    BOOST_CHECK(cc.check(be = vector<unsigned>({0U, 8U}))); // weight: 1+9 < 12
    BOOST_CHECK( ! notCc.check(be));
    BOOST_CHECK(cc.check(be = vector<unsigned>({4U, 6U}))); // weight: 5+7 <= 12
    BOOST_CHECK( ! notCc.check(be));

    // These are some of the configurations which satisfy 2nd constraint
    BOOST_CHECK(cc.check(be = vector<unsigned>({1U, 8U}))); // weight: 2+9 < 12
    BOOST_CHECK( ! notCc.check(be));

    // Several configurations which should not satisfy neither of the constraints
    BOOST_CHECK( ! cc.check( // both constraints require at least 2 id-s
      be = vector<unsigned>{}));
    BOOST_CHECK(notCc.check(be));
    BOOST_CHECK( ! cc.check( // both constraints require at least 2 id-s
      be = vector<unsigned>{0U}));
    BOOST_CHECK(notCc.check(be));
    BOOST_CHECK( ! cc.check( // missing unspecified mandatory id
      be = vector<unsigned>({2U, 7U})));
    BOOST_CHECK(notCc.check(be));
    BOOST_CHECK( ! cc.check( // capacity overflow: 3 > 2
      be = vector<unsigned>({0U, 2U, 6U})));
    BOOST_CHECK( ! notCc.check(be)); // negated constraints still need to respect capacity
    BOOST_CHECK( ! cc.check( // beyond max load: 6+8 = 14 > 12
      be = vector<unsigned>({5U, 7U})));
    BOOST_CHECK( ! notCc.check(be)); // negated constraints still need to respect maxLoad
  });

  BOOST_CHECK_NO_THROW({ //
    shared_ptr<const IdsConstraint> ic1 =
      make_shared<const IdsConstraint>();
    IdsConstraint *pIc1 = const_cast<IdsConstraint*>(ic1.get());
    pIc1-> // not(1) (2|3)? (6|7|8) .{1}
      addAvoidedId(1U).
      addOptionalGroup(vector<unsigned>{2U, 3U}).
      addMandatoryGroup(vector<unsigned>{6U, 7U, 8U}).
      addUnspecifiedMandatory();

    shared_ptr<const IdsConstraint> ic2 =
      make_shared<const IdsConstraint>();
    IdsConstraint *pIc2 = const_cast<IdsConstraint*>(ic2.get());
    pIc2-> // .{4,} not(8)
      addUnspecifiedMandatory().addUnspecifiedMandatory().
      addUnspecifiedMandatory().addUnspecifiedMandatory().
      addAvoidedId(8U).
      setUnbounded();

    shared_ptr<const TypesConstraint> tc =
      make_shared<const TypesConstraint>();
    TypesConstraint *pTc = const_cast<TypesConstraint*>(tc.get());
    pTc-> // 't0'{1,2} 't1'{,2} 't5'{1}
      addTypeRange("t0", 1U, 2U). // 0 | 1 | (0 1)
      addTypeRange("t1", 0U, 2U). // (not(2) not(3)) | 2 | 3 | (2 3)
      addTypeRange("t5", 1U, 1U); // 8

    {
      /*
       Using only the first Ids constraint, which can be accommodated
       by a raft/bridge with a capacity of 3:
       - 1 for the optionals `(2|3)?`
       - 1 for the mandatories `(6|7|8)`
       - 1 for the unspecified mandatory `.{1}`
      */
      TransferConstraints cc(grammar::ConstraintsVec{ic1}, spAe,
                             cap, maxLoad);
      BOOST_CHECK( ! cc.empty());
      BOOST_CHECK(cc.allowed());
      BOOST_CHECK(cc.minRequiredCapacity() == 3U);

      TransferConstraints notCc(grammar::ConstraintsVec{ic1}, spAe,
                                cap, maxLoad, false);
      BOOST_CHECK( ! notCc.empty());
      BOOST_CHECK( ! notCc.allowed());
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
      TransferConstraints cc(grammar::ConstraintsVec{tc}, spAe,
                             cap, maxLoad);
      BOOST_CHECK( ! cc.empty());
      BOOST_CHECK(cc.allowed());
      BOOST_CHECK(cc.minRequiredCapacity() == 5U);

      TransferConstraints notCc(grammar::ConstraintsVec{tc}, spAe,
                                cap, maxLoad, false);
      BOOST_CHECK( ! notCc.empty());
      BOOST_CHECK( ! notCc.allowed());
      BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    }

    {
      /*
       Using the first Ids constraint and the Types constraint,
       which can be accommodated by a raft/bridge with a capacity of 5,
       value dictated by the first Types constraint, which is spacier.
      */
      TransferConstraints cc(grammar::ConstraintsVec{ic1, tc}, spAe,
                             cap, maxLoad);
      BOOST_CHECK( ! cc.empty());
      BOOST_CHECK(cc.allowed());
      BOOST_CHECK(cc.minRequiredCapacity() == 5U);

      TransferConstraints notCc(grammar::ConstraintsVec{ic1, tc}, spAe,
                                cap, maxLoad, false);
      BOOST_CHECK( ! notCc.empty());
      BOOST_CHECK( ! notCc.allowed());
      BOOST_CHECK(notCc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);
    }

    {
      /*
       Using only the second Ids constraint, which can be accommodated
       by a raft/bridge with a capacity of 3 when negated:
       - anything that is less than 4 entities and doesn't contain entity 8
      */
      TransferConstraints cc(grammar::ConstraintsVec{ic2}, spAe,
                             cap, maxLoad);
      BOOST_CHECK( ! cc.empty());
      BOOST_CHECK(cc.allowed());
      BOOST_CHECK(cc.minRequiredCapacity() == (unsigned)countAvailEnts - 1U);

      TransferConstraints notCc(grammar::ConstraintsVec{ic2}, spAe,
                                cap, maxLoad, false);
      BOOST_CHECK( ! notCc.empty());
      BOOST_CHECK( ! notCc.allowed());
      BOOST_CHECK(notCc.minRequiredCapacity() == 3U);
    }
  });
}

BOOST_AUTO_TEST_SUITE_END()

#endif // for CONFIG_CONSTRAINT_CPP and UNIT_TESTING
