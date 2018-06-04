/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_CONFIG_CONSTRAINT
#define H_CONFIG_CONSTRAINT

#include "absConfigConstraint.h"
#include "configParser.h"
#include "util.h"

#include <sstream>
#include <unordered_set>
#include <iterator>

namespace rc {
namespace cond {

/**
  A collection of several configuration constraints within the context
  of the provided entities.

  The constraints should be either all enforced, or none of them is allowed
*/
class ConfigConstraints {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  grammar::ConstraintsVec constraints; ///< parsed constraints to be validated

  std::shared_ptr<const ent::AllEntities> allEnts; ///< all known entities

  /// Are these constraints allowing certain configurations or disallowing them?
  const bool _allowed;

public:
  /// The constraints should be either all enforced, or none of them is allowed
  /// @throw logic_error if any constraint is invalid
  ConfigConstraints(grammar::ConstraintsVec &&constraints_,
                    const std::shared_ptr<const ent::AllEntities> &allEnts_,
                    bool allowed_ = true, bool postponeValidation = false);

  /// Are these constraints allowing certain configurations or disallowing them?
  bool allowed() const;

  bool empty() const; ///< are there any constraints?

  /**
    For _allowed == true - are these entities
      matching at least 1 of the allowed configurations?
    For _allowed == false - are these entities
      violating all of the forbidden configurations?

    @param ents the entities to be checked against these ConfigConstraints

    @return as explained above
  */
  virtual bool check(const ent::IsolatedEntities &ents) const;

  std::string toString() const;
};

/// ConfigConstraints for raft/bridge configurations
class TransferConstraints : public ConfigConstraints {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  const double &_maxLoad;  ///< Overall max load of the raft/bridge

  /// How many entities are allowed on the raft/bridge at once
  const unsigned &_capacity;

public:
  /// @throw logic_error for an invalid constraint
  TransferConstraints(grammar::ConstraintsVec &&constraints_,
                      const std::shared_ptr<const ent::AllEntities> &allEnts_,
                      const unsigned &capacity, const double &maxLoad,
                      bool allowed_ = true);

  /**
    For _allowed == true - are these entities respecting
      first the capacity and max load conditions
      and then matching at least 1 of the allowed configurations?
    For _allowed == false - are these entities respecting
      the capacity and max load conditions,
      but violating all of the forbidden configurations?

    @param ents the entities to be checked against these ConfigConstraints

    @return as explained above
  */
  bool check(const ent::IsolatedEntities &ents) const override;

  /// @return the minimal capacity suitable for these constraints
  unsigned minRequiredCapacity() const;
};

/// Valid configurations of same duration
class ConfigurationsTransferDuration {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  TransferConstraints constraints;  ///< all configurations with the given duration
  unsigned _duration = 0U;          ///< traversal duration for those configurations

public:
  /// @throw logic_error for an invalid constraint
  ConfigurationsTransferDuration(
      grammar::ConfigurationsTransferDurationInitType &&initType,
      const std::shared_ptr<const ent::AllEntities> &allEnts_,
      const unsigned &capacity, const double &maxLoad);

  /// all configurations with the given duration
  const TransferConstraints& configConstraints() const;

  unsigned duration() const; ///< traversal duration for those configurations

  std::string toString() const;
};

/// The provided constraint uses entity types
class TypesConstraint : public IConfigConstraint {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  /// All the mentioned types
  std::unordered_set<std::string> mentionedTypes;

  /// Map between expected types and the range for the count of each such type.
  /// The range comes as a pair of 2 unsigned values: min and max inclusive
  std::unordered_map<std::string, std::pair<unsigned, unsigned>> mandatoryTypes;

  /// Map between optional types and their max inclusive count
  std::unordered_map<std::string, unsigned> optionalTypes;

