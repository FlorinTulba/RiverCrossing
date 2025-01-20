/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_CONFIG_CONSTRAINT
#define H_CONFIG_CONSTRAINT

#include "configParser.h"
#include "util.h"

#include <iterator>
#include <unordered_set>
#include <variant>

#include <gsl/pointers>

#include <boost/logic/tribool.hpp>

namespace rc::cond {

// Forward declarations
class IdsConstraint;
class TypesConstraint;

/**
Base class for extending the validation of IConfigConstraint.
Some of the new virtual methods are abstract and must be implemented
by every derived class.
*/
class AbsConfigConstraintValidatorExt
    : public IConfigConstraintValidatorExt,

      // provides `selectExt` static method
      public DecoratorManager<AbsConfigConstraintValidatorExt,
                              IConfigConstraintValidatorExt> {
  // for accessing nextExt
  friend class DecoratorManager<AbsConfigConstraintValidatorExt,
                                IConfigConstraintValidatorExt>;

 public:
  ~AbsConfigConstraintValidatorExt() noexcept override;

  AbsConfigConstraintValidatorExt(const AbsConfigConstraintValidatorExt&) =
      delete;
  AbsConfigConstraintValidatorExt(AbsConfigConstraintValidatorExt&&) = delete;
  void operator=(const AbsConfigConstraintValidatorExt&) = delete;
  void operator=(AbsConfigConstraintValidatorExt&&) = delete;

  /// @throw logic_error if cfg does not respect all the extensions
  void check(const IConfigConstraint& cfg,
             const ent::AllEntities& allEnts) const final;

 protected:
  AbsConfigConstraintValidatorExt(
      std::unique_ptr<const IConfigConstraintValidatorExt> nextExt_) noexcept;

  /// @throw logic_error if the types configuration does not respect current
  /// extension
  virtual void checkTypesCfg(const TypesConstraint& /*cfg*/,
                             const ent::AllEntities& /*allEnts*/) const {}

  /// @throw logic_error if the ids configuration does not respect current
  /// extension
  virtual void checkIdsCfg(const IdsConstraint& /*cfg*/,
                           const ent::AllEntities& /*allEnts*/) const {}

  PROTECTED :

      gsl::not_null<gsl::owner<const IConfigConstraintValidatorExt*>>
          nextExt;
};

/**
  A collection of several configuration constraints within the context
  of the provided entities.

  The constraints should be either all enforced, or none of them is allowed
*/
class ConfigConstraints {
 public:
  /// The constraints should be either all enforced, or none of them is allowed
  /// @throw logic_error if any constraint is invalid
  ConfigConstraints(grammar::ConstraintsVec&& constraints_,
                    const ent::AllEntities& allEnts_,
                    bool allowed_ = true,
                    bool postponeValidation = false);
  virtual ~ConfigConstraints() noexcept = default;

  void operator=(const ConfigConstraints&) = delete;
  void operator=(ConfigConstraints&&) = delete;

  /// Are these constraints allowing certain configurations or disallowing them?
  [[nodiscard]] bool allowed() const noexcept;

  [[nodiscard]] bool empty() const noexcept;  ///< are there any constraints?

  /**
    For _allowed == true - are these entities
      matching at least 1 of the allowed configurations?
    For _allowed == false - are these entities
      violating all of the forbidden configurations?

    @param ents the entities to be checked against these ConfigConstraints

    @return as explained above
  */
  [[nodiscard]] virtual bool check(const ent::IsolatedEntities& ents) const;

  [[nodiscard]] std::string toString() const;

 protected:
  ConfigConstraints(const ConfigConstraints&) = default;
  ConfigConstraints(ConfigConstraints&&) noexcept = default;

  PROTECTED :

      /// Parsed constraints to be validated
      grammar::ConstraintsVec constraints;

  gsl::not_null<const ent::AllEntities*> allEnts;  ///< all known entities

