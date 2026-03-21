/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.83) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2026 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef HPP_CONFIG_PARSER_DETAIL
#define HPP_CONFIG_PARSER_DETAIL

#include "configConstraint.h"
#include "configParser.h"
#include "warnings.h"

#include <climits>
#include <cstdio>

#include <exception>
#include <format>
#include <iostream>
#include <iterator>
#include <memory>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <boost/core/demangle.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/spirit/home/x3.hpp>  // IWYU pragma: keep
#include <boost/spirit/home/x3/auxiliary/attr.hpp>
#include <boost/spirit/home/x3/auxiliary/eoi.hpp>
#include <boost/spirit/home/x3/auxiliary/eps.hpp>
#include <boost/spirit/home/x3/auxiliary/guard.hpp>
#include <boost/spirit/home/x3/char/char.hpp>
#include <boost/spirit/home/x3/char/char_class.hpp>
#include <boost/spirit/home/x3/directive/omit.hpp>
#include <boost/spirit/home/x3/nonterminal/rule.hpp>
#include <boost/spirit/home/x3/numeric/real.hpp>
#include <boost/spirit/home/x3/numeric/real_policies.hpp>
#include <boost/spirit/home/x3/numeric/uint.hpp>
#include <boost/spirit/home/x3/string/literal_string.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

// Boost Spirit X3 restructured and the definition of 'expectation_failure'
// changed its namespace and location
#if __has_include(<boost/spirit/home/x3/support/expectation.hpp>)
#include <boost/spirit/home/x3/support/expectation.hpp>
using boost::spirit::x3::throwing::expectation_failure;

#elif __has_include(<boost/spirit/home/x3/directive/expect.hpp>)
#include <boost/spirit/home/x3/directive/expect.hpp>
using boost::spirit::x3::expectation_failure;

#else
#error Cannot provide a header containing \
  boost::spirit::x3[::throwing]::expectation_failure!
#endif

namespace bs = boost::spirit;
namespace bsx = bs::x3;

namespace rc::cond {
// Forward declarations
class LogicalExpr;
class ValueOrRange;
class NumericExpr;
}  // namespace rc::cond

