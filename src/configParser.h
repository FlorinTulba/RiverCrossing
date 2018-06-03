/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 @2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_CONFIG_PARSER
#define H_CONFIG_PARSER

#include <vector>
#include <string>
#include <memory>

#include <boost/optional/optional.hpp>

namespace rc {

namespace cond {

// Forward declarations
struct IConfigConstraint;
struct LogicalExpr;
template<typename> struct IValues;

} // namespace cond

namespace grammar {

/// Unchecked vector of constraints - right after parsing
typedef std::vector<std::shared_ptr<const cond::IConfigConstraint>> ConstraintsVec;

/// Unchecked vector of configurations with same duration - right after parsing
class ConfigurationsTransferDurationInitType {

  #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
  #else // keep fields protected otherwise
protected:
  #endif

  ConstraintsVec _constraints; ///< Unchecked vector of configuration of same duration
  unsigned _duration = 0U; ///< duration of the provided configurations

public:
  ConfigurationsTransferDurationInitType&
    setConstraints(ConstraintsVec &&constraints_); ///< move the constraints in
  ConstraintsVec&& moveConstraints(); ///< move the constraints out

  ConfigurationsTransferDurationInitType& setDuration(unsigned d);
  unsigned duration() const;
};

/**
Read a CanRow syntax.
@param s the string to parse
@return the contained logical expression or NULL when reporting errors
*/
std::shared_ptr<const cond::LogicalExpr> parseCanRowExpr(const std::string &s);

/**
Read an AllowedLoads syntax.
@param s the string to parse
@return the contained value set or NULL when reporting errors
*/
std::unique_ptr<const cond::IValues<double>> parseAllowedLoadsExpr(
    const std::string &s);

/**
Read a Configurations syntax.
@param s the string to parse
@return the contained configurations or None when reporting errors
*/
boost::optional<ConstraintsVec> parseConfigurationsExpr(
    const std::string &s);

/**
Read a CrossingDurationForConfigurations syntax.
@param s the string to parse
@return the contained configurations plus the duration or None when reporting errors
*/
boost::optional<ConfigurationsTransferDurationInitType>
      parseCrossingDurationForConfigurationsExpr(const std::string &s);

} // namespace grammar
} // namespace rc

#endif // H_CONFIG_PARSER not defined