  /// Are these constraints allowing certain configurations or disallowing them?
  bool _allowed;
};

/// Allows performing canRow, allowedLoads and other checks on raft/bridge
/// configurations
class IContextValidator {
 public:
  virtual ~IContextValidator() noexcept = default;

  IContextValidator(const IContextValidator&) = delete;
  IContextValidator(IContextValidator&&) = delete;
  void operator=(const IContextValidator&) = delete;
  void operator=(IContextValidator&&) = delete;

  /// @return true if `ents` is a valid raft/bridge configuration within `st`
  /// context
  /// @throw logic_error if ents misses some extension(s)
  [[nodiscard]] virtual bool validate(const ent::MovingEntities& ents,
                                      const SymbolsTable& st) const = 0;

 protected:
  IContextValidator() noexcept = default;
};

/// Neutral context validator - accepts any raft/bridge configuration
class DefContextValidator final : public IContextValidator {
 public:
  ~DefContextValidator() noexcept override = default;

  DefContextValidator(const DefContextValidator&) = delete;
  DefContextValidator(DefContextValidator&&) = delete;
  void operator=(const DefContextValidator&) = delete;
  void operator=(DefContextValidator&&) = delete;

  /// Allows sharing the default instance
  static const std::shared_ptr<const DefContextValidator>& SHARED_INST();

  [[nodiscard]] bool validate(const ent::MovingEntities&,
                              const SymbolsTable&) const noexcept final;

  PRIVATE :

      DefContextValidator() noexcept = default;
};

/**
Calling `IContextValidator::validate()` might throw.
Some valid contexts (expressed mainly by the Symbols Table)
allow this to happen.
In those cases there should actually be a validation result.

This interface allows handling those exceptions so that
whenever they occur, the validation will provide a result instead of throwing.

The rest of the exceptions will still propagate.

When an exception is caught, an instance of a derived class assesses the
context.
*/
class IValidatorExceptionHandler {
 public:
  virtual ~IValidatorExceptionHandler() noexcept = default;

  IValidatorExceptionHandler(const IValidatorExceptionHandler&) = delete;
  IValidatorExceptionHandler(IValidatorExceptionHandler&&) = delete;
  void operator=(const IValidatorExceptionHandler&) = delete;
  void operator=(IValidatorExceptionHandler&&) = delete;

  /**
  Assesses the context of the exception.
  If it doesn't match the exempted cases, returns `indeterminate`.
  If it matches the exempted cases generates a boolean validation result
  */
  [[nodiscard]] virtual boost::logic::tribool assess(
      const ent::MovingEntities& ents,
      const SymbolsTable& st) const noexcept = 0;

 protected:
  IValidatorExceptionHandler() noexcept = default;
};

/// Abstract base class for the context validator decorators.
class AbsContextValidator : public IContextValidator {
 public:
  ~AbsContextValidator() noexcept override = default;

  AbsContextValidator(const AbsContextValidator&) = delete;
  AbsContextValidator(AbsContextValidator&&) = delete;
  void operator=(const AbsContextValidator&) = delete;
  void operator=(AbsContextValidator&&) = delete;

  /**
  Performs local validation and then delegates to the next validator.
  The local validation might throw and the optional handler might
  stop the exception propagation and generate a validation result instead.

  @return true if `ents` is a valid raft/bridge configuration within `st`
  context
  @throw logic_error if ents misses some extension(s)
  */
  [[nodiscard]] bool validate(const ent::MovingEntities& ents,
                              const SymbolsTable& st) const final;

 protected:
  explicit AbsContextValidator(
      const std::shared_ptr<const IContextValidator>& nextValidator_,
      const std::shared_ptr<const IValidatorExceptionHandler>&
          ownValidatorExcHandler_ = {}) noexcept;

  /// @return true if `ents` is a valid raft/bridge configuration within `st`
  /// context
  /// @throw logic_error if ents misses some extension(s)
  [[nodiscard]] virtual bool doValidate(const ent::MovingEntities& ents,
                                        const SymbolsTable& st) const = 0;

  PROTECTED :

      // using shared_ptr as more configurations might use these fields