  unsigned _longestMatchLength = 0U; ///< length of the longest possible match

public:
  /**
    Expands the types constraint with a range for a new type.

    @return *this
    @throw logic_error for duplicate types or wrong range limits
  */
  TypesConstraint& addTypeRange(const std::string &newType,
                                unsigned minIncl = 0U,
                                unsigned maxIncl = UINT_MAX);

  /// @return a copy of this on heap
  std::unique_ptr<const IConfigConstraint> clone() const override;

  /**
    Checks the validity of this constraint using entities information
    @throw logic_error for an invalid constraint
  */
  void validate(const std::shared_ptr<const ent::AllEntities> &allEnts,
                unsigned capacity = UINT_MAX,
                double maxLoad = DBL_MAX) const override;

  /// Is there a match between the provided collection and the constraint's data?
  bool matches(const ent::IsolatedEntities &ents) const override;

  /// @return the length of the longest possible match
  unsigned longestMatchLength() const override;

  std::string toString() const override;
};

/// The provided constraint uses entity ids
class IdsConstraint : public IConfigConstraint {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  std::unordered_set<unsigned> mentionedIds; ///< all id-s mentioned by the constraint

  /// Set of mandatory groups
  std::vector<std::unordered_set<unsigned>> mandatoryGroups;

  /// Set of optional groups
  std::vector<std::unordered_set<unsigned>> optionalGroups;

  std::unordered_set<unsigned> avoidedIds;    ///< set of entity ids to avoid

  /// Count of additional mandatory entities (with ids not covered by the 3 sets)
  unsigned expectedExtraIds = 0U;

  unsigned _longestMatchLength = 0U; ///< length of the longest possible match

  /**
    Prevents (when true) matching configurations with other ids,
    apart from mandatory, optional and avoided.

    On both situations, the configuration's length will be at least
    size of mandatory + expectedExtraIds.

    But when true, the configuration's length will be at most size of mandatory +
    expectedExtraIds + size of optional.
  */
  bool capacityLimit = true;

public:
  /// Creates a new mandatory group with this id
  IdsConstraint& addMandatoryId(unsigned id);

  /// Creates a new mandatory group with the ids from the provided container
  template<class Cont>
  IdsConstraint& addMandatoryGroup(const Cont &group) {
    static_assert(std::is_same<unsigned, typename Cont::value_type>::value);
    std::unordered_set<unsigned> newMentionedIds(mentionedIds);
    newMentionedIds.insert(CBOUNDS(group));
    if(mentionedIds.size() + (size_t)std::distance(CBOUNDS(group)) >
        newMentionedIds.size())
      throw std::domain_error(std::string(__func__) +
        " - Found id(s) mentioned earlier in the same Ids constraint!");
    mentionedIds = newMentionedIds;

    mandatoryGroups.emplace_back(CBOUNDS(group));

    if(_longestMatchLength != UINT_MAX)
      ++_longestMatchLength;

    return *this;
  }

  /// Creates a new optional group with this id
  IdsConstraint& addOptionalId(unsigned id);

  /// Creates a new optional group with the ids from the provided container
  template<class Cont>
  IdsConstraint& addOptionalGroup(const Cont &group) {
    static_assert(std::is_same<unsigned, typename Cont::value_type>::value);
    std::unordered_set<unsigned> newMentionedIds(mentionedIds);
    newMentionedIds.insert(CBOUNDS(group));
    if(mentionedIds.size() + (size_t)std::distance(CBOUNDS(group)) >
        newMentionedIds.size())
      throw std::domain_error(std::string(__func__) +
        " - Found id(s) mentioned earlier in the same Ids constraint!");
    mentionedIds = newMentionedIds;

    optionalGroups.emplace_back(CBOUNDS(group));

    if(_longestMatchLength != UINT_MAX)
      ++_longestMatchLength;

    return *this;
  }

  IdsConstraint& addAvoidedId(unsigned id); ///< new id to avoid

  /// Increments the count of mandatory ids
  IdsConstraint& addUnspecifiedMandatory();

  /// Allows more entities apart from mandatory, optional and avoided ones
  IdsConstraint& setUnbounded();

