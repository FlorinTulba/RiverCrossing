/*
Part of the RiverCrossing project,
which allows to describe and solve River Crossing puzzles:
https://en.wikipedia.org/wiki/River_crossing_puzzle

Requires Boost installation (www.boost.org).

(c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#include "configParser.h"
#include "configConstraint.h"

#include <iostream>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/core/demangle.hpp>

namespace bs = boost::spirit;
namespace bsx = bs::x3;

namespace rc {

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
namespace grammar {
using namespace bsx;
using namespace cond;

/// Allows non-terminals to display the location of the error
struct ErrHandler {
  /// Handle errors by forwarding the exception and displaying its location
  template <typename Iterator, typename Exception, typename Context>
  error_handler_result on_error(Iterator &first, const Iterator &last,
      const Exception &ex, const Context &context) {
    error_handler<Iterator>(first, last, std::cerr)(ex.where(), "");
    throw ex;
  }
};

// Rule tags (id-s)
struct LogicalExprTag : ErrHandler {};
struct MathExprTag {};
struct ConditionTag {};
struct LeftRecursiveMathExprTag {};
struct OperandTag {};
struct UnfencedOperandTag {};
struct VariableTag {};
struct ValueSetTag : ErrHandler {};
struct ValueOrRangeTag {};
struct ValueTag {};
struct CrossingDurationForConfigurationsTag : ErrHandler {};
struct ConfigurationsTag : ErrHandler {};
struct TypesConfigTag {};
struct TypeNameTag {};
struct IdsConfigTag {};
struct IdsGroupTag {};

// Declaring the rules
rule<LogicalExprTag, std::shared_ptr<const LogicalExpr>>
  logicalExpr("logicalExpr");
rule<ConditionTag, std::shared_ptr<const LogicalExpr>>
  condition("condition");
rule<ValueSetTag, ValueSet> valueSet("valueSet");
rule<ValueOrRangeTag, std::shared_ptr<const ValueOrRange>>
  valueOrRange("valueOrRange");
rule<MathExprTag, std::shared_ptr<const NumericExpr>>
  mathExpr("mathExpr");
rule<LeftRecursiveMathExprTag, std::shared_ptr<const NumericExpr>>
  leftRecursiveMathExpr("leftRecursiveMathExpr");
rule<OperandTag, std::shared_ptr<const NumericExpr>>
  operand("operand");
rule<UnfencedOperandTag, std::shared_ptr<const NumericExpr>>
  unfencedOperand("unfencedOperand");
rule<ValueTag, std::shared_ptr<const NumericExpr>> value("value");
rule<VariableTag, std::string> variable("variable");
rule<CrossingDurationForConfigurationsTag,
    ConfigurationsTransferDurationInitType>
  crossingDurationForConfigurations("crossingDurationForConfigurations");
rule<ConfigurationsTag, ConstraintsVec> configurations("configurations");
rule<TypesConfigTag, TypesConstraint> typesConfig("typesConfig");
rule<TypeNameTag, std::string> typeName("typeName");
rule<IdsConfigTag, IdsConstraint> idsConfig("idsConfig");
rule<IdsGroupTag, std::vector<unsigned>> idsGroup("idsGroup");

// Defining the rules and their semantic actions
namespace bsa = bsx::ascii;
using bsa::char_;
using bsa::blank;
using bsa::alnum;
using bsa::alpha;

using boost::fusion::at_c;

// Semantic actions and definition for 'logicalExpr'
template<class Type>
const auto createWith1Param = [](const auto &ctx) {
  _val(ctx) = std::make_shared<const Type>(_attr(ctx));
};
const auto forwardAttr = [](const auto &ctx) {_val(ctx) = _attr(ctx);};
const auto logicalExpr_def =
  omit[-("if" > +blank)] > (
    ("not" >> omit[*blank] > '(' >> omit[*blank]
      > condition
      >> omit[*blank] > ')')                        [createWith1Param<Not>]
    |
    condition                                       [forwardAttr]
  )
  ;

// Semantic actions and definition for 'condition'
const auto setBoolConst = [](const auto &ctx) {
  bool b = true;
  std::istringstream iss(boost::get<std::string>(_attr(ctx)));
  iss>>std::boolalpha>>b;
  _val(ctx) = std::make_shared<const BoolConst>(b);
};
const auto belongTo = [](const auto &ctx) {
  auto belongExpr = std::make_shared<const BelongToCondition<double>>(
    boost::get<std::shared_ptr<const NumericExpr>>(at_c<0>(_attr(ctx))),
    std::make_shared<const ValueSet>(at_c<2>(_attr(ctx))));
  if(at_c<1>(_attr(ctx))) // optional<bool> set on true when parsing 'not'
    _val(ctx) = std::make_shared<const Not>(belongExpr);
  else
    _val(ctx) = belongExpr;
};
const auto condition_def =
  (bsx::string("true") | bsx::string("false"))            [setBoolConst]
  |
  ((mathExpr | ('(' >> omit[*blank] > mathExpr >> omit[*blank] > ')'))
    > omit[+blank]
    >> -("not" > omit[+blank] >> attr(true))
    > "in" >> omit[*blank]
    > '{' > valueSet > '}')                               [belongTo]
  ;

// Semantic actions and definition for 'valueSet'
const auto appendToValueSet = [](const auto &ctx) {
  _val(ctx).add(*_attr(ctx));
};
const auto valueSet_def =
  omit[*blank] >> !blank >> (
    (
      (valueOrRange                               [appendToValueSet]
        % (*blank >> ',' >> *blank)
      ) >> omit[*blank] >> !blank
    )
    |
    eps
  )
  ;

// Semantic actions and definition for 'valueOrRange'
template<class Type>
const auto createWith2Params = [](const auto &ctx) {
  _val(ctx) = std::make_shared<const Type>(
    at_c<0>(_attr(ctx)), at_c<1>(_attr(ctx)));
};
const auto valueOrRange_def =
  (mathExpr >> !(+blank >> ".."))           [createWith1Param<ValueOrRange>]
  |
  (mathExpr > omit[+blank] > ".." > omit[+blank] > mathExpr)
                                            [createWith2Params<ValueOrRange>]
  ;

// Definition for 'mathExp'
const auto mathExpr_def =
  leftRecursiveMathExpr | unfencedOperand
  ;

// Definition for 'leftRecursiveMathExpr'
const auto leftRecursiveMathExpr_def =
  (operand >> omit[+blank] >> "mod" > omit[+blank]
    > operand)                                [createWith2Params<Modulus>]
  ;

// Definition for 'operand'
const auto operand_def =
  ('(' >> omit[*blank] > leftRecursiveMathExpr >> omit[*blank] > ')')
  |
  unfencedOperand
  ;

// Definition for 'unfencedOperand'
const auto unfencedOperand_def =
  (lit("add") >> omit[*blank] > '(' >> omit[*blank]
    > mathExpr >> omit[*blank] > ',' >> omit[*blank]
    > mathExpr >> omit[*blank] > ')')         [createWith2Params<Addition>]
  |
  value                                       [forwardAttr]
  ;

// Definition for 'value'
const auto value_def =
  double_       [createWith1Param<NumericConst>]
  |
  variable      [createWith1Param<NumericVariable>]
  ;

// Definition for 'variable'
const auto variable_def =
  '%' > typeName > '%'
  ;

// Semantic actions and definition for 'crossingDurationForConfigurations'
const auto setDurationForConfigs = [](const auto &ctx) {
  _val(ctx).setDuration(_attr(ctx));
};
const auto configsWithSameDuration = [](const auto &ctx) {
  _val(ctx).setConstraints(std::move(_attr(ctx)));
};
const auto crossingDurationForConfigurations_def =
  eps > uint_                           [setDurationForConfigs]
    > +blank > ':' > +blank
    > configurations                    [configsWithSameDuration]
  ;

// Semantic actions and definition for 'configurations'
const auto appendConfig = [](const auto &ctx) {
  _val(ctx).emplace_back(std::move(_attr(ctx).clone()));
};
const auto configurations_def =
  *blank
    >> (
      (
        typesConfig                       [appendConfig]
        |
        idsConfig                         [appendConfig]
      ) % (+blank >> ';' >> +blank)
    )
    >> *blank > !blank // consume any remaining blanks
  ;

// Semantic actions and definition for 'typesConfig'
const auto exactlyOneOfType = [](const auto &ctx) {
  _val(ctx).addTypeRange(_attr(ctx), 1U, 1U);
};
const auto exactlyNOfType = [](const auto &ctx) {
  _val(ctx).addTypeRange(at_c<1>(_attr(ctx)),
              at_c<0>(_attr(ctx)),
              at_c<0>(_attr(ctx)));
};
const auto atLeastNOfType = [](const auto &ctx) {
  _val(ctx).addTypeRange(at_c<1>(_attr(ctx)),
              at_c<0>(_attr(ctx)),
              UINT_MAX);
};
const auto atMostNOfType = [](const auto &ctx) {
  _val(ctx).addTypeRange(at_c<1>(_attr(ctx)),
              0U,
              at_c<0>(_attr(ctx)));
};
const auto typesConfig_def =
  (
    (
      (uint_ >> !char_("+-")
        >> omit[+blank >> 'x' > +blank]
        > typeName)                         [exactlyNOfType]
      |
      (uint_ >> '+'
        > omit[+blank >> 'x' > +blank]
        > typeName)                         [atLeastNOfType]
      |
      (uint_ >> '-'
        > omit[+blank >> 'x' > +blank]
        > typeName)                         [atMostNOfType]
      |
      typeName                              [exactlyOneOfType]
    ) % (+blank >> '+' > +blank)
  ) > &(blank | eoi)
  ;

// Definition for 'typeName'
const auto typeName_def =
  alpha > *(alnum | char_("-_"))
    > !alnum > !char_("-_")
  ;

// Semantic actions and definition for 'idsConfig'
const auto unspecifiedMandatory = [](const auto &ctx) {
  _val(ctx).addUnspecifiedMandatory();
};
const auto avoidedId = [](const auto &ctx) {
  _val(ctx).addAvoidedId(_attr(ctx));
};
const auto optionalId = [](const auto &ctx) {
  _val(ctx).addOptionalId(_attr(ctx));
};
const auto mandatoryId = [](const auto &ctx) {
  _val(ctx).addMandatoryId(_attr(ctx));
};
const auto handleIdGroup = [](const auto &ctx) {
  if(at_c<1>(_attr(ctx))) // optional<char> set on '?' when parsing '?'
    _val(ctx).addOptionalGroup(at_c<0>(_attr(ctx)));
  else
    _val(ctx).addMandatoryGroup(at_c<0>(_attr(ctx)));
};
const auto unboundedIdConfig = [](const auto &ctx) {
  _val(ctx).setUnbounded();
};
const auto idsConfig_def =
  (
    lit('-')
    |
    (
      ((
        lit('*')                            [unspecifiedMandatory]
        |
        ('!' > uint_)                       [avoidedId]
        |
        (uint_ >> '?')                      [optionalId]
        |
        (uint_ >> !char_("?+-") >> !(+blank >> 'x'))
                                            [mandatoryId]
        |
        (idsGroup >> -char_('?'))           [handleIdGroup]
      ) % +blank)
      >> -(+blank
            >> bsx::string("...")           [unboundedIdConfig]
          )
    )
  ) > &(blank | eoi)
  ;

// Definition for 'idsGroup'
const auto idsGroup_def =
  '(' >> omit[*blank] > uint_
    > +(omit[*blank] >> '|' >> omit[*blank] > uint_)
    >> omit[*blank] > ')'
  ;

// Rules registration
BOOST_SPIRIT_DEFINE(logicalExpr, condition, valueSet, valueOrRange,
  mathExpr, leftRecursiveMathExpr, operand, unfencedOperand, value, variable,
  crossingDurationForConfigurations, configurations, typesConfig, typeName,
  idsConfig, idsGroup)

/**
  Extracts the semantic from an expression based on a given rule.
  Displays on cerr any encountered problem.

  @param s the expression to parse
  @param usedRule the grammar rule to be checked against the expression
  @param expr the resulted semantic object

  @return true for a correct expression and false otherwise
*/
template<class RuleType>
bool tackle(const std::string &s, const RuleType &usedRule,
    typename RuleType::attribute_type &expr) {
  using namespace std;

  auto reachedPoint = cbegin(s), itEnd = cend(s);
  ostringstream oss;
  try {
    const bool anyMatch = bsx::parse(reachedPoint, itEnd, usedRule, expr);
    if(anyMatch && reachedPoint==itEnd)
      return true;

    oss<<"\tThere is a problem with: `"
      <<std::string(reachedPoint, itEnd)<<'`'<<endl;

  } catch(const expectation_failure<std::string::const_iterator> &e) {
    oss<<"\tExpecting "<<boost::core::demangle(e.which().c_str())<<endl;
    oss<<"\t      Got `"<<std::string(e.where(), itEnd)<<'`';

  } catch(const std::exception &e) {
    oss<<e.what();
  }

  cerr<<"Expression `"<<s<<"` didn't parse correctly."<<endl;
  cerr<<oss.str()<<endl;

  return false;
}