      /// A chained next validator
      gsl::not_null<std::shared_ptr<const IContextValidator>>
          nextValidator;

  /// Possible handler for particular contexts
  std::shared_ptr<const IValidatorExceptionHandler> ownValidatorExcHandler;
};

/// Interface for the extensions for transfer constraints
class ITransferConstraintsExt {
 public:
  virtual ~ITransferConstraintsExt() noexcept = default;

  ITransferConstraintsExt(const ITransferConstraintsExt&) = delete;
  ITransferConstraintsExt(ITransferConstraintsExt&&) = delete;
  void operator=(const ITransferConstraintsExt&) = delete;
  void operator=(ITransferConstraintsExt&&) = delete;

  /// @return validator extensions of a configuration
  [[nodiscard]] virtual std::unique_ptr<const IConfigConstraintValidatorExt>
  configValidatorExt() const = 0;

  /// @return true only if cfg satisfies these extensions
  [[nodiscard]] virtual bool check(const ent::MovingEntities& cfg) const = 0;

 protected:
  ITransferConstraintsExt() noexcept = default;
};

/// Neutral TransferConstraints extension
class DefTransferConstraintsExt final : public ITransferConstraintsExt {
 public:
  ~DefTransferConstraintsExt() noexcept override = default;

  DefTransferConstraintsExt(const DefTransferConstraintsExt&) = delete;
  DefTransferConstraintsExt(DefTransferConstraintsExt&&) = delete;
  void operator=(const DefTransferConstraintsExt&) = delete;
  void operator=(DefTransferConstraintsExt&&) = delete;

  /// Allows sharing the default instance
  static const DefTransferConstraintsExt& INST() noexcept;

  /// Getting a new instance
  [[nodiscard]] static std::unique_ptr<const DefTransferConstraintsExt>
  NEW_INST();

  /// @return new validator extensions of a configuration
  [[nodiscard]] std::unique_ptr<const IConfigConstraintValidatorExt>
  configValidatorExt() const final;

  /// @return true only if cfg satisfies these extensions
  [[nodiscard]] bool check(const ent::MovingEntities&) const noexcept final {
    return true;
  }

  PRIVATE :

      DefTransferConstraintsExt() noexcept = default;
};

/**
Base class for handling transfer constraints extensions.
Some of the new virtual methods are abstract and must be implemented
by every derived class.
*/
class AbsTransferConstraintsExt
    : public ITransferConstraintsExt,

      // provides `selectExt` static
      // method
      public DecoratorManager<AbsTransferConstraintsExt,
                              ITransferConstraintsExt> {
  // for accessing nextExt
  friend class DecoratorManager<AbsTransferConstraintsExt,
                                ITransferConstraintsExt>;

 public:
  ~AbsTransferConstraintsExt() noexcept override;

  AbsTransferConstraintsExt(const AbsTransferConstraintsExt&) = delete;
  AbsTransferConstraintsExt(AbsTransferConstraintsExt&&) = delete;
  void operator=(const AbsTransferConstraintsExt&) = delete;
  void operator=(AbsTransferConstraintsExt&&) = delete;

  /// @return validator extensions of a configuration
  [[nodiscard]] std::unique_ptr<const IConfigConstraintValidatorExt>
  configValidatorExt() const noexcept final;

  /// @return true only if cfg satisfies these extensions
  [[nodiscard]] bool check(const ent::MovingEntities& cfg) const final;

 protected:
  AbsTransferConstraintsExt(
      std::unique_ptr<const ITransferConstraintsExt> nextExt_);

  /// @return validator extensions of a configuration
  [[nodiscard]] virtual std::unique_ptr<const IConfigConstraintValidatorExt>
  _configValidatorExt(std::unique_ptr<const IConfigConstraintValidatorExt>
                          fromNextExt) const noexcept = 0;

  /// @return true only if cfg satisfies current extension
  /// @throw logic_error for invalid extension
  [[nodiscard]] virtual bool _check(const ent::MovingEntities&) const {
    return true;
  }

  PROTECTED :

      gsl::not_null<gsl::owner<const ITransferConstraintsExt*>>
          nextExt;
};

/// ConfigConstraints for raft/bridge configurations
class TransferConstraints : public ConfigConstraints {
 public:
  /// @throw logic_error for an invalid constraint
  TransferConstraints(grammar::ConstraintsVec&& constraints_,
                      const ent::AllEntities& allEnts_,
                      const unsigned& capacity_,
                      bool allowed_ = true,
                      const ITransferConstraintsExt& extension_ =
                          DefTransferConstraintsExt::INST());
  TransferConstraints(const TransferConstraints&) = default;
  TransferConstraints(TransferConstraints&&) noexcept = default;
  ~TransferConstraints() noexcept override = default;

