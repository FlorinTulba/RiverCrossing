/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#include "precompiled.h"

#ifdef UNIT_TESTING

/*
The 'configParser.h' header isn't helpful for Unit Testing this module.
Therefore, the testing is performed from within the cpp file.

Including 'test/configParser.hpp' here avoids recompiling the project
when only the tests need to be updated
*/
#define CPP_CONFIG_PARSER
#include "configParser.hpp"
#undef CPP_CONFIG_PARSER

#endif  // UNIT_TESTING defined

#include "configParserDetail.hpp"
#include "util.h"

#include <iostream>

#include <boost/core/demangle.hpp>

using namespace rc::cond;

namespace rc::grammar {

ConfigurationsTransferDurationInitType&
ConfigurationsTransferDurationInitType::setConstraints(
    ConstraintsVec&& constraints_) noexcept {
  _constraints = std::move(constraints_);
  return *this;
}

ConstraintsVec&&
ConfigurationsTransferDurationInitType::moveConstraints() noexcept {
  return std::move(_constraints);
}

ConfigurationsTransferDurationInitType&
ConfigurationsTransferDurationInitType::setDuration(unsigned d) {
  if (!d)
    throw std::logic_error{HERE.function_name() +
                           " - 0 isn't allowed as duration parameter!"s};
  _duration = d;
  return *this;
}

unsigned ConfigurationsTransferDurationInitType::duration() const noexcept {
  return _duration;
}

std::shared_ptr<const LogicalExpr> parseNightModeExpr(
    const std::string& s) noexcept {
  std::shared_ptr<const LogicalExpr> result;
  if (tackle(s, logicalExpr, result))
    return result;

  return {};
}

std::shared_ptr<const LogicalExpr> parseCanRowExpr(
    const std::string& s) noexcept {
  std::shared_ptr<const LogicalExpr> result;
  if (tackle(s, logicalExpr, result))
    return result;

  return {};
}

std::unique_ptr<const IValues<double>> parseAllowedLoadsExpr(
    const std::string& s) noexcept {
  ValueSet result;
  if (tackle(s, valueSet, result))
    return std::make_unique<const ValueSet>(result);

  return {};
}

std::optional<ConstraintsVec> parseConfigurationsExpr(
    const std::string& s) noexcept {
  ConstraintsVec result;
  if (tackle(s, configurations, result))
    return result;

  return {};
}

std::optional<ConfigurationsTransferDurationInitType>
parseCrossingDurationForConfigurationsExpr(const std::string& s) noexcept {
  ConfigurationsTransferDurationInitType result;
  if (tackle(s, crossingDurationForConfigurations, result))
    return result;

  return {};
}

}  // namespace rc::grammar