/**
Defines the following grammar:

LogicalExpr ::= (if \s+)? (Condition | not \s* \( \s* Condition \s* \))

Condition ::= true | false |
  (MathExpr | \( \s* MathExpr \s* \)) \s+ (not \s+)? in \s* { ValueSet }

ValueSet ::= \s* (ValueOrRange (\s* , \s* ValueOrRange)* \s* | eps)

ValueOrRange ::= MathExpr (?!(\s+ \.\.)) | MathExpr \s+ \.\. \s+ MathExpr

MathExpr ::= LeftRecursiveMathExpr | UnfencedOperand

LeftRecursiveMathExpr ::= Operand \s+ mod \s+ Operand

Operand ::= UnfencedOperand | \( \s* LeftRecursiveMathExpr \s* \)

UnfencedOperand ::= Value | add \s* \( \s* MathExpr \s* , \s* MathExpr \s* \)

Value ::= <<constant>> | Variable

Variable ::= % TypeName %

CrossingDurationForConfigurations ::= <<unsigned>> \s+ : \s+ Configurations

Configurations ::= Configuration ( \s+ ; \s+ Configuration )*

Configuration ::= TypesConfig | IdsConfig

TypesConfig ::= TypeTerm ( \s+ \+ \s+ TypeTerm)*

TypeTerm ::= (<<unsigned>>(\+|-)? \s+ x \s+)? TypeName

TypeName ::= <<alpha>> (<<alnum>> | - | _)*

IdsConfig ::= - | IdsTerm (\s+ IdsTerm)* \s+ (...)?

IdsTerm ::= \* | !?<<unsigned>> | <<unsigned>>\? | IdsGroup\??

IdsGroup ::= \(<<unsigned>> (\s* \| \s* <<unsigned>>)+\)
*/
namespace rc::grammar {

/// Allows non-terminals to display the location of the error
class ErrHandler {
 public:
  /// Handle errors by forwarding the exception and displaying its location
  template <typename Iterator, typename Exception, typename Context>
    requires std::is_nothrow_copy_constructible_v<Exception>
  [[nodiscard]] bsx::error_handler_result on_error(Iterator& first,
                                                   const Iterator& last,
                                                   const Exception& ex,
                                                   const Context& /*context*/) {
    bsx::error_handler<Iterator>{first, last, std::cerr}(ex.where(), "");
    throw ex;
  }
};

// Rule tags (id-s)
class LogicalExprTag : public ErrHandler {};
class MathExprTag {};
class ConditionTag {};
class LeftRecursiveMathExprTag {};
class OperandTag {};
class UnfencedOperandTag {};
class VariableTag {};
class ValueSetTag : public ErrHandler {};
class ValueOrRangeTag {};
class ValueTag {};
class CrossingDurationForConfigurationsTag : public ErrHandler {};
class ConfigurationsTag : public ErrHandler {};
class TypesConfigTag {};
class TypeNameTag {};
class IdsConfigTag {};
class IdsGroupTag {};

// NOLINTBEGIN(bugprone-throwing-static-initialization) : Startup crash allowed

// Declaring the rules
inline const bsx::rule<LogicalExprTag,
                       std::shared_ptr<const rc::cond::LogicalExpr>>
    logicalExpr{"logicalExpr"};
inline const bsx::rule<ConditionTag,
                       std::shared_ptr<const rc::cond::LogicalExpr>>
    condition{"condition"};
inline const bsx::rule<ValueSetTag, rc::cond::ValueSet> valueSet{"valueSet"};
inline const bsx::rule<ValueOrRangeTag,
                       std::shared_ptr<const rc::cond::ValueOrRange>>
    valueOrRange{"valueOrRange"};
inline const bsx::rule<MathExprTag,
                       std::shared_ptr<const rc::cond::NumericExpr>>
    mathExpr{"mathExpr"};
inline const bsx::rule<LeftRecursiveMathExprTag,
                       std::shared_ptr<const rc::cond::NumericExpr>>
    leftRecursiveMathExpr{"leftRecursiveMathExpr"};
inline const bsx::rule<OperandTag, std::shared_ptr<const rc::cond::NumericExpr>>
    operand{"operand"};
inline const bsx::rule<UnfencedOperandTag,
                       std::shared_ptr<const rc::cond::NumericExpr>>
    unfencedOperand{"unfencedOperand"};
inline const bsx::rule<ValueTag, std::shared_ptr<const rc::cond::NumericExpr>>
    value{"value"};
inline const bsx::rule<VariableTag, std::string> variable{"variable"};
inline const bsx::rule<CrossingDurationForConfigurationsTag,
                       ConfigurationsTransferDurationInitType>
    crossingDurationForConfigurations{"crossingDurationForConfigurations"};
inline const bsx::rule<ConfigurationsTag, ConstraintsVec> configurations{
    "configurations"};
inline const bsx::rule<TypesConfigTag, rc::cond::TypesConstraint> typesConfig{
    "typesConfig"};
inline const bsx::rule<TypeNameTag, std::string> typeName{"typeName"};
inline const bsx::rule<IdsConfigTag, rc::cond::IdsConstraint> idsConfig{
    "idsConfig"};
inline const bsx::rule<IdsGroupTag, std::vector<unsigned>> idsGroup{"idsGroup"};

/// A policy that behaves like the standard bsx::real_policies<double> used by
/// bsx::double_, but ignores non-finite double values as only finite values
/// should be handled by scenarios.
class FiniteDoublePolicy : public bsx::real_policies<double> {
 public:
  /// Fail to parse "inf[inity]" and "INF[INITY]" on purpose
  template <typename Iterator, typename Attribute>
  static bool parse_inf(Iterator&, const Iterator&, Attribute&) noexcept {
    return false;
  }