  void operator=(const TransferConstraints&) = delete;
  void operator=(TransferConstraints&&) = delete;

  /**
    For _allowed == true - are these entities respecting
      first the capacity and extension conditions
      and then matching at least 1 of the allowed configurations?
    For _allowed == false - are these entities respecting
      the capacity and extension conditions,
      but violating all of the forbidden configurations?

    @param ents the entities to be checked against these ConfigConstraints

    @return as explained above
  */
  [[nodiscard]] bool check(const ent::IsolatedEntities& ents) const override;

  /// @return the minimal capacity suitable for these constraints
  [[nodiscard]] unsigned minRequiredCapacity() const noexcept;

  PROTECTED :

      gsl::not_null<const ITransferConstraintsExt*>
          extension;

  /// How many entities are allowed on the raft/bridge at once
  gsl::not_null<const unsigned*> capacity;
};

/// Valid configurations of same duration
class ConfigurationsTransferDuration {
 public:
  /// @throw logic_error for an invalid constraint
  ConfigurationsTransferDuration(
      grammar::ConfigurationsTransferDurationInitType&& initType,
      const ent::AllEntities& allEnts_,
      const unsigned& capacity,
      const ITransferConstraintsExt& extension_ =
          DefTransferConstraintsExt::INST());
  ConfigurationsTransferDuration(const ConfigurationsTransferDuration&) =
      default;
  ConfigurationsTransferDuration(ConfigurationsTransferDuration&&) noexcept =
      default;
  ~ConfigurationsTransferDuration() noexcept = default;

  void operator=(const ConfigurationsTransferDuration&) = delete;
  void operator=(ConfigurationsTransferDuration&&) = delete;

  /// All configurations with the given duration
  [[nodiscard]] const TransferConstraints& configConstraints() const noexcept;

  /// Traversal duration for those configurations
  [[nodiscard]] unsigned duration() const noexcept;

  [[nodiscard]] std::string toString() const;

  PROTECTED :

      /// All configurations with the given duration
      TransferConstraints constraints;
  unsigned _duration{};  ///< traversal duration for those configurations
};

/// The provided constraint uses entity types
class TypesConstraint : public IConfigConstraint {
 public:
  TypesConstraint() noexcept = default;
  TypesConstraint(const TypesConstraint&) = default;
  TypesConstraint(TypesConstraint&&) noexcept = default;
  ~TypesConstraint() noexcept override = default;

  void operator=(const TypesConstraint&) = delete;
  void operator=(TypesConstraint&&) = delete;

  /**
    Expands the types constraint with a range for a new type.

    @return *this
    @throw logic_error for duplicate types or wrong range limits
  */
  TypesConstraint& addTypeRange(const std::string& newType,
                                unsigned minIncl = 0U,
                                unsigned maxIncl = UINT_MAX);
  /// @return a copy of this on heap
  [[nodiscard]] std::unique_ptr<const IConfigConstraint> clone()
      const noexcept override;

  /**
  Checks the validity of this constraint using entities information,
  raft/bridge capacity and additional validation logic

  @throw logic_error for an invalid constraint
  */
  void validate(const ent::AllEntities& allEnts,
                unsigned capacity = UINT_MAX,
                const IConfigConstraintValidatorExt& valExt =
                    DefConfigConstraintValidatorExt::INST()) const override;