  /// @return a copy of this on heap
  std::unique_ptr<const IConfigConstraint> clone() const override;

  /**
    Checks the validity of this constraint using entities information
    @throw logic_error for an invalid constraint
  */
  void validate(const std::shared_ptr<const ent::AllEntities> &allEnts,
                unsigned capacity = UINT_MAX,
                double maxLoad = DBL_MAX) const override;

  /// Is there a match between the provided collection and the constraint's data?
  bool matches(const ent::IsolatedEntities &ents) const override;

  /// @return the length of the longest possible match
  unsigned longestMatchLength() const override;

  /// @return the length of the longest possible mismatch
  unsigned longestMismatchLength() const override;

  std::string toString() const override;
};

/// bool constants: true or false
struct BoolConst : LogicalExpr {
  BoolConst(bool b);

  /// @return the contained bool constant
  bool eval(const SymbolsTable &) const override;

  std::string toString() const override;
};

/// Negation of a logical expression
class Not : public LogicalExpr {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif
  std::shared_ptr<const LogicalExpr> _le; ///< the expression to negate

public:
  Not(const std::shared_ptr<const LogicalExpr> &le);

  /// Checks if there is a dependency on `varName`
  bool dependsOnVariable(const std::string &varName) const override;

  bool eval(const SymbolsTable &st) const override;

  std::string toString() const override;
};

/// Value of a numeric expression or a range provided as 2 numeric expressions
class ValueOrRange {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  union {
    std::shared_ptr<const NumericExpr> _value;  ///< either the value
    std::shared_ptr<const NumericExpr> _from;   ///< or the inferior range limit
  };
  std::shared_ptr<const NumericExpr> _to; ///< the superior range limit (for a range)

  /// @throw logic_error for NaN values
  static void validateDouble(double d);

  /// @throw logic_error for out of order values
  static void validateRange(double a, double b);

  /// @throw logic_error for NULL / NaN values or for out of order values
  void validate() const;

public:
  ~ValueOrRange();

  /// Ctor for the value of a numeric expression
  ValueOrRange(const std::shared_ptr<const NumericExpr> &value_);

  /// Ctor for a range provided as 2 numeric expressions
  ValueOrRange(const std::shared_ptr<const NumericExpr> &from_,
      const std::shared_ptr<const NumericExpr> &to_);
  ValueOrRange(const ValueOrRange &other);

  ValueOrRange(ValueOrRange&&) = delete;
  void operator=(const ValueOrRange &other) = delete;
  void operator=(ValueOrRange &&other) = delete;

  /// Checks if there is a dependency on `varName`
  bool dependsOnVariable(const std::string &varName) const;

  /// @return true if the value / range contains only constants
  bool isConst() const;

  /// @return true for range; false for value
  bool isRange() const;

  /**
    @param st the symbols' table
    @return the value based on the data from the symbols' table
    @throw logic_error for ranges or for NaN values
    @throw out_of_range when referring symbols missing from the table
  */
  double value(const SymbolsTable &st) const;

  /**
    @param st the symbols' table
    @return the range based on the data from the symbols' table
    @throw logic_error for non-ranges, for NaN values and for out of order values
    @throw out_of_range when referring symbols missing from the table
  */
  std::pair<double, double> range(const SymbolsTable &st) const;

  std::string toString() const;
};

/// A mixture of values and ranges
class ValueSet : public IValues<double> {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  std::vector<ValueOrRange> vors; ///< the values and the ranges

  bool isConst = true; ///< true if the values / ranges are all constant

public:
  /// Appends a value / range; Returns *this
  ValueSet& add(const ValueOrRange &vor);

  bool empty() const override;

  /// Checks if there is a dependency on `varName`
  bool dependsOnVariable(const std::string &varName) const override;

  bool constSet() const override; ///< are the values all constant?

  /// Checks if v is covered by current values and ranges based on the symbols' table
  bool contains(const double &v, const SymbolsTable &st = {}) const override;