  /// Fail to parse "nan" and "NAN" on purpose (despite they are supported even
  /// on clang, due to the '-fhonor-nans' flag)
  template <typename Iterator, typename Attribute>
  static bool parse_nan(Iterator&, const Iterator&, Attribute&) noexcept {
    return false;
  }
};

// Define a finite-only double parser
inline const bsx::real_parser<double, FiniteDoublePolicy> finiteDoubleParser{};

// Defining the rules and their semantic actions
namespace bsa = bsx::ascii;
using bsa::alnum;
using bsa::alpha;
using bsa::blank;
using bsa::char_;

using bsx::attr;
using bsx::eoi;
using bsx::eps;
using bsx::lit;
using bsx::omit;
using bsx::uint_;

using boost::fusion::at_c;

// Suppress -Wglobal-constructors for parser-expression objects created here.
MUTE_GLOBAL_CTOR_WARN

// Semantic actions and definition for 'logicalExpr', 'valueOrRange' and 'value'
template <class Type>
inline const auto createWith1Param = [](const auto& ctx) {
  _val(ctx) = std::make_shared<const Type>(_attr(ctx));
};
const auto forwardAttr = [](const auto& ctx) { _val(ctx) = _attr(ctx); };

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access) :
// Normal grammar expressions
// NOLINTBEGIN(bugprone-chained-comparison) : Normal grammar expressions
const auto logicalExpr_def =
    omit[-("if" > +blank)] >
    ((("not" >> omit[*blank]) > ('(' >> omit[*blank]) >
      (condition >> omit[*blank]) > ')')[createWith1Param<rc::cond::Not>] |
     condition[forwardAttr]);

// Semantic actions and definition for 'condition'
inline auto setBoolConst() noexcept {  // noexcept because just returns lambda
  return [](const auto& ctx) {
    using namespace std;

    bool b{true};
    istringstream iss{boost::get<string>(_attr(ctx))};
    iss >> boolalpha >> b;
    _val(ctx) = make_shared<const rc::cond::BoolConst>(b);
  };
}

const auto belongTo = [](const auto& ctx) {
  using namespace rc::cond;

  auto belongExpr = std::make_shared<const BelongToCondition<double>>(
      boost::get<std::shared_ptr<const NumericExpr>>(at_c<0>(_attr(ctx))),
      std::make_shared<const ValueSet>(at_c<2>(_attr(ctx))));

  if (at_c<1>(_attr(ctx)))  // optional<bool> set on true when parsing 'not'
    _val(ctx) = std::make_shared<const Not>(belongExpr);
  else
    _val(ctx) = belongExpr;
};

const auto condition_def =
    (bsx::string("true") | bsx::string("false"))[setBoolConst()] |
    ((((mathExpr | (('(' >> omit[*blank]) > (mathExpr >> omit[*blank]) > ')')) >
       omit[+blank]) >>
      -(("not" > omit[+blank]) >> attr(true))) > ("in" >> omit[*blank]) > '{' >
     valueSet > '}')[belongTo];

// Semantic actions and definition for 'valueSet'
const auto appendToValueSet = [](const auto& ctx) {
  _val(ctx).add(*_attr(ctx));
};

const auto valueSet_def =
    omit[*blank] >> !blank >>
    (((valueOrRange[appendToValueSet] % (*blank >> ',' >> *blank)) >>
      omit[*blank] >> !blank) |
     eps);

// Semantic actions and definition for 'valueOrRange'
template <class Type>
inline const auto createWith2Params = [](const auto& ctx) {
  _val(ctx) =
      std::make_shared<const Type>(at_c<0>(_attr(ctx)), at_c<1>(_attr(ctx)));
};