  /// Is there a match between the provided collection and the constraint's
  /// data?
  [[nodiscard]] bool matches(
      const ent::IsolatedEntities& ents) const noexcept override;

  /// @return the length of the longest possible match
  [[nodiscard]] unsigned longestMatchLength() const noexcept override;

  [[nodiscard]] const std::unordered_map<std::string,
                                         std::pair<unsigned, unsigned>>&
  mandatoryTypeNames() const noexcept {
    return mandatoryTypes;
  }

  [[nodiscard]] std::string toString() const override;

  PROTECTED :

      /// All the mentioned types
      std::unordered_set<std::string>
          mentionedTypes;

  /// Map between expected types and the range for the count of each such type.
  /// The range comes as a pair of 2 unsigned values: min and max inclusive
  std::unordered_map<std::string, std::pair<unsigned, unsigned>> mandatoryTypes;

  /// Map between optional types and their max inclusive count
  std::unordered_map<std::string, unsigned> optionalTypes;

  unsigned _longestMatchLength{};  ///< length of the longest possible match
};

/// The provided constraint uses entity ids
class IdsConstraint : public IConfigConstraint {
 public:
  IdsConstraint() noexcept = default;
  IdsConstraint(const IdsConstraint&) = default;
  IdsConstraint(IdsConstraint&&) noexcept = default;

  ~IdsConstraint() noexcept override = default;

  void operator=(const IdsConstraint&) = delete;
  void operator=(IdsConstraint&&) = delete;

  /// Creates a new mandatory group with this id
  IdsConstraint& addMandatoryId(unsigned id);

  /// Creates a new mandatory group with the ids from the provided container
  template <class Cont>
  IdsConstraint& addMandatoryGroup(const Cont& group) {
    static_assert(std::is_same<unsigned, typename Cont::value_type>::value);

    std::unordered_set<unsigned> newMentionedIds{mentionedIds};
    newMentionedIds.insert(CBOUNDS(group));
    if (std::size(mentionedIds) + std::size(group) > std::size(newMentionedIds))
      throw std::domain_error{
          HERE.function_name() +
          " - Found id(s) mentioned earlier in the same Ids constraint!"s};
    mentionedIds = newMentionedIds;

    mandatoryGroups.emplace_back(CBOUNDS(group));

    if (_longestMatchLength != UINT_MAX)
      ++_longestMatchLength;

    return *this;
  }

  /// Creates a new optional group with this id
  IdsConstraint& addOptionalId(unsigned id);

  /// Creates a new optional group with the ids from the provided container
  template <class Cont>
  IdsConstraint& addOptionalGroup(const Cont& group) {
    static_assert(std::is_same<unsigned, typename Cont::value_type>::value);

    std::unordered_set<unsigned> newMentionedIds{mentionedIds};
    newMentionedIds.insert(CBOUNDS(group));
    if (std::size(mentionedIds) + std::size(group) > std::size(newMentionedIds))
      throw std::domain_error{
          HERE.function_name() +
          " - Found id(s) mentioned earlier in the same Ids constraint!"s};
    mentionedIds = newMentionedIds;

    optionalGroups.emplace_back(CBOUNDS(group));

    if (_longestMatchLength != UINT_MAX)
      ++_longestMatchLength;

    return *this;
  }

  IdsConstraint& addAvoidedId(unsigned id);  ///< new id to avoid

  /// Increments the count of mandatory ids
  IdsConstraint& addUnspecifiedMandatory() noexcept;

  /// Allows more entities apart from mandatory, optional and avoided ones
  IdsConstraint& setUnbounded() noexcept;

  /// @return a copy of this on heap
  [[nodiscard]] std::unique_ptr<const IConfigConstraint> clone()
      const noexcept override;