ConfigurationsTransferDurationInitType&
  ConfigurationsTransferDurationInitType::setConstraints(
    ConstraintsVec &&constraints_) {
  _constraints = std::move(constraints_);
  return *this;
}

ConstraintsVec&& ConfigurationsTransferDurationInitType::moveConstraints() {
  return std::move(_constraints);
}

ConfigurationsTransferDurationInitType&
      ConfigurationsTransferDurationInitType::setDuration(unsigned d) {
  if(d == 0U)
    throw std::logic_error(std::string(__func__) +
      " - 0 isn't allowed as duration parameter!");
  _duration = d;
  return *this;
}

unsigned ConfigurationsTransferDurationInitType::duration() const {
  return _duration;
}

std::shared_ptr<const LogicalExpr> parseNightModeExpr(const std::string &s) {
  std::shared_ptr<const LogicalExpr> result;
  if(tackle(s, logicalExpr, result))
    return result;

  return nullptr;
}

std::shared_ptr<const LogicalExpr> parseCanRowExpr(const std::string &s) {
  std::shared_ptr<const LogicalExpr> result;
  if(tackle(s, logicalExpr, result))
    return result;

  return nullptr;
}

std::unique_ptr<const IValues<double>> parseAllowedLoadsExpr(
      const std::string &s) {
  ValueSet result;
  if(tackle(s, valueSet, result))
    return std::make_unique<const ValueSet>(result);

  return nullptr;
}

boost::optional<ConstraintsVec> parseConfigurationsExpr(
    const std::string &s) {
  ConstraintsVec result;
  if(tackle(s, configurations, result))
    return result;

  return boost::none;
}

boost::optional<ConfigurationsTransferDurationInitType>
      parseCrossingDurationForConfigurationsExpr(const std::string &s) {
  ConfigurationsTransferDurationInitType result;
  if(tackle(s, crossingDurationForConfigurations, result))
    return result;

  return boost::none;
}

} // namespace grammar
} // namespace rc

#ifdef UNIT_TESTING

/*
The 'configParser.h' header isn't helpful for Unit Testing this module.
Therefore, the testing is performed from within the cpp file.

Including 'test/configParser.hpp' here avoids recompiling the project
when only the tests need to be updated
*/
#define CONFIG_PARSER_CPP
#include "../test/configParser.hpp"

#endif // UNIT_TESTING defined
