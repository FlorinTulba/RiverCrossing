/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.83) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2026 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_ABS_CONFIG_CONSTRAINT
#define H_ABS_CONFIG_CONSTRAINT

#include "entitiesManager.h"

#include <climits>
#include <optional>

namespace rc::cond {

class IConfigConstraint;  // forward declaration

/// Allows an extension of the validation of IConfigConstraint
class IConfigConstraintValidatorExt {
 public:
  virtual ~IConfigConstraintValidatorExt() noexcept = default;

  IConfigConstraintValidatorExt(const IConfigConstraintValidatorExt&) = delete;
  IConfigConstraintValidatorExt(IConfigConstraintValidatorExt&&) = delete;
  IConfigConstraintValidatorExt& operator=(
      const IConfigConstraintValidatorExt&) = delete;
  IConfigConstraintValidatorExt& operator=(
      IConfigConstraintValidatorExt&&) noexcept = delete;

  /// @throw logic_error if cfg does not respect all the extensions
  virtual void check(const IConfigConstraint& cfg,
                     const ent::AllEntities& allEnts) const = 0;

 protected:
  IConfigConstraintValidatorExt() noexcept = default;
};

/// Neutral IConfigConstraint validator extension
class DefConfigConstraintValidatorExt final
    : public IConfigConstraintValidatorExt {
 public:
  ~DefConfigConstraintValidatorExt() noexcept override = default;

  DefConfigConstraintValidatorExt(const DefConfigConstraintValidatorExt&) =
      delete;
  DefConfigConstraintValidatorExt(DefConfigConstraintValidatorExt&&) = delete;
  DefConfigConstraintValidatorExt& operator=(
      const DefConfigConstraintValidatorExt&) = delete;
  DefConfigConstraintValidatorExt& operator=(
      DefConfigConstraintValidatorExt&&) noexcept = delete;

  /// Allows sharing the default instance
  static constinit const DefConfigConstraintValidatorExt INST;

  /// Creates a new instance
  [[nodiscard]] static std::unique_ptr<const DefConfigConstraintValidatorExt>
  NEW_INST() noexcept;

  void check(const IConfigConstraint& /*cfg*/,
             const ent::AllEntities& /*allEnts*/) const noexcept final {}

  PRIVATE:
  DefConfigConstraintValidatorExt() noexcept = default;
};

inline constinit const DefConfigConstraintValidatorExt
    DefConfigConstraintValidatorExt::INST{};

/// Expresses a configuration for the raft(/bridge) / banks
class IConfigConstraint {
 public:
  virtual ~IConfigConstraint() noexcept = default;

  IConfigConstraint& operator=(const IConfigConstraint&) = delete;
  IConfigConstraint& operator=(IConfigConstraint&&) noexcept = delete;

  /**
  Checks the validity of this constraint using entities information,
  raft/bridge capacity and additional validation logic

  @throw logic_error for an invalid constraint
  */
  virtual void validate(const ent::AllEntities& allEnts,
                        unsigned capacity = UINT_MAX,
                        const IConfigConstraintValidatorExt& valExt =
                            DefConfigConstraintValidatorExt::INST) const = 0;

  /// @return a copy of this on heap
  [[nodiscard]] virtual std::unique_ptr<const IConfigConstraint> clone()
      const noexcept = 0;

  /// Is there a match between the provided collection and the constraint's
  /// data?
  virtual bool matches(const ent::IsolatedEntities& ents) const noexcept = 0;

  /// @return the length of the longest possible match
  virtual unsigned longestMatchLength() const noexcept { return UINT_MAX; }

  /// @return the length of the longest possible mismatch
  virtual unsigned longestMismatchLength() const noexcept { return UINT_MAX; }

  /// Describes the constraint
  virtual std::string toString() const = 0;

 protected:
  IConfigConstraint() noexcept = default;

  IConfigConstraint(const IConfigConstraint&) = default;
  IConfigConstraint(IConfigConstraint&&) noexcept = default;
};

/// Base class for Numeric and Logical expressions
template <typename Type>
class AbsExpr {
 public:
  virtual ~AbsExpr() noexcept = default;

  AbsExpr(const AbsExpr&) = delete;
  AbsExpr(AbsExpr&&) = delete;
  AbsExpr& operator=(const AbsExpr&) = delete;
  AbsExpr& operator=(AbsExpr&&) noexcept = delete;

  /// Provide the cached result of the expression when this is a constant
  const std::optional<Type>& constValue() const noexcept { return val; }

  /// Checks if there is a dependency on `varName`
  virtual bool dependsOnVariable(
      const std::string& /*varName*/) const noexcept {
    return false;
  }

  /**
    @param st the symbols table
    @return the expression evaluated using the provided symbols' values
    @throw out_of_range when referring symbols missing from the table
  */
  virtual Type eval(const SymbolsTable& st) const = 0;

  virtual std::string toString() const = 0;

 protected:
  AbsExpr(const std::optional<Type>& val_ = {}) noexcept : val{val_} {}

  /// Cached result of the expression if possible
  std::optional<Type> val;
};

/// Base of numeric expression types
class NumericExpr : public AbsExpr<double> {
 public:
  ~NumericExpr() noexcept override = default;

  NumericExpr(const NumericExpr&) = delete;
  NumericExpr(NumericExpr&&) = delete;
  NumericExpr& operator=(const NumericExpr&) = delete;
  NumericExpr& operator=(NumericExpr&&) noexcept = delete;

 protected:
  NumericExpr(const std::optional<double>& val_ = {}) noexcept
      : AbsExpr{val_} {}
};

/// Base of logical expression types
class LogicalExpr : public AbsExpr<bool> {
 public:
  ~LogicalExpr() noexcept override = default;

  LogicalExpr(const LogicalExpr&) = delete;
  LogicalExpr(LogicalExpr&&) = delete;
  LogicalExpr& operator=(const LogicalExpr&) = delete;
  LogicalExpr& operator=(LogicalExpr&&) noexcept = delete;

 protected:
  LogicalExpr(const std::optional<bool>& val_ = {}) noexcept : AbsExpr{val_} {}
};

/// A set of values (expressions that can be evaluated using a symbols table)
template <typename Type>
class IValues {
 public:
  virtual ~IValues() noexcept = default;

  IValues& operator=(const IValues&) = delete;
  IValues& operator=(IValues&&) noexcept = delete;

  virtual bool empty() const noexcept = 0;

  virtual bool constSet() const noexcept = 0;  ///< are the values all constant?

  /// Checks if there is a dependency on `varName`
  virtual bool dependsOnVariable(
      const std::string& /*varName*/) const noexcept {
    return false;
  }

  /// Checks if v is covered by current values and ranges based on the symbols'
  /// table
  virtual bool contains(const Type& v, const SymbolsTable& st = {}) const = 0;

  virtual std::string toString() const = 0;

 protected:
  IValues() noexcept = default;

  IValues(const IValues&) = default;
  IValues(IValues&&) noexcept = default;
};

}  // namespace rc::cond

namespace std {

inline auto& operator<<(auto& os, const rc::cond::IConfigConstraint& c) {
  os << c.toString();
  return os;
}

template <typename Type>
inline auto& operator<<(auto& os, const rc::cond::AbsExpr<Type>& e) {
  os << e.toString();
  return os;
}

template <typename Type>
inline auto& operator<<(auto& os, const rc::cond::IValues<Type>& vs) {
  os << vs.toString();
  return os;
}

}  // namespace std

#endif  // H_ABS_CONFIG_CONSTRAINT not defined
