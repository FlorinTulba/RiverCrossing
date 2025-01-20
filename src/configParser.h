/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_CONFIG_PARSER
#define H_CONFIG_PARSER

#include "absConfigConstraint.h"

namespace rc::grammar {

/// Unchecked vector of constraints - right after parsing
using ConstraintsVec =
    std::vector<std::shared_ptr<const cond::IConfigConstraint>>;

/// Unchecked vector of configurations with same duration - right after parsing
class ConfigurationsTransferDurationInitType {
 public:
  /// Move the constraints in
  ConfigurationsTransferDurationInitType& setConstraints(
      ConstraintsVec&& constraints_) noexcept;

  [[nodiscard]] ConstraintsVec&&
  moveConstraints() noexcept;  ///< move the constraints out

  ConfigurationsTransferDurationInitType& setDuration(unsigned d);

  [[nodiscard]] unsigned duration() const noexcept;

  PROTECTED :

      /// Unchecked vector of configuration of same duration
      ConstraintsVec _constraints;
  unsigned _duration{};  ///< duration of the provided configurations
};

/**
Read a NightMode syntax.
@param s the string to parse
@return the contained logical expression or NULL when reporting errors
*/
[[nodiscard]] std::shared_ptr<const cond::LogicalExpr> parseNightModeExpr(
    const std::string& s) noexcept;

/**
Read a CanRow syntax.
@param s the string to parse
@return the contained logical expression or NULL when reporting errors
*/
[[nodiscard]] std::shared_ptr<const cond::LogicalExpr> parseCanRowExpr(
    const std::string& s) noexcept;

/**
Read an AllowedLoads syntax.
@param s the string to parse
@return the contained value set or NULL when reporting errors
*/
[[nodiscard]] std::unique_ptr<const cond::IValues<double>>
parseAllowedLoadsExpr(const std::string& s) noexcept;

/**
Read a Configurations syntax.
@param s the string to parse
@return the contained configurations or None when reporting errors
*/
[[nodiscard]] std::optional<ConstraintsVec> parseConfigurationsExpr(
    const std::string& s) noexcept;

/**
Read a CrossingDurationForConfigurations syntax.
@param s the string to parse
@return the contained configurations plus the duration or None when reporting
errors
*/
[[nodiscard]] std::optional<ConfigurationsTransferDurationInitType>
parseCrossingDurationForConfigurationsExpr(const std::string& s) noexcept;

}  // namespace rc::grammar

#endif  // H_CONFIG_PARSER not defined