  /**
  Checks the validity of this constraint using entities information,
  raft/bridge capacity and additional validation logic

  @throw logic_error for an invalid constraint
  */
  void validate(const ent::AllEntities& allEnts,
                unsigned capacity = UINT_MAX,
                const IConfigConstraintValidatorExt& valExt =
                    DefConfigConstraintValidatorExt::INST()) const override;

  /// Is there a match between the provided collection and the constraint's
  /// data?
  [[nodiscard]] bool matches(
      const ent::IsolatedEntities& ents) const noexcept override;

  /// @return the length of the longest possible match
  [[nodiscard]] unsigned longestMatchLength() const noexcept override;

  /// @return the length of the longest possible mismatch
  [[nodiscard]] unsigned longestMismatchLength() const noexcept override;

  [[nodiscard]] std::string toString() const override;

  PROTECTED :

      /// All id-s mentioned by the constraint
      std::unordered_set<unsigned>
          mentionedIds;

  /// Set of mandatory groups
  std::vector<std::unordered_set<unsigned>> mandatoryGroups;

  /// Set of optional groups
  std::vector<std::unordered_set<unsigned>> optionalGroups;

  std::unordered_set<unsigned> avoidedIds;  ///< set of entity ids to avoid

  /// Count of additional mandatory entities (with ids not covered by the 3
  /// sets)
  unsigned expectedExtraIds{};

  unsigned _longestMatchLength{};  ///< length of the longest possible match

  /**
    Prevents (when true) matching configurations with other ids,
    apart from mandatory, optional and avoided.

    On both situations, the configuration's length will be at least
    size of mandatory + expectedExtraIds.

    But when true, the configuration's length will be at most size of mandatory
    + expectedExtraIds + size of optional.
  */
  bool capacityLimit{true};
};

/// bool constants: true or false
class BoolConst : public LogicalExpr {
 public:
  explicit BoolConst(bool b) noexcept;
  ~BoolConst() noexcept override = default;

  BoolConst(const BoolConst&) = delete;
  BoolConst(BoolConst&&) = delete;
  void operator=(const BoolConst&) = delete;
  void operator=(BoolConst&&) = delete;

  /// @return the contained bool constant
  [[nodiscard]] bool eval(const SymbolsTable&) const noexcept override;

  [[nodiscard]] std::string toString() const override;
};

/// Negation of a logical expression
class Not : public LogicalExpr {
 public:
  explicit Not(const std::shared_ptr<const LogicalExpr>& le);
  ~Not() noexcept override = default;

  Not(const Not&) = delete;
  Not(Not&&) = delete;
  void operator=(const Not&) = delete;
  void operator=(Not&&) = delete;

  /// Checks if there is a dependency on `varName`
  [[nodiscard]] bool dependsOnVariable(
      const std::string& varName) const noexcept override;

  [[nodiscard]] bool eval(const SymbolsTable& st) const override;

  [[nodiscard]] std::string toString() const override;

  PROTECTED :

      /// The expression to negate
      gsl::not_null<std::shared_ptr<const LogicalExpr>>
          _le;
};

/// Value of a numeric expression or a range provided as 2 numeric expressions
class ValueOrRange {
 public:
  using ValueType = std::shared_ptr<const NumericExpr>;
  using RangeType = std::pair<ValueType, ValueType>;

  /// Ctor for the value of a numeric expression
  explicit ValueOrRange(const ValueType& value_);

  /// Ctor for a range provided as 2 numeric expressions
  ValueOrRange(const ValueType& from_, const ValueType& to_);

  ValueOrRange(const ValueOrRange& other) noexcept;
  ValueOrRange(ValueOrRange&& other) noexcept;

  ~ValueOrRange() noexcept;

  void operator=(const ValueOrRange& other) = delete;
  void operator=(ValueOrRange&& other) = delete;

  /// Checks if there is a dependency on `varName`
  [[nodiscard]] bool dependsOnVariable(
      const std::string& varName) const noexcept;

  /// @return true if the value / range contains only constants
  [[nodiscard]] bool isConst() const noexcept;

  /// @return true for range; false for value
  [[nodiscard]] bool isRange() const noexcept;

