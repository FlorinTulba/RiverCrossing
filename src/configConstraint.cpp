/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.83) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2026 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#include "precompiled.h"

#ifdef UNIT_TESTING

/*
This include allows recompiling only the Unit tests project when updating the
tests. It also keeps the count of total code units to recompile to a minimum
value.
*/
#define CPP_CONFIG_CONSTRAINT
#include "configConstraint.hpp"
#undef CPP_CONFIG_CONSTRAINT

#endif  // UNIT_TESTING defined

#include "configConstraint.h"
#include "entitiesManager.h"
#include "mathRelated.h"
#include "util.h"
#include "warnings.h"

#include <cassert>
#include <cmath>
#include <exception>
#include <iomanip>
#include <ranges>

using namespace std;

namespace rc::cond {

AbsConfigConstraintValidatorExt::~AbsConfigConstraintValidatorExt() noexcept {
  delete nextExt;
}

AbsConfigConstraintValidatorExt::AbsConfigConstraintValidatorExt(
    unique_ptr<const IConfigConstraintValidatorExt> nextExt_) noexcept
    : nextExt{nextExt_.release()} {}

void AbsConfigConstraintValidatorExt::check(
    const IConfigConstraint& cfg,
    const ent::AllEntities& allEnts) const {
  nextExt->check(cfg, allEnts);

  if (const TypesConstraint* pTypesCfg{
          dynamic_cast<const TypesConstraint*>(&cfg)}) {
    checkTypesCfg(*pTypesCfg, allEnts);
    return;
  }

  if (const IdsConstraint* pIdsCfg{dynamic_cast<const IdsConstraint*>(&cfg)}) {
    checkIdsCfg(*pIdsCfg, allEnts);
    return;
  }
}

const DefConfigConstraintValidatorExt&
DefConfigConstraintValidatorExt::INST() noexcept {
  static const DefConfigConstraintValidatorExt inst;
  return inst;
}

MUTE_EXPLICIT_NEW_DELETE_WARN
unique_ptr<const DefConfigConstraintValidatorExt>
DefConfigConstraintValidatorExt::NEW_INST() noexcept {
  // Using new instead of make_unique, since the ctor is private
  // and not a friend of make_unique
  return unique_ptr<const DefConfigConstraintValidatorExt>{
      makeNoexcept([]() -> gsl::owner<DefConfigConstraintValidatorExt*> {
        // NOLINTNEXTLINE(bugprone-unhandled-exception-at-new)
        return new DefConfigConstraintValidatorExt;
      })};
}

const shared_ptr<const DefContextValidator>&
DefContextValidator::SHARED_INST() noexcept {
  MUTE_EXIT_TIME_DTOR_WARN
  // Using new instead of make_shared, since the ctor is private
  // and not a friend of make_shared
  static const shared_ptr<const DefContextValidator> inst{makeNoexcept([] {
    return shared_ptr<const DefContextValidator>{
        // NOLINTNEXTLINE(bugprone-unhandled-exception-at-new)
        new DefContextValidator};
  })};
  UNMUTE_WARNING

  return inst;
}

bool DefContextValidator::validate(const ent::MovingEntities&,
                                   const SymbolsTable&) const noexcept {
  return true;
}

AbsContextValidator::AbsContextValidator(
    const shared_ptr<const IContextValidator>& nextValidator_,
    const shared_ptr<const IValidatorExceptionHandler>& ownValidatorExcHandler_
    /* = {}*/) noexcept
    : nextValidator{nextValidator_},
      ownValidatorExcHandler{ownValidatorExcHandler_} {}

bool AbsContextValidator::validate(const ent::MovingEntities& ents,
                                   const SymbolsTable& st) const {
  bool resultOwnValidator{};
  try {
    resultOwnValidator = doValidate(ents, st);
  } catch (const exception&) {
    if (ownValidatorExcHandler) {
      const boost::logic::tribool excAssessment{
          ownValidatorExcHandler->assess(ents, st)};

      if (boost::logic::indeterminate(excAssessment))
        throw;  // not an exempted case

      resultOwnValidator = static_cast<bool>(excAssessment);

    } else
      throw;  // no saving exception handler
  }

  if (!resultOwnValidator)
    return false;

  return nextValidator->validate(ents, st);
}

const DefTransferConstraintsExt& DefTransferConstraintsExt::INST() noexcept {
  static const DefTransferConstraintsExt inst;
  return inst;
}

unique_ptr<const DefTransferConstraintsExt>
DefTransferConstraintsExt::NEW_INST() {
  // Using new instead of make_unique, since the ctor is private
  // and not a friend of make_unique
  return unique_ptr<const DefTransferConstraintsExt>{
      new DefTransferConstraintsExt};
}
UNMUTE_WARNING

unique_ptr<const IConfigConstraintValidatorExt>
DefTransferConstraintsExt::configValidatorExt() const noexcept {
  return DefConfigConstraintValidatorExt::NEW_INST();
}

AbsTransferConstraintsExt::~AbsTransferConstraintsExt() noexcept {
  delete nextExt;
}

AbsTransferConstraintsExt::AbsTransferConstraintsExt(
    unique_ptr<const ITransferConstraintsExt> nextExt_) noexcept
    : nextExt{nextExt_.release()} {}

unique_ptr<const IConfigConstraintValidatorExt>
AbsTransferConstraintsExt::configValidatorExt() const noexcept {
  return _configValidatorExt(nextExt->configValidatorExt());
}

bool AbsTransferConstraintsExt::check(const ent::MovingEntities& cfg) const {
  if (!_check(cfg))
    return false;

  return nextExt->check(cfg);
}

ConfigConstraints::ConfigConstraints(grammar::ConstraintsVec&& constraints_,
                                     const ent::AllEntities& allEnts_,
                                     bool allowed_ /* = true*/,
                                     bool postponeValidation /* = false*/)
    : constraints{std::move(constraints_)},
      allEnts{&allEnts_},
      _allowed{allowed_} {
  if (!postponeValidation)
    for (const auto& c : constraints)
      c->validate(*allEnts);
}

bool ConfigConstraints::allowed() const noexcept {
  return _allowed;
}

bool ConfigConstraints::empty() const noexcept {
  return constraints.empty();
}

bool ConfigConstraints::check(const ent::IsolatedEntities& ents) const {
  bool found{};
  for (const auto& c : constraints)
    if (c->matches(ents)) {
      found = true;
#ifndef NDEBUG
      if (!_allowed)
        cout << "violates NOT{" << *c << "} : "
             << ContView{ents.ids(),
                         {.before = "", .between = " ", .after = "\n"}}
             << flush;
#endif  // NDEBUG
      break;
    }
#ifndef NDEBUG
  if (_allowed != found)
    cout << "violates " << *this << " : "
         << ContView{ents.ids(), {.before = "", .between = " ", .after = "\n"}}
         << flush;
#endif  // NDEBUG
  return found == _allowed;
}

string ConfigConstraints::toString() const {
  if (constraints.empty())
    return "{}"s;

  ostringstream oss;
  if (!_allowed)
    oss << "NOT";

  oss << ContView{constraints,
                  {.before = "{ ", .between = " ; ", .after = " }"},
                  [](const auto& pConf) noexcept -> const IConfigConstraint& {
                    return *pConf;
                  }};
  return oss.str();
}

TransferConstraints::TransferConstraints(
    grammar::ConstraintsVec&& constraints_,
    const ent::AllEntities& allEnts_,
    const unsigned& capacity_,
    bool allowed_ /* = true*/,
    const ITransferConstraintsExt& extension_
    /* = DefTransferConstraintsExt::INST()*/)
    : ConfigConstraints{std::move(constraints_), allEnts_, allowed_, true},
      extension{&extension_},
      capacity{&capacity_} {
  unique_ptr<const IConfigConstraintValidatorExt> cfgConstrValExt{
      extension->configValidatorExt()};
  for (const auto& c : constraints)
    c->validate(*allEnts, *capacity, *cfgConstrValExt);
}

bool TransferConstraints::check(const ent::IsolatedEntities& ents) const {
  const ent::MovingEntities& ents_{
      dynamic_cast<const ent::MovingEntities&>(ents)};
#ifndef NDEBUG
  const auto& entsIds = ents.ids();
#endif  // NDEBUG

  if (ents.count() > static_cast<size_t>(*capacity)) {
#ifndef NDEBUG
    cout << "violates capacity constraint [ " << ents.count() << " > "
         << *capacity << " ] : "
         << ContView{entsIds, {.before = "", .between = " ", .after = "\n"}}
         << flush;
#endif  // NDEBUG
    return false;
  }

  if (!extension->check(ents_))
    return false;

  return ConfigConstraints::check(ents);
}

unsigned TransferConstraints::minRequiredCapacity() const noexcept {
  unsigned cap{};
  if (_allowed) {
    cap = 0U;
    for (const auto& c : constraints) {
      const unsigned longestMatchLength{c->longestMatchLength()};
      cap = std::max(cap, longestMatchLength);
    }
  } else {  // disallowed
    cap = UINT_MAX;
    for (const auto& c : constraints) {
      const unsigned longestMismatchLength{c->longestMismatchLength()};
      cap = std::min(cap, longestMismatchLength);
    }
  }

  return min(cap, (static_cast<unsigned>(allEnts->count()) - 1U));
}

ConfigurationsTransferDuration::ConfigurationsTransferDuration(
    grammar::ConfigurationsTransferDurationInitType&& initType,
    const ent::AllEntities& allEnts_,
    const unsigned& capacity,
    const ITransferConstraintsExt& extension_
    /* = DefTransferConstraintsExt::INST()*/)
    : constraints{std::move(initType.moveConstraints()), allEnts_, capacity,
                  true, extension_},
      _duration{initType.duration()} {
  // Silence 'rvalue never moved' warning for initType, since the move is
  // already done in the member initializer list
  const auto dummy{std::move(initType)};
}

const TransferConstraints& ConfigurationsTransferDuration::configConstraints()
    const noexcept {
  return constraints;
}

unsigned ConfigurationsTransferDuration::duration() const noexcept {
  return _duration;
}

string ConfigurationsTransferDuration::toString() const {
  ostringstream oss;
  oss << constraints << " need " << _duration << " time units";
  return oss.str();
}

unique_ptr<const IConfigConstraint> TypesConstraint::clone() const noexcept {
  return make_unique<const TypesConstraint>(*this);
}

void TypesConstraint::validate(
    const ent::AllEntities& allEnts,
    unsigned capacity /* = UINT_MAX*/,
    const IConfigConstraintValidatorExt& valExt
    /* = DefConfigConstraintValidatorExt::INST()*/) const {
  const map<string, set<unsigned>>& idsByTypes{allEnts.idsByTypes()};

  for (const string& t : mentionedTypes)
    if (!idsByTypes.contains(t))
      throw logic_error{HERE.function_name() + " - Unknown type name `"s + t +
                        "` in constraint `"s + toString() + "`!"s};

  size_t minRequiredCount{};
  for (const auto& typeAndLimits : mandatoryTypes) {
    const string& t{typeAndLimits.first};
    const set<unsigned>& matchingIds{idsByTypes.at(t)};
    const size_t minLim{static_cast<size_t>(typeAndLimits.second.first)};
    const size_t available{size(matchingIds)};
    if (minLim > available)
      throw logic_error{HERE.function_name() + " - Constraint `"s + toString() +
                        "` is asking for more entities ("s + to_string(minLim) +
                        ") of type "s + t + " than available ("s +
                        to_string(available) + ")!"s};

    minRequiredCount += minLim;
  }

  if (minRequiredCount > static_cast<size_t>(capacity))
    throw logic_error{HERE.function_name() + " - Constraint `"s + toString() +
                      "` is asking for more entities ("s +
                      to_string(minRequiredCount) + ") than the capacity ("s +
                      to_string(capacity) + ")!"s};

  valExt.check(*this, allEnts);
}

TypesConstraint& TypesConstraint::addTypeRange(
    const string& newType,
    unsigned minIncl /* = 0U*/,
    unsigned maxIncl /* = UINT_MAX*/) {
  if (minIncl > maxIncl)
    throw logic_error{HERE.function_name() +
                      " - Parameter minIncl must be at most maxIncl!"s};

  if (!mentionedTypes.insert(newType).second)
    throw logic_error{HERE.function_name() +
                      " - Duplicate newType parameter: "s + newType};

  if (0U == maxIncl) {
    cout << "[Notification] " << HERE.function_name()
         << ": Unnecessary term within the configuration: 0 x " << newType
         << '\n'
         << flush;
    return *this;
  }

  if (_longestMatchLength != UINT_MAX) {
    if (maxIncl != UINT_MAX)
      _longestMatchLength += maxIncl;
    else
      _longestMatchLength = UINT_MAX;
  }

  if (0U == minIncl)
    optionalTypes.emplace(newType, maxIncl);
  else
    mandatoryTypes.emplace(newType, make_pair(minIncl, maxIncl));

  return *this;
}

bool TypesConstraint::matches(
    const ent::IsolatedEntities& ents) const noexcept {
  const map<string, set<unsigned>>& entsByTypes{ents.idsByTypes()};

  if (size(entsByTypes) > size(mandatoryTypes) + size(optionalTypes))
    return false;  // too many types

  const auto itEnd{entsByTypes.cend()};

  for (const auto& typeRange : mandatoryTypes) {
    const auto it{makeNoexcept([&entsByTypes, &typeRange] {
      return entsByTypes.find(typeRange.first);
    })};
    if (it == itEnd)
      return false;  // expected type not present

    const unsigned count{gsl::narrow_cast<unsigned>(size(it->second))};
    if (count < typeRange.second.first || count > typeRange.second.second)
      return false;  // too few / many of this expected type
  }

  for (const auto& typeRange : optionalTypes) {
    const auto it{makeNoexcept([&entsByTypes, &typeRange] {
      return entsByTypes.find(typeRange.first);
    })};
    if (it == itEnd)
      continue;  // type not present
    const unsigned maxIncl{typeRange.second};
    const unsigned count{gsl::narrow_cast<unsigned>(size(it->second))};
    if (count > maxIncl)
      return false;  // too many of this optional type
  }

  for (const auto& idsWithType : entsByTypes) {
    const string& type{idsWithType.first};
    if (makeNoexcept([this, &type] { return mandatoryTypes.contains(type); }))
      continue;
    if (!makeNoexcept([this, &type] { return optionalTypes.contains(type); }))
      return false;  // found unwanted type
  }

  return true;
}

unsigned TypesConstraint::longestMatchLength() const noexcept {
  return _longestMatchLength;
}

string TypesConstraint::toString() const {
  ostringstream oss;
  oss << "[ ";

  for (const auto& typeRange : mandatoryTypes) {
    oss << typeRange.first;
    const unsigned minIncl{typeRange.second.first},
        maxIncl{typeRange.second.second};
    assert(minIncl > 0U);
    if (minIncl == maxIncl) {
      if (minIncl == 1U) {
        oss << ' ';
        continue;
      }
      oss << '{' << minIncl << "} ";
    } else {
      if (minIncl == 1U && maxIncl == UINT_MAX) {
        oss << "+ ";
        continue;
      }
      oss << '{' << minIncl << ',';
      if (maxIncl < UINT_MAX)
        oss << maxIncl;
      oss << "} ";
    }
  }
  for (const auto& typeRange : optionalTypes) {
    oss << typeRange.first;
    const unsigned maxIncl{typeRange.second};
    if (maxIncl == 1U)
      oss << "? ";
    else if (maxIncl == UINT_MAX)
      oss << "* ";
    else
      oss << "{0," << maxIncl << "} ";
  }

  oss << ']';
  return oss.str();
}

unique_ptr<const IConfigConstraint> IdsConstraint::clone() const noexcept {
  return make_unique<const IdsConstraint>(*this);
}

void IdsConstraint::validate(
    const ent::AllEntities& allEnts,
    unsigned capacity /* = UINT_MAX*/,
    const IConfigConstraintValidatorExt& valExt
    /* = DefConfigConstraintValidatorExt::INST()*/) const {
  const size_t requiredIdsCount{size(mandatoryGroups) +
                                static_cast<size_t>(expectedExtraIds)};
  const size_t available{allEnts.count()};

  if (requiredIdsCount > static_cast<size_t>(capacity))
    throw logic_error{HERE.function_name() + " - Constraint `"s + toString() +
                      "` is asking for more entities ("s +
                      to_string(requiredIdsCount) + ") than the capacity ("s +
                      to_string(capacity) + ")!"s};

  if (requiredIdsCount > available)
    throw logic_error{HERE.function_name() + " - Constraint `"s + toString() +
                      "` is asking for more entities ("s +
                      to_string(requiredIdsCount) + ") than available ("s +
                      to_string(available) + ")!"s};

  const set<unsigned>& ids{allEnts.ids()};
  for (const unsigned id : mentionedIds)
    if (!ids.contains(id))
      throw logic_error{HERE.function_name() + " - Unknown entity id `"s +
                        to_string(id) + "` in constraint `"s + toString() +
                        "`!"s};

  valExt.check(*this, allEnts);
}

IdsConstraint& IdsConstraint::addMandatoryId(unsigned id) {
  if (!mentionedIds.insert(id).second)
    throw logic_error{HERE.function_name() + " - Duplicate id parameter: "s +
                      to_string(id)};

  mandatoryGroups.emplace_back();
  mandatoryGroups.back().insert(id);

  if (_longestMatchLength != UINT_MAX)
    ++_longestMatchLength;

  return *this;
}

IdsConstraint& IdsConstraint::addOptionalId(unsigned id) {
  if (!mentionedIds.insert(id).second)
    throw logic_error{HERE.function_name() + " - Duplicate id parameter: "s +
                      to_string(id)};

  optionalGroups.emplace_back();
  optionalGroups.back().insert(id);

  if (_longestMatchLength != UINT_MAX)
    ++_longestMatchLength;

  return *this;
}

IdsConstraint& IdsConstraint::addAvoidedId(unsigned id) {
  if (!mentionedIds.insert(id).second)
    throw logic_error{HERE.function_name() + " - Duplicate id parameter: "s +
                      to_string(id)};
  avoidedIds.insert(id);

  return *this;
}

IdsConstraint& IdsConstraint::addUnspecifiedMandatory() noexcept {
  ++expectedExtraIds;

  if (_longestMatchLength != UINT_MAX)
    ++_longestMatchLength;

  return *this;
}

IdsConstraint& IdsConstraint::setUnbounded() noexcept {
  capacityLimit = false;

  _longestMatchLength = UINT_MAX;

  return *this;
}

bool IdsConstraint::satisfiedGroups(set<unsigned>& ids,
                                    bool forMandatory) const noexcept {
  const auto& groups{forMandatory ? mandatoryGroups : optionalGroups};

  for (const auto& group : groups) {
    bool found{};
    for (const unsigned id : group)
      if (ids.erase(id) != 0ULL) {
        if (found)       // detected a 2nd entity from this group
          return false;  // only 1 entity from a group can appear

        found = true;  // detected 1st entity from this group
      }

    if (!found && forMandatory)
      return false;  // mandatory group not covered
  }

  return true;
}

bool IdsConstraint::canSatisfyMandatoryGroups(
    set<unsigned>& ids) const noexcept {
  const size_t expectedMandatoryIds{size(mandatoryGroups) +
                                    static_cast<size_t>(expectedExtraIds)};

  if (size(ids) < expectedMandatoryIds)
    return false;  // there can't be enough mandatory id-s

  return satisfiedGroups(ids, true);
}

bool IdsConstraint::matches(const ent::IsolatedEntities& ents) const noexcept {
  set<unsigned> ids{makeCopyNoexcept(ents.ids())};

  for (const unsigned id : avoidedIds)
    if (ids.erase(id) != 0ULL)
      return false;  // found unwanted entity id

  if (!canSatisfyMandatoryGroups(ids))
    return false;

  if (!satisfiedGroups(ids, false))
    return false;

  const auto remainingIdsCount{size(ids)};
  if (remainingIdsCount < static_cast<size_t>(expectedExtraIds))
    return false;  // not enough mandatory extra id-s

  if (capacityLimit &&
      (remainingIdsCount > static_cast<size_t>(expectedExtraIds)))
    return false;  // more id-s than the limit

  return true;
}

unsigned IdsConstraint::longestMatchLength() const noexcept {
  return _longestMatchLength;
}

unsigned IdsConstraint::longestMismatchLength() const noexcept {
  /*
  UINT_MAX for anything else than a number of stars followed by ellipsis:
  * * * * ... - this would return 3 (number of stars minus 1)
  Avoided id-s ( !id ) don't matter.
  */
  if (capacityLimit || !mandatoryGroups.empty() || !optionalGroups.empty())
    return UINT_MAX;

  if (expectedExtraIds <= 1U)
    return 0U;

  return expectedExtraIds - 1U;
}

string IdsConstraint::toString() const {
  ostringstream oss;
  oss << "[";

  if (!mandatoryGroups.empty() || expectedExtraIds > 0U) {
    oss << " Mandatory={";
    for (const auto& group : mandatoryGroups) {
      if (size(group) == 1ULL)
        oss << *cbegin(group);
      else
        oss << ContView{group, {.before = "(", .between = "|", .after = ")"}};
      oss << ' ';
    }

    if (expectedExtraIds > 0U)
      oss << "extra_ids_count=" << expectedExtraIds << ' ';

    // Overwrite last blank with }
    oss.seekp(-1, ios_base::cur);
    oss << '}';
  }

  if (!avoidedIds.empty())
    oss << " Avoided="
        << ContView{avoidedIds, {.before = "{", .between = ",", .after = "}"}};

  if (!optionalGroups.empty()) {
    oss << " Optional={";
    for (const auto& group : optionalGroups) {
      if (size(group) == 1ULL)
        oss << *cbegin(group);
      else
        oss << ContView{group, {.before = "(", .between = "|", .after = ")"}};
      oss << ' ';
    }

    // Overwrite last blank with }
    oss.seekp(-1, ios_base::cur);
    oss << '}';
  }

  if (!capacityLimit)
    oss << " any_number_from_the_others";

  oss << " ]";
  return oss.str();
}

BoolConst::BoolConst(bool b) noexcept : LogicalExpr{b} {}

bool BoolConst::eval(const SymbolsTable&) const noexcept {
  assert(val.has_value());
  return val.value_or(false);
}

string BoolConst::toString() const {
  ostringstream oss;
  assert(val.has_value());
  oss << boolalpha << val.value_or(false);
  return oss.str();
}

Not::Not(const shared_ptr<const LogicalExpr>& le)
    : LogicalExpr{le->constValue().transform([](bool b) { return !b; })},
      _le{le} {}

bool Not::dependsOnVariable(const string& varName) const noexcept {
  return _le->dependsOnVariable(varName);
}

bool Not::eval(const SymbolsTable& st) const {
  return val.value_or(!_le->eval(st));
}

string Not::toString() const {
  ostringstream oss;
  oss << "not(" << _le->toString() << ')';
  return oss.str();
}

void ValueOrRange::validateDouble(double d) {
  if (isnan(d))
    throw logic_error{
        HERE.function_name() +
        " - The value or the range limits need to be valid double values!"s};
}

void ValueOrRange::validateRange(double a, double b) {
  if (a > b)
    throw logic_error{HERE.function_name() +
                      " - The range limits need to be in ascending order!"s};
}

void ValueOrRange::validate() const {
  if (const ValueType* pValue{get_if<ValueType>(&_valueOrRange)}) {
    const gsl::not_null<const ValueType> value_{*pValue};
    if (const std::optional<double>& v{value_->constValue()})
      validateDouble(*v);
  } else {
    const auto& [from_, to_] = get<RangeType>(_valueOrRange);
    const optional<double>& from{
        gsl::not_null<const ValueType>(from_)->constValue()};
    if (from.has_value())
      validateDouble(*from);

    if (to_) {
      const std::optional<double>& to{to_->constValue()};
      if (to) {
        validateDouble(*to);

        if (from)
          validateRange(*from, *to);
      }
    }
  }
}

ValueOrRange::ValueOrRange(const ValueType& value_)
    : _valueOrRange{gsl::not_null<const ValueType>{value_}.get()} {
  validate();
}

ValueOrRange::ValueOrRange(const ValueType& from_, const ValueType& to_)
    : _valueOrRange{make_pair(gsl::not_null<const ValueType>{from_}.get(),
                              gsl::not_null<const ValueType>{to_}.get())} {
  validate();
}

ValueOrRange::~ValueOrRange() noexcept {
  _valueOrRange = {};
}

ValueOrRange::ValueOrRange(const ValueOrRange&) noexcept = default;

ValueOrRange::ValueOrRange(ValueOrRange&& other) noexcept
    : _valueOrRange{std::move(other._valueOrRange)} {}

bool ValueOrRange::dependsOnVariable(const string& varName) const noexcept {
  if (isConst())
    return false;

  if (const ValueType* value_{get_if<ValueType>(&_valueOrRange)})
    return (*value_)->dependsOnVariable(varName);

  const RangeType range{get<RangeType>(_valueOrRange)};
  return range.first->dependsOnVariable(varName) ||
         range.second->dependsOnVariable(varName);
}

bool ValueOrRange::isConst() const noexcept {
  if (const ValueType* value_{get_if<ValueType>(&_valueOrRange)})
    return (*value_)->constValue().has_value();

  const RangeType range{get<RangeType>(_valueOrRange)};
  return range.first->constValue().has_value() &&
         range.second->constValue().has_value();
}

bool ValueOrRange::isRange() const noexcept {
  return holds_alternative<RangeType>(_valueOrRange);
}

double ValueOrRange::value(const SymbolsTable& st) const {
  if (isRange())
    throw logic_error{HERE.function_name() +
                      " - cannot be called on a range!"s};
  const double v{get<ValueType>(_valueOrRange)->eval(st)};
  validateDouble(v);
  return v;
}

pair<double, double> ValueOrRange::range(const SymbolsTable& st) const {
  if (!isRange())
    throw logic_error{HERE.function_name() +
                      " - cannot be called on a simple value!"s};
  const RangeType range{get<RangeType>(_valueOrRange)};
  const double from{range.first->eval(st)};
  const double to{range.second->eval(st)};
  validateDouble(from);
  validateDouble(to);
  validateRange(from, to);
  return {from, to};
}

string ValueOrRange::toString() const {
  ostringstream oss;
  if (const ValueType* value_{get_if<ValueType>(&_valueOrRange)})
    oss << **value_;
  else {
    const RangeType range{get<RangeType>(_valueOrRange)};
    oss << *range.first << " .. " << *range.second;
  }

  return oss.str();
}

ValueSet& ValueSet::add(const ValueOrRange& vor) noexcept {
  makeNoexcept([this, &vor] { vors.push_back(vor); });

  if (isConst && !vor.isConst())
    isConst = false;

  return *this;
}

bool ValueSet::empty() const noexcept {
  return vors.empty();
}

bool ValueSet::dependsOnVariable(const string& varName) const noexcept {
  if (isConst)
    return false;

  return ranges::any_of(vors, [&varName](const ValueOrRange& vor) noexcept {
    return vor.dependsOnVariable(varName);
  });
}

bool ValueSet::constSet() const noexcept {
  return isConst;
}

bool ValueSet::contains(const double& v,
                        const SymbolsTable& st /* = {}*/) const {
  return ranges::any_of(vors, [&st, v](const ValueOrRange& vor) {
    if (vor.isRange()) {
      const auto limits = vor.range(st);
      if (limits.first <= v && limits.second >= v)
        return true;

    } else if (abs(v - vor.value(st)) < Eps)
      return true;

    return false;
  });
}

string ValueSet::toString() const {
  return ContView{vors, {.before = "{", .between = ", ", .after = "}"}}
      .toString();
}

NumericConst::NumericConst(double d) noexcept : NumericExpr{d} {}

double NumericConst::eval(const SymbolsTable&) const noexcept {
  assert(val.has_value());
  return val.value_or(0.);
}

string NumericConst::toString() const {
  ostringstream oss;
  assert(val.has_value());
  oss << val.value_or(0.);
  return oss.str();
}

NumericVariable::NumericVariable(string varName) noexcept
    : name{std::move(varName)} {}

bool NumericVariable::dependsOnVariable(const string& varName) const noexcept {
  return name == varName;
}

double NumericVariable::eval(const SymbolsTable& st) const {
  return st.at(name);
}

string NumericVariable::toString() const noexcept {
  return makeCopyNoexcept(name);
}

Addition::Addition(const shared_ptr<const NumericExpr>& left_,
                   const shared_ptr<const NumericExpr>& right_) noexcept
    : NumericExpr{[&left_, &right_]() noexcept -> optional<double> {
        if (const auto &lOpt{left_->constValue()}, &rOpt{right_->constValue()};
            lOpt.has_value() && rOpt.has_value())
          return make_optional(lOpt.value() + rOpt.value());
        return nullopt;
      }()},
      left(left_),
      right(right_) {}

bool Addition::dependsOnVariable(const string& varName) const noexcept {
  return !val.has_value() && (left->dependsOnVariable(varName) ||
                              right->dependsOnVariable(varName));
}

double Addition::eval(const SymbolsTable& st) const {
  return val.value_or(left->eval(st) + right->eval(st));
}

string Addition::toString() const {
  if (val)
    return to_string(*val);
  return "("s + left->toString() + " + "s + right->toString() + ")"s;
}

long Modulus::validLong(double v) {
  const long result{static_cast<long>(v)};
  if (abs(v - static_cast<double>(result)) > Eps)
    throw logic_error{HERE.function_name() +
                      " - Operands of modulus need to be integer values!"s};
  return result;
}

long Modulus::validOperation(long numeratorL, long denominatorL) {
  if (0L == denominatorL) {
    if (0L != numeratorL)
      throw overflow_error{HERE.function_name() + " - denominator is 0!"s};
    throw logic_error{HERE.function_name() + " - both operands are 0!"s};
  }

  return numeratorL % denominatorL;
}

Modulus::Modulus(const shared_ptr<const NumericExpr>& numerator_,
                 const shared_ptr<const NumericExpr>& denominator_)
    : numerator(numerator_),
      denominator(denominator_) {
  // when the denominator is 1 or -1, the result is always 0
  const auto& denomValOpt{denominator->constValue()};
  const bool hasConstDenominator{denomValOpt.has_value()};
  if (hasConstDenominator && 1L == abs(validLong(denomValOpt.value()))) {
    val = 0.;
    return;
  }

  const auto& numerValOpt{numerator->constValue()};
  const bool hasConstNumerator{numerValOpt.has_value()};
  if (hasConstNumerator) {
    const long numeratorL{validLong(numerValOpt.value())};

    // when both terms are constants, set the result directly
    if (hasConstDenominator) {
      const long denominatorL{validLong(denomValOpt.value())};
      val = static_cast<double>(validOperation(numeratorL, denominatorL));
      return;
    }
  }
}

bool Modulus::dependsOnVariable(const string& varName) const noexcept {
  return !val.has_value() && (numerator->dependsOnVariable(varName) ||
                              denominator->dependsOnVariable(varName));
}

double Modulus::eval(const SymbolsTable& st) const {
  if (val)  // return the modulus directly if it involves only constants
    return *val;

  const long numeratorL{validLong(numerator->eval(st))};
  const long denominatorL{validLong(denominator->eval(st))};
  return static_cast<double>(validOperation(numeratorL, denominatorL));
}

string Modulus::toString() const {
  if (val)
    return to_string(*val);
  return "("s + numerator->toString() + " % "s + denominator->toString() + ")"s;
}

}  // namespace rc::cond

namespace std {

auto& operator<<(auto& os, const rc::cond::ValueSet& vs) {
  os << vs.toString();
  return os;
}

}  // namespace std
