/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef HPP_CONFIG_PARSER_DETAIL
#define HPP_CONFIG_PARSER_DETAIL

#include "configConstraint.h"
#include "configParser.h"

#include <boost/fusion/include/at.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

namespace bs = boost::spirit;
namespace bsx = bs::x3;

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

// Declaring the rules
bsx::rule<LogicalExprTag, std::shared_ptr<const rc::cond::LogicalExpr>>
    logicalExpr{"logicalExpr"};
bsx::rule<ConditionTag, std::shared_ptr<const rc::cond::LogicalExpr>> condition{
    "condition"};
bsx::rule<ValueSetTag, rc::cond::ValueSet> valueSet{"valueSet"};
bsx::rule<ValueOrRangeTag, std::shared_ptr<const rc::cond::ValueOrRange>>
    valueOrRange{"valueOrRange"};
bsx::rule<MathExprTag, std::shared_ptr<const rc::cond::NumericExpr>> mathExpr{
    "mathExpr"};
bsx::rule<LeftRecursiveMathExprTag,
          std::shared_ptr<const rc::cond::NumericExpr>>
    leftRecursiveMathExpr{"leftRecursiveMathExpr"};
bsx::rule<OperandTag, std::shared_ptr<const rc::cond::NumericExpr>> operand{
    "operand"};
bsx::rule<UnfencedOperandTag, std::shared_ptr<const rc::cond::NumericExpr>>
    unfencedOperand{"unfencedOperand"};
bsx::rule<ValueTag, std::shared_ptr<const rc::cond::NumericExpr>> value{
    "value"};
bsx::rule<VariableTag, std::string> variable{"variable"};
bsx::rule<CrossingDurationForConfigurationsTag,
          ConfigurationsTransferDurationInitType>
    crossingDurationForConfigurations{"crossingDurationForConfigurations"};
bsx::rule<ConfigurationsTag, ConstraintsVec> configurations{"configurations"};
bsx::rule<TypesConfigTag, rc::cond::TypesConstraint> typesConfig{"typesConfig"};
bsx::rule<TypeNameTag, std::string> typeName{"typeName"};
bsx::rule<IdsConfigTag, rc::cond::IdsConstraint> idsConfig{"idsConfig"};
bsx::rule<IdsGroupTag, std::vector<unsigned>> idsGroup{"idsGroup"};

// Defining the rules and their semantic actions
namespace bsa = bsx::ascii;
using bsa::alnum;
using bsa::alpha;
using bsa::blank;
using bsa::char_;

using bsx::attr;
using bsx::double_;
using bsx::eoi;
using bsx::eps;
using bsx::lit;
using bsx::omit;
using bsx::uint_;

using boost::fusion::at_c;

// TODO: find a way to prevent formatting syntax definitions

// Semantic actions and definition for 'logicalExpr'
template <class Type>
const auto createWith1Param = [](const auto& ctx) noexcept {
  _val(ctx) = std::make_shared<const Type>(_attr(ctx));
};
const auto forwardAttr = [](const auto& ctx) noexcept {
  _val(ctx) = _attr(ctx);
};
const auto logicalExpr_def = omit[-("if" > +blank)] >
                             ((("not" >> omit[*blank]) > ('(' >> omit[*blank]) >
                               (condition >> omit[*blank]) >
                               ')')[createWith1Param<rc::cond::Not>] |
                              condition[forwardAttr]);

// Semantic actions and definition for 'condition'
const auto setBoolConst = [](const auto& ctx) noexcept {
  using namespace std;

  bool b{true};
  istringstream iss{boost::get<string>(_attr(ctx))};
  iss >> boolalpha >> b;
  _val(ctx) = make_shared<const rc::cond::BoolConst>(b);
};

const auto belongTo = [](const auto& ctx) noexcept {
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
    (bsx::string("true") | bsx::string("false"))[setBoolConst] |
    ((((mathExpr | (('(' >> omit[*blank]) > (mathExpr >> omit[*blank]) > ')')) >
       omit[+blank]) >>
      -(("not" > omit[+blank]) >> attr(true))) > ("in" >> omit[*blank]) > '{' >
     valueSet > '}')[belongTo];

// Semantic actions and definition for 'valueSet'
const auto appendToValueSet = [](const auto& ctx) noexcept {
  _val(ctx).add(*_attr(ctx));
};

const auto valueSet_def = omit[*blank] >> !blank >>
                          (((valueOrRange[appendToValueSet] %
                             (*blank >> ',' >> *blank)) >>
                            omit[*blank] >> !blank) |
                           eps);

// Semantic actions and definition for 'valueOrRange'
template <class Type>
const auto createWith2Params = [](const auto& ctx) {
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
const auto value_def = double_[createWith1Param<rc::cond::NumericConst>] |
                       variable[createWith1Param<rc::cond::NumericVariable>];

// Definition for 'variable'
const auto variable_def = '%' > typeName > '%';

// Semantic actions and definition for 'crossingDurationForConfigurations'
const auto setDurationForConfigs = [](const auto& ctx) {
  _val(ctx).setDuration(_attr(ctx));
};
const auto configsWithSameDuration = [](const auto& ctx) noexcept {
  _val(ctx).setConstraints(std::move(_attr(ctx)));
};
const auto crossingDurationForConfigurations_def =
    eps > uint_[setDurationForConfigs] > +blank > ':' > +blank >
    configurations[configsWithSameDuration];

// Semantic actions and definition for 'configurations'
const auto appendConfig = [](const auto& ctx) noexcept {
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
const auto unspecifiedMandatory = [](const auto& ctx) noexcept {
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

const auto unboundedIdConfig = [](const auto& ctx) noexcept {
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
const auto idsGroup_def = ('(' >> omit[*blank]) > uint_ >
                          (+((omit[*blank] >> '|' >> omit[*blank]) > uint_) >>
                           omit[*blank]) > ')';

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
  ostringstream oss;
  try {
    const bool anyMatch{bsx::parse(reachedPoint, itEnd, usedRule, expr)};
    if (anyMatch && reachedPoint == itEnd)
      return true;

    // Separate definition of problem avoids MSVC warning 4866
    const string problem{reachedPoint, itEnd};
    oss << "\tThere is a problem with: " << quoted(problem, '`') << endl;

  } catch (const bsx::expectation_failure<string::const_iterator>& e) {
    oss << "\tExpecting " << boost::core::demangle(e.which().c_str()) << endl;

    // Separate definition of problem avoids MSVC warning 4866
    const string got{e.where(), itEnd};
    oss << "\t      Got " << quoted(got, '`');

  } catch (const exception& e) {
    oss << e.what();
  }

  cerr << "Expression `" << s << "` didn't parse correctly.\n"
       << oss.str() << endl;

  return false;
}

}  // namespace rc::grammar

#endif  // !HPP_CONFIG_PARSER_DETAIL