const auto valueOrRange_def =
    (mathExpr >> !(+blank >> ".."))[createWith1Param<rc::cond::ValueOrRange>] |
    (mathExpr > omit[+blank] > ".." > omit[+blank] >
     mathExpr)[createWith2Params<rc::cond::ValueOrRange>];

// Definition for 'mathExp'
const auto mathExpr_def = leftRecursiveMathExpr | unfencedOperand;

// Definition for 'leftRecursiveMathExpr'
const auto leftRecursiveMathExpr_def =
    ((operand >> omit[+blank] >> "mod") > omit[+blank] >
     operand)[createWith2Params<rc::cond::Modulus>];

// Definition for 'operand'
const auto operand_def =
    (('(' >> omit[*blank]) > (leftRecursiveMathExpr >> omit[*blank]) > ')') |
    unfencedOperand;

// Definition for 'unfencedOperand'
const auto unfencedOperand_def =
    ((lit("add") >> omit[*blank]) > ('(' >> omit[*blank]) >
     (mathExpr >> omit[*blank]) > (',' >> omit[*blank]) >
     (mathExpr >> omit[*blank]) > ')')[createWith2Params<rc::cond::Addition>] |
    value[forwardAttr];

// Definition for 'value'
const auto value_def =
    finiteDoubleParser[createWith1Param<rc::cond::NumericConst>] |
    variable[createWith1Param<rc::cond::NumericVariable>];

// Definition for 'variable'
const auto variable_def = '%' > typeName > '%';

// Semantic actions and definition for 'crossingDurationForConfigurations'
const auto setDurationForConfigs = [](const auto& ctx) {
  _val(ctx).setDuration(_attr(ctx));
};
const auto configsWithSameDuration = [](const auto& ctx) {
  _val(ctx).setConstraints(std::move(_attr(ctx)));
};
const auto crossingDurationForConfigurations_def =
    eps > uint_[setDurationForConfigs] > +blank > ':' > +blank >
    configurations[configsWithSameDuration];

// Semantic actions and definition for 'configurations'
const auto appendConfig = [](const auto& ctx) {
  _val(ctx).emplace_back(std::move(_attr(ctx).clone()));
};
const auto configurations_def =
    (*blank >> ((typesConfig[appendConfig] | idsConfig[appendConfig]) %
                (+blank >> ';' >> +blank)) >>
     *blank) > !blank  // consume any remaining blanks
    ;

// Semantic actions and definition for 'typesConfig'
const auto exactlyOneOfType = [](const auto& ctx) {
  _val(ctx).addTypeRange(_attr(ctx), 1U, 1U);
};
const auto exactlyNOfType = [](const auto& ctx) {
  _val(ctx).addTypeRange(at_c<1>(_attr(ctx)), at_c<0>(_attr(ctx)),
                         at_c<0>(_attr(ctx)));
};
const auto atLeastNOfType = [](const auto& ctx) {
  _val(ctx).addTypeRange(at_c<1>(_attr(ctx)), at_c<0>(_attr(ctx)), UINT_MAX);
};
const auto atMostNOfType = [](const auto& ctx) {
  _val(ctx).addTypeRange(at_c<1>(_attr(ctx)), 0U, at_c<0>(_attr(ctx)));
};
const auto typesConfig_def =
    ((((uint_ >> !char_("+-") >> omit[(+blank >> 'x') > +blank]) >
       typeName)[exactlyNOfType] |
      ((uint_ >> '+') > omit[(+blank >> 'x') > +blank] >
       typeName)[atLeastNOfType] |
      ((uint_ >> '-') > omit[(+blank >> 'x') > +blank] >
       typeName)[atMostNOfType] |
      typeName[exactlyOneOfType]) %
     ((+blank >> '+') > +blank)) > &(blank | eoi);

// Definition for 'typeName'
const auto typeName_def = alpha > *(alnum | char_("-_")) > !alnum >
                          !char_("-_");

// Semantic actions and definition for 'idsConfig'
const auto unspecifiedMandatory = [](const auto& ctx) {
  _val(ctx).addUnspecifiedMandatory();
};

