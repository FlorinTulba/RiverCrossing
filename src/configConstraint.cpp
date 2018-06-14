/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#include "configConstraint.h"
#include "entitiesManager.h"
#include "mathRelated.h"

#include <algorithm>
#include <iomanip>

using namespace std;

namespace rc {
namespace cond {

AbsConfigConstraintValidatorExt::AbsConfigConstraintValidatorExt(
    const shared_ptr<const IConfigConstraintValidatorExt> &nextExt_) :
  nextExt(CP(nextExt_)) {}

void AbsConfigConstraintValidatorExt::check(const IConfigConstraint &cfg,
           const shared_ptr<const ent::AllEntities> &allEnts) const {
  assert(nullptr != nextExt);
  nextExt->check(cfg, allEnts);

  const TypesConstraint *pTypesCfg =
    dynamic_cast<const TypesConstraint*>(&cfg);
  if(nullptr != pTypesCfg) {
    checkTypesCfg(*pTypesCfg, allEnts);
    return;
  }

  const IdsConstraint *pIdsCfg =
    dynamic_cast<const IdsConstraint *>(&cfg);
  if(nullptr != pIdsCfg) {
    checkIdsCfg(*pIdsCfg, allEnts);
    return;
  }
}

const shared_ptr<const DefConfigConstraintValidatorExt>&
        DefConfigConstraintValidatorExt::INST() {
  static const shared_ptr<const DefConfigConstraintValidatorExt>
    inst(new DefConfigConstraintValidatorExt);
  return inst;
}

const shared_ptr<const DefContextValidator>& DefContextValidator::INST() {
  static const shared_ptr<const DefContextValidator>
    inst(new DefContextValidator);
  return inst;
}

AbsContextValidator::AbsContextValidator(
    const shared_ptr<const IContextValidator> &nextValidator_,
    const shared_ptr<const IValidatorExceptionHandler> &ownValidatorExcHandler_
      /* = nullptr*/) :
  nextValidator(CP(nextValidator_)),
  ownValidatorExcHandler(ownValidatorExcHandler_) {}

bool AbsContextValidator::validate(const ent::MovingEntities &ents,
              const SymbolsTable &st) const {
  bool resultOwnValidator = false;
  try {
    resultOwnValidator = doValidate(ents, st);
  } catch(const exception&) {
    if(nullptr != ownValidatorExcHandler) {
      const boost::logic::tribool excAssessment =
        ownValidatorExcHandler->assess(ents, st);

      if(boost::logic::indeterminate(excAssessment))
        throw; // not an exempted case

      resultOwnValidator = excAssessment;

    } else throw; // no saving exception handler
  }

  if( ! resultOwnValidator)
    return false;

  assert(nextValidator);
  return nextValidator->validate(ents, st);
}

const shared_ptr<const DefTransferConstraintsExt>&
            DefTransferConstraintsExt::INST() {
  static const shared_ptr<const DefTransferConstraintsExt>
    inst(new DefTransferConstraintsExt);
  return inst;
}

shared_ptr<const IConfigConstraintValidatorExt>
      DefTransferConstraintsExt::configValidatorExt() const {
  return DefConfigConstraintValidatorExt::INST();
}

AbsTransferConstraintsExt::AbsTransferConstraintsExt(
    const shared_ptr<const ITransferConstraintsExt> &nextExt_) :
  nextExt(CP(nextExt_)) {}

shared_ptr<const IConfigConstraintValidatorExt>
        AbsTransferConstraintsExt::configValidatorExt() const {
  assert(nullptr != nextExt);
  return _configValidatorExt(nextExt->configValidatorExt());
}

bool AbsTransferConstraintsExt::check(const ent::MovingEntities &cfg) const {
  if( ! _check(cfg))
    return false;

  assert(nullptr != nextExt);
  return nextExt->check(cfg);
}

ConfigConstraints::ConfigConstraints(grammar::ConstraintsVec &&constraints_,
      const shared_ptr<const ent::AllEntities> &allEnts_,
      bool allowed_/* = true*/,
      bool postponeValidation/* = false*/) :
    constraints(move(constraints_)), allEnts(allEnts_), _allowed(allowed_) {
  if( ! postponeValidation)
    for(const auto &c : constraints)
      c->validate(allEnts);
}

bool ConfigConstraints::allowed() const {
  return _allowed;
}

bool ConfigConstraints::empty() const {
  return constraints.empty();
}

bool ConfigConstraints::check(const ent::IsolatedEntities &ents) const {
  bool found = false;
  for(const auto &c : constraints)
    if(c->matches(ents)) {
      found = true;
#ifndef NDEBUG
    if( ! _allowed) {
      cout<<"violates NOT{"<<*c<<"} : ";
      copy(CBOUNDS(ents.ids()), ostream_iterator<unsigned>(cout, " "));
      cout<<endl;
    }
#endif // NDEBUG
      break;
    }
#ifndef NDEBUG
    if(_allowed != found) {
      cout<<"violates "<<*this<<" : ";
      copy(CBOUNDS(ents.ids()), ostream_iterator<unsigned>(cout, " "));
      cout<<endl;
    }
#endif // NDEBUG
  return found == _allowed;
}

string ConfigConstraints::toString() const {
  ostringstream oss;
  if(constraints.empty())
    oss<<"{}";
  else {
    if( ! _allowed)
      oss<<"NOT";
    oss<<"{ ";
    for(auto &pConf : constraints)
      oss<<*pConf<<" ; ";
    oss<<"\b\b}";
  }
  return oss.str();
}

TransferConstraints::TransferConstraints(grammar::ConstraintsVec &&constraints_,
        const shared_ptr<const ent::AllEntities> &allEnts_,
        const unsigned &capacity_, bool allowed_/* = true*/,
        const shared_ptr<const ITransferConstraintsExt> &extension_
            /* = DefTransferConstraintsExt::INST()*/) :
    ConfigConstraints(move(constraints_), allEnts_, allowed_, true),
    capacity(capacity_), extension(CP(extension_)) {
  for(const auto &c : constraints)
    c->validate(allEnts, capacity, extension->configValidatorExt());
}

bool TransferConstraints::check(const ent::IsolatedEntities &ents) const {
  const ent::MovingEntities *pEnts =
    dynamic_cast<const ent::MovingEntities*>(&ents);
  CP_EX_MSG(pEnts, std::logic_error,
            "needs a (subclass of) MovingEntities parameter!");
#ifndef NDEBUG
  const auto &entsIds = ents.ids();
#endif // NDEBUG

  if((unsigned)ents.count() > capacity) {
#ifndef NDEBUG
    cout<<"violates capacity constraint [ "
      <<ents.count()<<" > "<<capacity<<" ] : ";
    copy(CBOUNDS(entsIds), ostream_iterator<unsigned>(cout, " "));
    cout<<endl;
#endif // NDEBUG
    return false;
  }

  assert(nullptr != extension);
  if( ! extension->check(*pEnts))
    return false;

  return ConfigConstraints::check(ents);
}

unsigned TransferConstraints::minRequiredCapacity() const {
  unsigned cap;
  if(_allowed) {
    cap = 0U;
    for(const auto &c : constraints) {
      const unsigned longestMatchLength = c->longestMatchLength();
      if(cap < longestMatchLength)
        cap = longestMatchLength;
    }
  } else { // disallowed
    cap = UINT_MAX;
    for(const auto &c : constraints) {
      const unsigned longestMismatchLength = c->longestMismatchLength();
      if(cap > longestMismatchLength)
        cap = longestMismatchLength;
    }
  }

  return min(cap, ((unsigned)allEnts->count() - 1U));
}

ConfigurationsTransferDuration::ConfigurationsTransferDuration(
      grammar::ConfigurationsTransferDurationInitType &&initType,
      const shared_ptr<const ent::AllEntities> &allEnts_,
      const unsigned &capacity,
      const shared_ptr<const ITransferConstraintsExt> &extension_
            /* = DefTransferConstraintsExt::INST()*/) :
    constraints(move(initType.moveConstraints()),
                allEnts_, capacity, true, extension_),
    _duration(initType.duration()) {}

const TransferConstraints& ConfigurationsTransferDuration::configConstraints() const {
  return constraints;
}

unsigned ConfigurationsTransferDuration::duration() const {
  return _duration;
}

string ConfigurationsTransferDuration::toString() const {
  ostringstream oss;
  oss<<constraints<<" need "<<_duration<<" time units";
  return oss.str();
}

unique_ptr<const IConfigConstraint> TypesConstraint::clone() const {
  return make_unique<const TypesConstraint>(*this);
}

void TypesConstraint::validate(const shared_ptr<const ent::AllEntities> &allEnts,
         unsigned capacity/* = UINT_MAX*/,
         const shared_ptr<const IConfigConstraintValidatorExt> &valExt
            /* = DefConfigConstraintValidatorExt::INST()*/) const {
  CP(valExt);

  const map<string, set<unsigned>> &idsByTypes = allEnts->idsByTypes();
  const auto idsByTypesEnd = idsByTypes.cend();

  for(const string &t : mentionedTypes)
    if(idsByTypes.find(t) == idsByTypesEnd)
      throw logic_error(string(__func__) +
        " - Unknown type name `"s + t + "` in constraint `"s + toString() + "`!"s);

  size_t minRequiredCount = 0ULL;
  for(const auto &typeAndLimits : mandatoryTypes) {
    const string &t = typeAndLimits.first;
    const set<unsigned> &matchingIds = idsByTypes.at(t);
    const size_t minLim = (size_t)typeAndLimits.second.first,
      available = matchingIds.size();
    if(minLim > available)
      throw logic_error(string(__func__) + " - Constraint `"s + toString() +
        "` is asking for more entities ("s + to_string(minLim) +
        ") of type "s + t + " than available ("s + to_string(available) + ")!"s);

    minRequiredCount += minLim;
  }

  if(minRequiredCount > (size_t)capacity)
    throw logic_error(string(__func__) + " - Constraint `"s + toString() +
      "` is asking for more entities ("s + to_string(minRequiredCount) +
      ") than the capacity ("s + to_string(capacity) + ")!"s);

  valExt->check(*this, allEnts);
}

TypesConstraint& TypesConstraint::addTypeRange(const string &newType,
                                               unsigned minIncl/* = 0U*/,
                                               unsigned maxIncl/* = UINT_MAX*/) {
  if(minIncl > maxIncl)
    throw logic_error(string(__func__) +
      " - Parameter minIncl must be at most maxIncl!");

  if( ! mentionedTypes.insert(newType).second)
    throw logic_error(string(__func__) + " - Duplicate newType parameter: "s +
      newType);

  if(maxIncl == 0U) {
    cout<<"[Notification] "<<__func__
      <<": Unnecessary term within the configuration: 0 x "<<newType<<endl;
    return *this;
  }

  if(_longestMatchLength != UINT_MAX) {
    if(maxIncl != UINT_MAX) _longestMatchLength += maxIncl;
    else                    _longestMatchLength = UINT_MAX;
  }

  if(minIncl == 0U)
    optionalTypes.emplace(newType, maxIncl);
  else
    mandatoryTypes.emplace(newType, make_pair(minIncl, maxIncl));

  return *this;
}

bool TypesConstraint::matches(const ent::IsolatedEntities &ents) const {
  const map<string, set<unsigned>> &entsByTypes = ents.idsByTypes();

  if(entsByTypes.size() > mandatoryTypes.size() + optionalTypes.size())
    return false; // too many types

  auto itEnd = entsByTypes.cend();

  for(const auto &typeRange : mandatoryTypes) {
    auto it = entsByTypes.find(typeRange.first);
    if(it == itEnd)
      return false; // expected type not present

    const unsigned count = (unsigned)it->second.size();
    if(count < typeRange.second.first || count > typeRange.second.second)
      return false; // too few / many of this expected type
  }

  for(const auto &typeRange : optionalTypes) {
    auto it = entsByTypes.find(typeRange.first);
    if(it == itEnd)
      continue; // type not present
    const unsigned maxIncl = typeRange.second,
      count = (unsigned)it->second.size();
    if(count > maxIncl)
      return false; // too many of this optional type
  }

  for(const auto &idsWithType : entsByTypes) {
    const string &type = idsWithType.first;
    if(mandatoryTypes.find(type) != mandatoryTypes.cend())
      continue;
    if(optionalTypes.find(type) == optionalTypes.cend())
      return false; // found unwanted type
  }

  return true;
}

unsigned TypesConstraint::longestMatchLength() const {
  return _longestMatchLength;
}

string TypesConstraint::toString() const {
  ostringstream oss;
  oss<<"[ ";

  for(const auto &typeRange : mandatoryTypes) {
    oss<<typeRange.first;
    const unsigned minIncl = typeRange.second.first,
      maxIncl = typeRange.second.second;
    assert(minIncl > 0U);
    if(minIncl == maxIncl) {
      if(minIncl == 1U) {
        oss<<' ';
        continue;
      }
      oss<<'{'<<minIncl<<"} ";
    } else {
      if(minIncl == 1U && maxIncl == UINT_MAX) {
        oss<<"+ ";
        continue;
      }
      oss<<'{'<<minIncl<<',';
      if(maxIncl < UINT_MAX)
        oss<<maxIncl;
      oss<<"} ";
    }
  }
  for(const auto &typeRange : optionalTypes) {
    oss<<typeRange.first;
    const unsigned maxIncl = typeRange.second;
    if(maxIncl == 1U)
      oss<<"? ";
    else if(maxIncl == UINT_MAX)
      oss<<"* ";
    else oss<<"{0,"<<maxIncl<<"} ";
  }

  oss<<']';
  return oss.str();
}

unique_ptr<const IConfigConstraint> IdsConstraint::clone() const {
  return make_unique<const IdsConstraint>(*this);
}

void IdsConstraint::validate(const shared_ptr<const ent::AllEntities> &allEnts,
         unsigned capacity/* = UINT_MAX*/,
         const shared_ptr<const IConfigConstraintValidatorExt> &valExt
            /* = DefConfigConstraintValidatorExt::INST()*/) const {
  CP(valExt);

  const size_t requiredIdsCount = mandatoryGroups.size() + (size_t)expectedExtraIds,
    available = allEnts->count();
  if(requiredIdsCount > (size_t)capacity)
    throw logic_error(string(__func__) + " - Constraint `"s + toString() +
      "` is asking for more entities ("s + to_string(requiredIdsCount) +
      ") than the capacity ("s + to_string(capacity) + ")!"s);

  if(requiredIdsCount > available)
    throw logic_error(string(__func__) + " - Constraint `"s + toString() +
      "` is asking for more entities ("s + to_string(requiredIdsCount) +
      ") than available ("s + to_string(available) + ")!"s);

  const set<unsigned> &ids = allEnts->ids();
  const auto idsEnd = ids.cend();
  for(unsigned id : mentionedIds)
    if(ids.find(id) == idsEnd)
      throw logic_error(string(__func__) + " - Unknown entity id `"s +
        to_string(id) + "` in constraint `"s + toString() + "`!"s);

  valExt->check(*this, allEnts);
}

IdsConstraint& IdsConstraint::addMandatoryId(unsigned id) {
  if( ! mentionedIds.insert(id).second)
      throw logic_error(string(__func__) + " - Duplicate id parameter: "s +
        to_string(id));

  mandatoryGroups.push_back({});
  mandatoryGroups.back().insert(id);

  if(_longestMatchLength != UINT_MAX)
    ++_longestMatchLength;

  return *this;
}

IdsConstraint& IdsConstraint::addOptionalId(unsigned id) {
  if( ! mentionedIds.insert(id).second)
      throw logic_error(string(__func__) + " - Duplicate id parameter: "s +
        to_string(id));

  optionalGroups.push_back({});
  optionalGroups.back().insert(id);

  if(_longestMatchLength != UINT_MAX)
    ++_longestMatchLength;

  return *this;
}

IdsConstraint& IdsConstraint::addAvoidedId(unsigned id) {
  if( ! mentionedIds.insert(id).second)
      throw logic_error(string(__func__) + " - Duplicate id parameter: "s +
        to_string(id));
  avoidedIds.insert(id);

  return *this;
}

IdsConstraint& IdsConstraint::addUnspecifiedMandatory() {
  ++expectedExtraIds;

  if(_longestMatchLength != UINT_MAX)
    ++_longestMatchLength;

  return *this;
}

IdsConstraint& IdsConstraint::setUnbounded() {
  capacityLimit = false;

  _longestMatchLength = UINT_MAX;

  return *this;
}

bool IdsConstraint::matches(const ent::IsolatedEntities &ents) const {
  auto ids = ents.ids();

  for(const unsigned id : avoidedIds)
    if(ids.erase(id) != 0ULL)
      return false; // found unwanted entity id

  const unsigned expectedMandatoryIds =
    (unsigned)mandatoryGroups.size() + expectedExtraIds;

  if(ids.size() < (size_t)expectedMandatoryIds)
    return false; // there can't be enough mandatory id-s

  for(const auto &group : mandatoryGroups) {
    bool found = false;
    for(const unsigned id : group)
      if(ids.erase(id) != 0ULL) {
        if(found) // detected a 2nd entity from this mandatory group
          return false; // only 1 entity from a mandatory group can appear
        found = true; // detected 1st entity from this mandatory group
      }
    if( ! found)
      return false; // mandatory group not covered
  }

  for(const auto &group : optionalGroups) {
    bool found = false;
    for(const unsigned id : group)
      if(ids.erase(id) != 0ULL) {
        if(found) // detected a 2nd entity from this optional group
          return false; // only 1 entity from a optional group can appear
        found = true; // detected 1st entity from this optional group
      }
  }

  if(ids.size() < (size_t)expectedExtraIds)
    return false; // not enough mandatory extra id-s

  if(capacityLimit && (ids.size() > (size_t)expectedExtraIds))
    return false; // more id-s than the limit

  return true;
}

unsigned IdsConstraint::longestMatchLength() const {
  return _longestMatchLength;
}

unsigned IdsConstraint::longestMismatchLength() const {
  /*
  UINT_MAX for anything else than a number of stars followed by ellipsis:
  * * * * ... - this would return 3 (number of stars minus 1)
  Avoided id-s ( !id ) don't matter.
  */
  if(capacityLimit || ! mandatoryGroups.empty() || ! optionalGroups.empty())
    return UINT_MAX;

  if(expectedExtraIds <= 1U)
    return 0U;

  return expectedExtraIds - 1U;
}

string IdsConstraint::toString() const {
  ostringstream oss;
  oss<<"[";

  if( ! mandatoryGroups.empty() || expectedExtraIds > 0U) {
    oss<<" Mandatory={";
    for(const auto &group : mandatoryGroups) {
      if(group.size() == 1ULL)
        oss<<*group.begin()<<' ';
      else {
        // This branch produces: `(2|}` for `mothersWithChildren.json`
        // for the constraint: `0 (4 | 5) !3 ... ; 3 (1 | 2) !0 ...`
        // Iterating the unordered_set seems to be the problem
        // Probably a bug in unordered_set
        oss<<'(';
        // oss<<"sz="<<group.size()<<'!'; // this appears to remove the problem
        copy(CBOUNDS(group), ostream_iterator<unsigned>(oss, "|"));
        oss<<"\b) ";
      }
    }

    if(expectedExtraIds > 0U)
      oss<<"extra_ids_count="<<expectedExtraIds<<' ';
    oss<<"\b}";
  }

  if( ! avoidedIds.empty()) {
    oss<<" Avoided={";
    copy(CBOUNDS(avoidedIds), ostream_iterator<unsigned>(oss, ","));
    oss<<"\b}";
  }

  if( ! optionalGroups.empty()) {
    oss<<" Optional={";
    for(const auto &group : optionalGroups) {
      if(group.size() == 1ULL)
        oss<<*group.begin()<<' ';
      else {
        oss<<'(';
        copy(CBOUNDS(group), ostream_iterator<unsigned>(oss, "|"));
        oss<<"\b) ";
      }
    }
    oss<<"\b}";
  }

  if( ! capacityLimit)
    oss<<" any_number_from_the_others";

  oss<<" ]";
  return oss.str();
}

BoolConst::BoolConst(bool b) : LogicalExpr() {val = b;}

bool BoolConst::eval(const SymbolsTable &) const {return *val;}

string BoolConst::toString() const {
  ostringstream oss;
  oss<<boolalpha<<*val;
  return oss.str();
}

Not::Not(const shared_ptr<const LogicalExpr> &le) :
    LogicalExpr(), _le(CP_EX(le, std::logic_error)) {
  if(_le->constValue())
    val = ! *_le->constValue();
}

bool Not::dependsOnVariable(const string &varName) const {
  return _le->dependsOnVariable(varName);
}

bool Not::eval(const SymbolsTable &st) const {
  if(val)
    return *val;
  return ! _le->eval(st);
}

string Not::toString() const {
  ostringstream oss;
  oss<<"not("<<_le->toString()<<')';
  return oss.str();
}

void ValueOrRange::validateDouble(double d) {
  if(isNaN(d))
    throw logic_error(string(__func__) +
      " - The value or the range limits need to be valid double values!");
}

void ValueOrRange::validateRange(double a, double b) {
  if(a > b)
    throw logic_error(string(__func__) +
      " - The range limits need to be in ascending order!");
}

void ValueOrRange::validate() const {
  const boost::optional<double> &v =
    CP_EX(_value, std::logic_error)->constValue();
  if(v)
    validateDouble(*v);

  if(_to) {
    const boost::optional<double> &to = _to->constValue();
    if(to) {
      validateDouble(*to);

      const boost::optional<double> &from = v;
      if(from)
        validateRange(*from, *to);
    }
  }
}

ValueOrRange::ValueOrRange(const shared_ptr<const NumericExpr> &value_) :
    _value(value_) {
  validate();
}

ValueOrRange::ValueOrRange(const shared_ptr<const NumericExpr> &from_,
    const shared_ptr<const NumericExpr> &to_) : _from(from_), _to(CP(to_)) {
  validate();
}

ValueOrRange::~ValueOrRange() {
  _value = _to = nullptr;
}

ValueOrRange::ValueOrRange(const ValueOrRange &other) :
  _value(other._value), _to(other._to) {}

bool ValueOrRange::dependsOnVariable(const string &varName) const {
  return ! isConst() && (
    _value->dependsOnVariable(varName) || (
      isRange() &&
      _to->dependsOnVariable(varName)));
}

bool ValueOrRange::isConst() const {
  return _value->constValue() &&
    ( ! isRange() || _to->constValue());
}

bool ValueOrRange::isRange() const {return _to != nullptr;}

double ValueOrRange::value(const SymbolsTable &st) const {
  if(isRange())
    throw logic_error(string(__func__) +
      " - cannot be called on a range!");
  const double v = _value->eval(st);
  validateDouble(v);
  return v;
}

pair<double, double> ValueOrRange::range(const SymbolsTable &st) const {
  if( ! isRange())
    throw logic_error(string(__func__) +
      " - cannot be called on a simple value!");
  const double from = _from->eval(st), to = _to->eval(st);
  validateDouble(from); validateDouble(to); validateRange(from, to);
  return {from, to};
}

string ValueOrRange::toString() const {
  ostringstream oss;
  if(isRange())
    oss<<*_from<<" .. "<<*_to;
  else
    oss<<*_value;
  return oss.str();
}

ValueSet& ValueSet::add(const ValueOrRange &vor) {
  vors.push_back(vor);
  if(isConst && ! vor.isConst())
    isConst = false;

  return *this;
}

bool ValueSet::empty() const {
  return vors.empty();
}

bool ValueSet::dependsOnVariable(const string &varName) const {
  if(isConst)
    return false;

  for(const ValueOrRange &vor : vors)
    if(vor.dependsOnVariable(varName))
      return true;

  return false;
}

bool ValueSet::constSet() const {
  return isConst;
}

bool ValueSet::contains(const double &v, const SymbolsTable &st/* = {}*/) const {
  for(const ValueOrRange &vor : vors) {
    if(vor.isRange()) {
      const auto limits = vor.range(st);
      if(limits.first <= v && limits.second >= v)
        return true;
    } else if(abs(v - vor.value(st)) < Eps)
      return true;
  }
  return false;
}

string ValueSet::toString() const {
  ostringstream oss;
  oss<<'{';
  if( ! vors.empty()) {
    copy(CBOUNDS(vors), ostream_iterator<const ValueOrRange&>(oss, ", "));
    oss<<"\b\b";
  }
  oss<<'}';
  return oss.str();
}

NumericConst::NumericConst(double d) : NumericExpr() {val = d;}

double NumericConst::eval(const SymbolsTable &) const {return *val;}

string NumericConst::toString() const {
  return to_string(*val);
}

NumericVariable::NumericVariable(const string &varName) :
  NumericExpr(), name(varName) {}

bool NumericVariable::dependsOnVariable(const string &varName) const {
  return name.compare(varName) == 0;
}

double NumericVariable::eval(const SymbolsTable &st) const {return st.at(name);}

string NumericVariable::toString() const {
  return name;
}

Addition::Addition(const shared_ptr<const NumericExpr> &left_,
                   const shared_ptr<const NumericExpr> &right_) {
  VP(left_); VP(right_);

  // when both terms are constants, set the result directly
  if(left_->constValue() && right_->constValue()) {
    val = *(left_->constValue()) + *(right_->constValue());

  } else { // keep the terms only if they're not constants
    left = left_;
    right = right_;
  }
}

bool Addition::dependsOnVariable(const string &varName) const {
  return ! val &&
    (left->dependsOnVariable(varName) || right->dependsOnVariable(varName));
}

double Addition::eval(const SymbolsTable &st) const {
  if(val) // return the sum directly if it involves only constants
    return *val;
  return left->eval(st) + right->eval(st);
}

string Addition::toString() const {
  if(val)
    return to_string(*val);
  return "("s + left->toString() + " + "s + right->toString() + ")"s;
}

long Modulus::validLong(double v) {
  const long result = (long)v;
  if(abs(v - (double)result) > Eps)
    throw logic_error(string(__func__) +
      " - Operands of modulus need to be integer values!"s);
  return result;
}

long Modulus::validOperation(long numeratorL, long denominatorL) {
  if(0L == denominatorL) {
    if(0L != numeratorL)
      throw overflow_error(string(__func__) +
        " - denominator is 0!"s);
    throw logic_error(string(__func__) +
      " - both operands are 0!"s);
  }

  return numeratorL % denominatorL;
}

Modulus::Modulus(const shared_ptr<const NumericExpr> &numerator_,
                 const shared_ptr<const NumericExpr> &denominator_) {
  VP(numerator_); VP(denominator_);

  // when the denominator is 1 or -1, the result is always 0
  const bool hasConstDenominator = (bool)denominator_->constValue();
  if(hasConstDenominator && 1L == abs(validLong(*denominator_->constValue()))) {
    val = 0.;
    return;
  }

  const bool hasConstNumerator = (bool)numerator_->constValue();
  if(hasConstNumerator) {
    const long numeratorL = validLong(*(numerator_->constValue()));

    // when both terms are constants, set the result directly
    if(hasConstDenominator) {
      const long denominatorL = validLong(*(denominator_->constValue()));
      val = (double)validOperation(numeratorL, denominatorL);
      return;
    }
  }

  // keep the terms only if they're not constants
  numerator = numerator_;
  denominator = denominator_;
}

bool Modulus::dependsOnVariable(const string &varName) const {
  return ! val && (
    numerator->dependsOnVariable(varName) ||
    denominator->dependsOnVariable(varName));
}

double Modulus::eval(const SymbolsTable &st) const {
  if(val) // return the modulus directly if it involves only constants
    return *val;

  const long numeratorL = validLong(numerator->eval(st)),
    denominatorL = validLong(denominator->eval(st));
  return (double)validOperation(numeratorL, denominatorL);
}

string Modulus::toString() const {
  if(val)
    return to_string(*val);
  return "("s + numerator->toString() + " % "s + denominator->toString() + ")"s;
}

} // namespace cond
} // namespace rc

namespace std {

ostream& operator<<(ostream &os, const rc::cond::IConfigConstraint &c) {
  os<<c.toString();
  return os;
}

ostream& operator<<(ostream &os, const rc::cond::ConfigConstraints &c) {
  os<<c.toString();
  return os;
}

ostream& operator<<(ostream &os, const rc::cond::ConfigurationsTransferDuration &ctd) {
  os<<ctd.toString();
  return os;
}

ostream& operator<<(ostream &os, const rc::cond::ValueOrRange &vor) {
  os<<vor.toString();
  return os;
}

ostream& operator<<(ostream &os, const rc::cond::ValueSet &vs) {
  os<<vs.toString();
  return os;
}

} // namespace std

#ifdef UNIT_TESTING

/*
  This include allows recompiling only the Unit tests project when updating the tests.
  It also keeps the count of total code units to recompile to a minimum value.
*/
#define CONFIG_CONSTRAINT_CPP
#include "../test/configConstraint.hpp"

#endif // UNIT_TESTING defined