  std::string toString() const override;
};

/// Checks if an expression is covered by a set of values
template<typename Type>
class BelongToCondition : public LogicalExpr {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  std::shared_ptr<const AbsExpr<Type>> _e; ///< expression to be checked
  std::shared_ptr<const IValues<Type>> _valueSet; ///< the set of values

public:
  BelongToCondition(const std::shared_ptr<const AbsExpr<Type>> &e,
                    const std::shared_ptr<const IValues<Type>> &valueSet) :
      _e(VP(e)), _valueSet(VP(valueSet)) {
    if(_e->constValue() && _valueSet->constSet())
      val = _valueSet->contains(*_e->constValue());
  }

  /// Checks if there is a dependency on `varName`
  bool dependsOnVariable(const std::string &varName) const override {
    return _e->dependsOnVariable(varName) ||
      _valueSet->dependsOnVariable(varName);
  }

  /// Performs the membership test
  bool eval(const SymbolsTable &st) const override {
    if(val)
      return *val;

    return _valueSet->contains(_e->eval(st), st);
  }

  std::string toString() const override {
    std::ostringstream oss;
    oss<<*_e<<" in "<<*_valueSet;
    return oss.str();
  }
};

/// A number
struct NumericConst : NumericExpr {
  NumericConst(double d);

  double eval(const SymbolsTable &) const override; ///< @return the contained constant

  std::string toString() const override;
};

/// The name of a variable
class NumericVariable : public NumericExpr {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  std::string name; ///< the considered name

public:
  NumericVariable(const std::string &varName);

  /// Checks if there is a dependency on `varName`
  bool dependsOnVariable(const std::string &varName) const override;

  /// @throw out_of_range when name is missing from the symbols table
  double eval(const SymbolsTable &st) const override;

  std::string toString() const override;
};

/// Adding 2 numeric expressions
class Addition : public NumericExpr {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  std::shared_ptr<const NumericExpr> left, right;

public:
  /// @throw invalid_argument for NULL arguments
  Addition(const std::shared_ptr<const NumericExpr> &left_,
           const std::shared_ptr<const NumericExpr> &right_);

  /// Checks if there is a dependency on `varName`
  bool dependsOnVariable(const std::string &varName) const override;

  /// @throw out_of_range whenever a variable isn't found
  double eval(const SymbolsTable &st) const override;

  std::string toString() const override;
};

/// Modulus of 2 numeric expressions
class Modulus : public NumericExpr {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  std::shared_ptr<const NumericExpr> numerator, denominator;

  /**
  @throw logic_error for non-integer values
  @return the corresponding integer value
  */
  static long validLong(double v);

  /**
  @throw logic_error when both values are 0
  @throw overflow_error when only the denominator is 0
  @return modulus result
  */
  static long validOperation(long numeratorL, long denominatorL);

public:
  /**
  @throw invalid_argument for NULL arguments
  @throw logic_error when both values are 0 or one is non-integer
  @throw overflow_error when only the denominator is 0
  */
  Modulus(const std::shared_ptr<const NumericExpr> &numerator_,
          const std::shared_ptr<const NumericExpr> &denominator_);

  /// Checks if there is a dependency on `varName`
  bool dependsOnVariable(const std::string &varName) const override;

  /**
  @throw out_of_range whenever a variable isn't found
  @throw logic_error when both values are 0 or one is non-integer
  @throw overflow_error when only the denominator is 0
  */
  double eval(const SymbolsTable &st) const override;

  std::string toString() const override;
};

} // namespace cond
} // namespace rc

namespace std {

std::ostream& operator<<(std::ostream &os, const rc::cond::ConfigConstraints &c);

std::ostream& operator<<(std::ostream &os,
                         const rc::cond::ConfigurationsTransferDuration &ctd);

std::ostream& operator<<(std::ostream &os, const rc::cond::ValueOrRange &vor);

} // namespace std

#endif // H_CONFIG_CONSTRAINT not defined