  /**
    @param st the symbols' table
    @return the value based on the data from the symbols' table
    @throw logic_error for ranges or for NaN values
    @throw out_of_range when referring symbols missing from the table
  */
  [[nodiscard]] double value(const SymbolsTable& st) const;

  /**
    @param st the symbols' table
    @return the range based on the data from the symbols' table
    @throw logic_error for non-ranges, for NaN values and for out of order
    values
    @throw out_of_range when referring symbols missing from the table
  */
  [[nodiscard]] std::pair<double, double> range(const SymbolsTable& st) const;

  [[nodiscard]] std::string toString() const;

 private:
  /// @throw logic_error for NaN values
  static void validateDouble(double d);

  /// @throw logic_error for out of order values
  static void validateRange(double a, double b);

  /// @throw logic_error for NULL / NaN values or for out of order values
  void validate() const;

  PROTECTED :

      std::variant<ValueType, RangeType>
          _valueOrRange;
};

/// A mixture of values and ranges
class ValueSet : public IValues<double> {
 public:
  ValueSet() noexcept = default;
  ValueSet(const ValueSet&) = default;
  ValueSet(ValueSet&&) noexcept = default;
  ~ValueSet() noexcept override = default;

  void operator=(const ValueSet&) = delete;
  void operator=(ValueSet&&) = delete;

  /// Appends a value / range; Returns *this
  ValueSet& add(const ValueOrRange& vor) noexcept;

  [[nodiscard]] bool empty() const noexcept override;

  /// Checks if there is a dependency on `varName`
  [[nodiscard]] bool dependsOnVariable(
      const std::string& varName) const noexcept override;

  [[nodiscard]] bool constSet()
      const noexcept override;  ///< are the values all constant?

  /// Checks if v is covered by current values and ranges based on the symbols'
  /// table
  [[nodiscard]] bool contains(const double& v,
                              const SymbolsTable& st = {}) const override;

  [[nodiscard]] std::string toString() const override;

  PROTECTED :

      /// The values and the ranges
      std::vector<ValueOrRange>
          vors;

  bool isConst{true};  ///< true if the values / ranges are all constant
};

/// Checks if an expression is covered by a set of values
template <typename Type>
class BelongToCondition : public LogicalExpr {
 public:
  BelongToCondition(const std::shared_ptr<const AbsExpr<Type>>& e,
                    const std::shared_ptr<const IValues<Type>>& valueSet_)
      : _e{e}, _valueSet{valueSet_} {
    if (_e->constValue() && _valueSet->constSet())
      val = _valueSet->contains(*_e->constValue());
  }
  ~BelongToCondition() noexcept override = default;

  BelongToCondition(const BelongToCondition&) = delete;
  BelongToCondition(BelongToCondition&&) = delete;
  void operator=(const BelongToCondition&) = delete;
  void operator=(BelongToCondition&&) = delete;

  /// Checks if there is a dependency on `varName`
  [[nodiscard]] bool dependsOnVariable(
      const std::string& varName) const noexcept override {
    return _e->dependsOnVariable(varName) ||
           _valueSet->dependsOnVariable(varName);
  }

  /// Performs the membership test
  [[nodiscard]] bool eval(const SymbolsTable& st) const override {
    if (val)
      return *val;

    return _valueSet->contains(_e->eval(st), st);
  }

  [[nodiscard]] std::string toString() const override {
    std::ostringstream oss;
    oss << *_e << " in " << *_valueSet;
    return oss.str();
  }

  PROTECTED :

      /// Expression to be checked
      gsl::not_null<std::shared_ptr<const AbsExpr<Type>>>
          _e;

  /// The set of values
  gsl::not_null<std::shared_ptr<const IValues<Type>>> _valueSet;
};

/// A number
class NumericConst : public NumericExpr {
 public:
  explicit NumericConst(double d) noexcept;
  ~NumericConst() noexcept override = default;

  NumericConst(const NumericConst&) = delete;
  NumericConst(NumericConst&&) = delete;
  void operator=(const NumericConst&) = delete;
  void operator=(NumericConst&&) = delete;

