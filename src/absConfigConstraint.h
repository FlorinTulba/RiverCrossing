/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_ABS_CONFIG_CONSTRAINT
#define H_ABS_CONFIG_CONSTRAINT

#include "symbolsTable.h"

#include <memory>
#include <climits>
#include <iostream>

#include <boost/optional/optional.hpp>

namespace rc {

namespace ent {

// Forward declarations
class AllEntities;
class IsolatedEntities;

} // namespace ent

namespace cond {

struct IConfigConstraint; // forward declaration

/// Allows an extension of the validation of IConfigConstraint
struct IConfigConstraintValidatorExt /*abstract*/ {
  virtual ~IConfigConstraintValidatorExt() /*= 0*/ {}

  /// @throw logic_error if cfg does not respect all the extensions
  virtual void check(const IConfigConstraint &cfg,
                     const std::shared_ptr<const ent::AllEntities> &allEnts) const = 0;
};

/// Neutral IConfigConstraint validator extension
struct DefConfigConstraintValidatorExt final : IConfigConstraintValidatorExt {
  /// Allows sharing the default instance
  static const std::shared_ptr<const DefConfigConstraintValidatorExt>& INST();

  /// @throw logic_error if cfg does not respect all the extensions
  void check(const IConfigConstraint &cfg,
             const std::shared_ptr<const ent::AllEntities> &allEnts)
      const override final {}

    #ifndef UNIT_TESTING // leave ctor public only for Unit tests
protected:
    #endif

  DefConfigConstraintValidatorExt() {}
};

/// Expresses a configuration for the raft(/bridge) / banks
struct IConfigConstraint /*abstract*/ {
  virtual ~IConfigConstraint() /*= 0*/ {}

  /**
  Checks the validity of this constraint using entities information,
  raft/bridge capacity and additional validation logic

  @throw logic_error for an invalid constraint
  */
  virtual void validate(const std::shared_ptr<const ent::AllEntities> &allEnts,
      unsigned capacity = UINT_MAX,
      const std::shared_ptr<const IConfigConstraintValidatorExt> &valExt
        = DefConfigConstraintValidatorExt::INST()) const = 0;

  /// @return a copy of this on heap
  virtual std::unique_ptr<const IConfigConstraint> clone() const = 0;

  /// Is there a match between the provided collection and the constraint's data?
  virtual bool matches(const ent::IsolatedEntities &ents) const = 0;

  /// @return the length of the longest possible match
  virtual unsigned longestMatchLength() const {
    return UINT_MAX;
  }

  /// @return the length of the longest possible mismatch
  virtual unsigned longestMismatchLength() const {
    return UINT_MAX;
  }

  /// Describes the constraint
  virtual std::string toString() const = 0;
};

/// Base class for Numeric and Logical expressions
template<typename Type>
class AbsExpr /*abstract*/ {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  boost::optional<Type> val; ///< cached result of the expression if possible

public:
  virtual ~AbsExpr()/* = 0*/ {}

  /// Provide the cached result of the expression when this is a constant
  const boost::optional<Type>& constValue() const {return val;}

  /// Checks if there is a dependency on `varName`
  virtual bool dependsOnVariable(const std::string &varName) const {
    return false;
  }

  /**
    @param st the symbols table
    @return the expression evaluated using the provided symbols' values
    @throw out_of_range when referring symbols missing from the table
  */
  virtual Type eval(const SymbolsTable &st) const = 0;

  virtual std::string toString() const = 0;
};

/// Base of numeric expression types
struct NumericExpr /*abstract*/ : AbsExpr<double> {};

/// Base of logical expression types
struct LogicalExpr /*abstract*/ : AbsExpr<bool> {};

/// A set of values (expressions that can be evaluated using a symbols table)
template<typename Type>
struct IValues {
  virtual ~IValues() /*= 0*/ {}

  virtual bool empty() const = 0;

  virtual bool constSet() const = 0; ///< are the values all constant?

  /// Checks if there is a dependency on `varName`
  virtual bool dependsOnVariable(const std::string &varName) const {
    return false;
  }

  /// Checks if v is covered by current values and ranges based on the symbols' table
  virtual bool contains(const Type &v, const SymbolsTable &st = {}) const = 0;

  virtual std::string toString() const = 0;
};

} // namespace cond
} // namespace rc

namespace std {

std::ostream& operator<<(std::ostream &os, const rc::cond::IConfigConstraint &c);

template<typename Type>
std::ostream& operator<<(std::ostream &os, const rc::cond::AbsExpr<Type> &e) {
  os<<e.toString();
  return os;
}

template<typename Type>
std::ostream& operator<<(std::ostream &os, const rc::cond::IValues<Type> &vs) {
  os<<vs.toString();
  return os;
}

} // namespace std

#endif // H_ABS_CONFIG_CONSTRAINT not defined
