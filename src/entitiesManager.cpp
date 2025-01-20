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
  This include allows recompiling only the Unit tests project when updating the
  tests. It also keeps the count of total code units to recompile to a minimum
  value.
*/
#define CPP_ENTITIES_MANAGER
#include "entitiesManager.hpp"
#undef CPP_ENTITIES_MANAGER

#endif  // UNIT_TESTING

#include "entitiesManager.h"
#include "entity.h"
#include "util.h"

#include <cassert>
#include <iterator>
#include <ranges>
#include <set>

using namespace std;
using namespace boost::property_tree;

namespace rc::ent {

bool IEntities::operator==(const set<unsigned>& ids_) const noexcept {
  return ranges::equal(ids(), ids_);
}

bool IEntities::operator==(const IEntities& other) const noexcept {
  return *this == other.ids();
}

AllEntities::AllEntities(const ptree& entTree) {
  if (!entTree.count(""))
    throw domain_error{
        HERE.function_name() +
        " - The entities section should be an array of 3 or more entities!"s};
  for (const auto& entPair : entTree)
    if (entPair.first.empty())
      operator+=(make_shared<const Entity>(entPair.second));
    else
      throw domain_error{
          HERE.function_name() +
          " - The entities section should be an array of 3 or more entities!"s};

  if (size(entities) < 3ULL)
    throw domain_error{HERE.function_name() +
                       " - Please specify at least 3 entities!"s};

  const bool anyEntitiesWithoutType{_idsByTypes.contains("")};
  const bool singleType{size(_idsByTypes) == 1ULL};
  if ((singleType && !anyEntitiesWithoutType) ||
      (!singleType && anyEntitiesWithoutType))
    throw domain_error{
        HERE.function_name() +
        " - Either none or all entities must have the type specified! "
        "When the type is specified, there must be at least 2 types!"s};

  const bool anyEntitiesWithoutWeight{_idsByWeight.contains(0.)};
  const bool singleWeight{size(_idsByWeight) == 1ULL};
  if (!singleWeight && anyEntitiesWithoutWeight)
    throw domain_error{
        HERE.function_name() +
        " - Either none or all entities must have the weight specified!"s};

  if (_idsStartingFromLeftBank.empty())
    throw domain_error{
        HERE.function_name() +
        " - By convention, the first river crossing starts from the left bank, "
        "but all provided entities are initially on the right bank!"s};

  /*
  Ensuring there is someone able to row on the left bank at the beginning.

  Still not checking whether any of those entities can be included in
  valid raft configurations (like not exceeding raft capacity / load).
  Leaving this thorough test for the actual solving algorithm.
  */
  const BankEntities leftBank{make_shared<const AllEntities>(*this),
                              idsStartingFromLeftBank()};
  if (!leftBank.anyRowCapableEnts(InitialSymbolsTable()))
    throw domain_error{
        HERE.function_name() +
        " - There is nobody able to row on the starting left bank!"s};
}

AllEntities& AllEntities::operator+=(const shared_ptr<const IEntity>& e) {
  assert(e);

  const IEntity& ent{*e};
  const unsigned id{ent.id()};
  const string name{ent.name()}, type{ent.type()};
  if (byId.contains(id))
    throw domain_error{HERE.function_name() + " - Duplicate entity id: "s +
                       to_string(id)};
  if (byName.contains(name))
    throw domain_error{HERE.function_name() + " - Duplicate entity name: `"s +
                       name + "`"s};

  byId[id] = byName[name] = e;
  _idsByTypes[type].insert(id);
  _idsByWeight[ent.weight()].insert(id);
  if (!ent.startsFromRightBank())
    _idsStartingFromLeftBank.push_back(id);
  else
    _idsStartingFromRightBank.push_back(id);

  entities.push_back(e);

  _ids.insert(id);

  return *this;
}

bool AllEntities::empty() const noexcept {
  return entities.empty();
}

size_t AllEntities::count() const noexcept {
  return size(entities);
}

const set<unsigned>& AllEntities::ids() const noexcept {
  return _ids;
}

const map<string, set<unsigned>>& AllEntities::idsByTypes() const noexcept {
  return _idsByTypes;
}

const map<double, set<unsigned>>& AllEntities::idsByWeights() const noexcept {
  return _idsByWeight;
}

shared_ptr<const IEntity> AllEntities::operator[](unsigned id) const {
  return byId.at(id);
}

const vector<unsigned>& AllEntities::idsStartingFromLeftBank() const noexcept {
  return _idsStartingFromLeftBank;
}

const vector<unsigned>& AllEntities::idsStartingFromRightBank() const noexcept {
  return _idsStartingFromRightBank;
}

string AllEntities::toString() const {
  ostringstream oss;
  oss << "Entities: [ " << **cbegin(entities);
  for (const shared_ptr<const IEntity>& ent : entities | views::drop(1))
    oss << ", " << *ent;
  oss << " ]";
  return oss.str();
}

IsolatedEntities::IsolatedEntities(const IsolatedEntities& other) noexcept
    : all{other.all}, _ids{other._ids}, byType{other.byType} {}

IsolatedEntities::IsolatedEntities(IsolatedEntities&& other) noexcept
    : all{other.all},
      _ids{std::move(other._ids)},
      byType{std::move(other.byType)} {}

IsolatedEntities& IsolatedEntities::operator=(const IsolatedEntities& other) {
  if (&other != this) {
    if (all.get() != other.all.get())
      throw logic_error{HERE.function_name() +
                        " - Don't assign a group that refers entities from a "
                        "different scenario!"s};
    _ids = other._ids;
    byType = other.byType;
  }
  return *this;
}
IsolatedEntities& IsolatedEntities::operator=(IsolatedEntities&& other) {
  if (&other != this) {
    if (all.get() != other.all.get())
      throw logic_error{HERE.function_name() +
                        " - Don't move assign a group that refers entities "
                        "from a different scenario!"s};
    _ids = std::move(other._ids);
    byType = std::move(other.byType);
  }
  return *this;
}

IsolatedEntities& IsolatedEntities::operator=(
    const initializer_list<unsigned>& ids_) {
  return operator=(vector(ids_));
}

void IsolatedEntities::clear() noexcept {
  _ids.clear();
  byType.clear();
}

IsolatedEntities& IsolatedEntities::operator+=(unsigned id) {
  shared_ptr<const IEntity> ent{(*all)[id]};
  if (!_ids.insert(id).second)
    throw domain_error{HERE.function_name() + " - Duplicate entity id: "s +
                       to_string(id)};

  byType[ent->type()].insert(id);

  return *this;
}

IsolatedEntities& IsolatedEntities::operator-=(unsigned id) {
  const string& entType{(*all)[id]->type()};
  if (!_ids.erase(id))
    throw domain_error{HERE.function_name() + " - Missing entity id: "s +
                       to_string(id)};

  set<unsigned>& forType{byType[entType]};
  forType.erase(id);
  if (forType.empty())
    byType.erase(entType);

  return *this;
}

bool IsolatedEntities::empty() const noexcept {
  return _ids.empty();
}

size_t IsolatedEntities::count() const noexcept {
  return size(_ids);
}

const set<unsigned>& IsolatedEntities::ids() const noexcept {
  return _ids;
}

const map<string, set<unsigned>>& IsolatedEntities::idsByTypes()
    const noexcept {
  return byType;
}

bool IsolatedEntities::anyRowCapableEnts(const SymbolsTable& st) const {
  for (const unsigned id : _ids)
    if ((*all)[id]->canRow(st))
      return true;
  return false;
}

string IsolatedEntities::toString() const {
  if (_ids.empty())
    return "[]"s;

  const auto streamNamePlusId = [this](ostream& os, unsigned id) {
    os << (*all)[id]->name() << '(' << id << ')';
  };

  ostringstream oss;
  oss << "[ ";
  streamNamePlusId(oss, *cbegin(_ids));
  for (const unsigned id : _ids | views::drop(1)) {
    oss << ", ";
    streamNamePlusId(oss, id);
  }
  oss << " ]";
  return oss.str();
}

unique_ptr<IMovingEntitiesExt> DefMovingEntitiesExt::clone() const noexcept {
  return make_unique<DefMovingEntitiesExt>();
}

AbsMovingEntitiesExt::AbsMovingEntitiesExt(
    const shared_ptr<const AllEntities>& all_,
    unique_ptr<IMovingEntitiesExt> nextExt_) noexcept
    : all{all_}, nextExt{std::move(nextExt_)} {
  Expects(nextExt);
}

void AbsMovingEntitiesExt::newGroup(const set<unsigned>& ids) {
  nextExt->newGroup(ids);
  _newGroup(ids);
}

void AbsMovingEntitiesExt::addEntity(unsigned id) {
  nextExt->addEntity(id);
  _addEntity(id);
}

void AbsMovingEntitiesExt::removeEntity(unsigned id) {
  nextExt->removeEntity(id);
  _removeEntity(id);
}

void AbsMovingEntitiesExt::addMovePostProcessing(
    SymbolsTable& SymTb) const noexcept {
  nextExt->addMovePostProcessing(SymTb);
  _addMovePostProcessing(SymTb);
}

void AbsMovingEntitiesExt::removeMovePostProcessing(
    SymbolsTable& SymTb) const noexcept {
  nextExt->removeMovePostProcessing(SymTb);
  _removeMovePostProcessing(SymTb);
}

unique_ptr<IMovingEntitiesExt> AbsMovingEntitiesExt::clone() const noexcept {
  return _clone(nextExt->clone());
}

string AbsMovingEntitiesExt::toString(
    bool suffixesInsteadOfPrefixes /* = true*/) const noexcept {
  // Only the matching extension categories will return non-empty strings
  // given suffixesInsteadOfPrefixes
  // (some display only as prefixes, the rest only as suffixes)
  return _toString(suffixesInsteadOfPrefixes) +
         nextExt->toString(suffixesInsteadOfPrefixes);
}

MovingEntities::MovingEntities(const MovingEntities& other) noexcept
    : IsolatedEntities{other}, extension{other.extension->clone()} {}

MovingEntities::MovingEntities(MovingEntities&& other) noexcept
    : IsolatedEntities{std::move(other)},
      extension{std::move(other.extension)} {}

MovingEntities& MovingEntities::operator=(const MovingEntities& other) {
  IsolatedEntities::operator=(other);
  extension = other.extension->clone();
  return *this;
}

MovingEntities& MovingEntities::operator=(MovingEntities&& other) noexcept(
    !UT) {
  IsolatedEntities::operator=(std::move(other));
  extension = std::move(other.extension);
  return *this;
}

MovingEntities& MovingEntities::operator=(
    const initializer_list<unsigned>& ids_) {
  return operator=(vector(ids_));
}

MovingEntities& MovingEntities::operator+=(unsigned id) {
  IsolatedEntities::operator+=(id);
  extension->addEntity(id);
  return *this;
}

MovingEntities& MovingEntities::operator-=(unsigned id) {
  IsolatedEntities::operator-=(id);
  extension->removeEntity(id);
  return *this;
}

void MovingEntities::clear() noexcept {
  IsolatedEntities::clear();
  extension->newGroup({});
}

string MovingEntities::toString() const {
  ostringstream oss;
  {
    ToStringManager<IMovingEntitiesExt> tsm{*extension,
                                            oss};  // extension wrapper
    oss << IsolatedEntities::toString();
  }  // ensures tsm's destructor flushes to oss before the return
  return oss.str();
}

BankEntities::BankEntities(const BankEntities& other) noexcept
    : IsolatedEntities{other} {}

BankEntities::BankEntities(BankEntities&& other) noexcept
    : IsolatedEntities{std::move(other)} {}

BankEntities& BankEntities::operator=(const BankEntities& other) {
  IsolatedEntities::operator=(other);
  return *this;
}

BankEntities& BankEntities::operator=(BankEntities&& other) noexcept(!UT) {
  IsolatedEntities::operator=(std::move(other));
  return *this;
}

BankEntities& BankEntities::operator=(const initializer_list<unsigned>& ids_) {
  IsolatedEntities::operator=(ids_);
  return *this;
}

BankEntities& BankEntities::operator+=(const MovingEntities& arrivedEnts) {
  for (const unsigned id : arrivedEnts.ids())
    IsolatedEntities::operator+=(id);
  return *this;
}

BankEntities& BankEntities::operator-=(const MovingEntities& leftEnts) {
  for (const unsigned id : leftEnts.ids())
    IsolatedEntities::operator-=(id);
  return *this;
}

BankEntities BankEntities::operator~() const noexcept {
  const set<unsigned> allIds{all->ids()}, theseIds{ids()};
  vector<unsigned> restIds;
  ranges::set_difference(allIds, theseIds, back_inserter(restIds));
  return BankEntities{all, restIds};
}

size_t BankEntities::differencesCount(
    const BankEntities& other) const noexcept {
  if (this == &other)
    return 0ULL;

  const set<unsigned> theseIds{ids()}, otherIds{other.ids()};
  vector<unsigned> diffIds;
  ranges::set_symmetric_difference(otherIds, theseIds, back_inserter(diffIds));
  return size(diffIds);
}

}  // namespace rc::ent

namespace std {

ostream& operator<<(ostream& os, const rc::ent::IEntities& ents) {
  os << ents.toString();
  return os;
}

}  // namespace std