  /// @return the contained constant
  [[nodiscard]] double eval(const SymbolsTable&) const noexcept override;

  [[nodiscard]] std::string toString() const override;
};

/// The name of a variable
class NumericVariable : public NumericExpr {
 public:
  explicit NumericVariable(const std::string& varName) noexcept;
  ~NumericVariable() noexcept override = default;

  NumericVariable(const NumericVariable&) = delete;
  NumericVariable(NumericVariable&&) = delete;
  void operator=(const NumericVariable&) = delete;
  void operator=(NumericVariable&&) = delete;

  /// Checks if there is a dependency on `varName`
  [[nodiscard]] bool dependsOnVariable(
      const std::string& varName) const noexcept override;

  /// @throw out_of_range when name is missing from the symbols table
  [[nodiscard]] double eval(const SymbolsTable& st) const override;

  [[nodiscard]] std::string toString() const noexcept override;

  PROTECTED :

      /// The considered name
      std::string name;
};

/// Adding 2 numeric expressions
class Addition : public NumericExpr {
 public:
  Addition(const std::shared_ptr<const NumericExpr>& left_,
           const std::shared_ptr<const NumericExpr>& right_);
  ~Addition() noexcept override = default;

  Addition(const Addition&) = delete;
  Addition(Addition&&) = delete;
  void operator=(const Addition&) = delete;
  void operator=(Addition&&) = delete;

  /// Checks if there is a dependency on `varName`
  [[nodiscard]] bool dependsOnVariable(
      const std::string& varName) const noexcept override;

  /// @throw out_of_range whenever a variable isn't found
  [[nodiscard]] double eval(const SymbolsTable& st) const override;

  [[nodiscard]] std::string toString() const override;

  PROTECTED :

      gsl::not_null<std::shared_ptr<const NumericExpr>>
          left;
  gsl::not_null<std::shared_ptr<const NumericExpr>> right;
};

/// Modulus of 2 numeric expressions
class Modulus : public NumericExpr {
 public:
  /**
  @throw logic_error when both values are 0 or one is non-integer
  @throw overflow_error when only the denominator is 0
  */
  Modulus(const std::shared_ptr<const NumericExpr>& numerator_,
          const std::shared_ptr<const NumericExpr>& denominator_);
  ~Modulus() noexcept override = default;

  Modulus(const Modulus&) = delete;
  Modulus(Modulus&&) = delete;
  void operator=(const Modulus&) = delete;
  void operator=(Modulus&&) = delete;

  /// Checks if there is a dependency on `varName`
  [[nodiscard]] bool dependsOnVariable(
      const std::string& varName) const noexcept override;

  /**
  @throw out_of_range whenever a variable isn't found
  @throw logic_error when both values are 0 or one is non-integer
  @throw overflow_error when only the denominator is 0
  */
  [[nodiscard]] double eval(const SymbolsTable& st) const override;

  [[nodiscard]] std::string toString() const override;

 protected:
  /**
  @throw logic_error for non-integer values
  @return the corresponding integer value
  */
  [[nodiscard]] static long validLong(double v);

  /**
  @throw logic_error when both values are 0
  @throw overflow_error when only the denominator is 0
  @return modulus result
  */
  [[nodiscard]] static long validOperation(long numeratorL, long denominatorL);

  PROTECTED :

      gsl::not_null<std::shared_ptr<const NumericExpr>>
          numerator;
  gsl::not_null<std::shared_ptr<const NumericExpr>> denominator;
};

}  // namespace rc::cond

namespace std {

std::ostream& operator<<(std::ostream& os,
                         const rc::cond::ConfigConstraints& c);

std::ostream& operator<<(std::ostream& os,
                         const rc::cond::ConfigurationsTransferDuration& ctd);

std::ostream& operator<<(std::ostream& os, const rc::cond::ValueOrRange& vor);

}  // namespace std

#endif  // H_CONFIG_CONSTRAINT not defined