const auto avoidedId = [](const auto& ctx) {
  _val(ctx).addAvoidedId(_attr(ctx));
};

const auto optionalId = [](const auto& ctx) {
  _val(ctx).addOptionalId(_attr(ctx));
};

const auto mandatoryId = [](const auto& ctx) {
  _val(ctx).addMandatoryId(_attr(ctx));
};

const auto handleIdGroup = [](const auto& ctx) {
  if (at_c<1>(_attr(ctx)))  // optional<char> set on '?' when parsing '?'
    _val(ctx).addOptionalGroup(at_c<0>(_attr(ctx)));
  else
    _val(ctx).addMandatoryGroup(at_c<0>(_attr(ctx)));
};

const auto unboundedIdConfig = [](const auto& ctx) {
  _val(ctx).setUnbounded();
};

const auto idsConfig_def =
    (lit('-') | (((lit('*')[unspecifiedMandatory] | ('!' > uint_)[avoidedId] |
                   (uint_ >> '?')[optionalId] |
                   (uint_ >> !char_("?+-") >> !(+blank >> 'x'))[mandatoryId] |
                   (idsGroup >> -char_('?'))[handleIdGroup]) %
                  +blank) >>
                 -(+blank >> bsx::string("...")[unboundedIdConfig]))) >
    &(blank | eoi);

// Definition for 'idsGroup'
const auto idsGroup_def =
    ('(' >> omit[*blank]) > uint_ >
    (+((omit[*blank] >> '|' >> omit[*blank]) > uint_) >> omit[*blank]) > ')';
// NOLINTEND(bugprone-chained-comparison)
// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
// NOLINTEND(bugprone-throwing-static-initialization)

// Rules registration
BOOST_SPIRIT_DEFINE(logicalExpr,
                    condition,
                    valueSet,
                    valueOrRange,
                    mathExpr,
                    leftRecursiveMathExpr,
                    operand,
                    unfencedOperand,
                    value,
                    variable,
                    crossingDurationForConfigurations,
                    configurations,
                    typesConfig,
                    typeName,
                    idsConfig,
                    idsGroup)

UNMUTE_WARNING

/**
  Extracts the semantic from an expression based on a given rule.
  Displays on cerr any encountered problem.

  @param s the expression to parse
  @param usedRule the grammar rule to be checked against the expression
  @param expr the resulted semantic object

  @return true for a correct expression and false otherwise
*/
template <class RuleType>
[[nodiscard]] bool tackle(const std::string& s,
                          const RuleType& usedRule,
                          typename RuleType::attribute_type& expr) {
  using namespace std;

  auto reachedPoint = cbegin(s);
  const auto itEnd = cend(s);
  string err;
  err.reserve(512ULL);
  const auto errIt{back_inserter(err)};

  try {
    const bool anyMatch{bsx::parse(reachedPoint, itEnd, usedRule, expr)};
    if (anyMatch && reachedPoint == itEnd)
      return true;

    // NOLINTNEXTLINE(bugprone-dangling-handle) : view of 's' won't dangle
    const string_view problem{reachedPoint, itEnd};
    format_to(errIt, "\tThere is a problem with: `{:s}`\n", problem);

  } catch (const expectation_failure<string::const_iterator>& e) {
    // NOLINTNEXTLINE(bugprone-dangling-handle) : view of 's' won't dangle
    const string_view got{e.where(), itEnd};
    format_to(errIt, "\tExpecting {}\n\t      Got `{:s}`",
              boost::core::demangle(e.which().c_str()), got);

  } catch (const exception& e) {
    format_to(errIt, "{}", e.what());
  }

  println(stderr, "Expression `{}` didn't parse correctly.\n{}", s, err);
  fflush(stderr);

  return false;
}

}  // namespace rc::grammar

#endif  // !HPP_CONFIG_PARSER_DETAIL
