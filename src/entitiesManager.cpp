/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 @2018 Florin Tulba (florintulba@yahoo.com)
*/

#include "entitiesManager.h"
#include "entity.h"
#include "util.h"

#include <cassert>
#include <set>
#include <iterator>
#include <algorithm>
#include <numeric>

using namespace std;
using namespace boost::property_tree;

namespace rc {
namespace ent {

bool IEntities::operator==(const set<unsigned> &ids_) const {
  return equal(CBOUNDS(ids()), CBOUNDS(ids_));
}

bool IEntities::operator!=(const set<unsigned> &ids_) const {
  return ! (*this == ids_);
}

bool IEntities::operator==(const IEntities &other) const {
  return *this == other.ids();
}

bool IEntities::operator!=(const IEntities &other) const {
  return ! (*this == other);
}

AllEntities::AllEntities(const ptree &entTree) {
  if(entTree.count("") == 0ULL)
    throw domain_error(string(__func__) +
      " - The entities section should be an array of 3 or more entities!");
  for(const auto &entPair : entTree)
    if(entPair.first.empty())
      operator+=(make_shared<const Entity>(entPair.second));
    else
      throw domain_error(string(__func__) +
        " - The entities section should be an array of 3 or more entities!");

  if(entities.size() < 3ULL)
    throw domain_error(string(__func__) + " - Please specify at least 3 entities!");

  const bool anyEntitiesWithoutType = (_idsByTypes.find("") != _idsByTypes.cend());
  const bool singleType = (_idsByTypes.size() == 1ULL);
  if((singleType && ! anyEntitiesWithoutType) ||
      ( ! singleType && anyEntitiesWithoutType))
    throw domain_error(string(__func__) +
      " - Either none or all entities must have the type specified! "
      "When the type is specified, there must be at least 2 types!");

  const bool anyEntitiesWithoutWeight = (_idsByWeight.find(0.) != _idsByWeight.cend());
  const bool singleWeight = (_idsByWeight.size() == 1ULL);
  if( ! singleWeight && anyEntitiesWithoutWeight)
    throw domain_error(string(__func__) +
      " - Either none or all entities must have the weight specified!");

  if(_idsStartingFromLeftBank.empty())
    throw domain_error(string(__func__) +
      " - By convention, the first river crossing starts from the left bank, "
      "but all provided entities are initially on the right bank!");

  /*
  Ensuring there is someone able to row on the left bank at the beginning.

  Still not checking whether any of those entities can be included in
  valid raft configurations (like not exceeding raft capacity / load).
  Leaving this thorough test for the actual solving algorithm.
  */
  const BankEntities leftBank(make_shared<const AllEntities>(*this),
                              idsStartingFromLeftBank());
  if( ! leftBank.anyRowCapableEnts(InitialSymbolsTable))
    throw domain_error(string(__func__) +
      " - There is nobody able to row on the starting left bank!");
}

AllEntities& AllEntities::operator+=(const shared_ptr<const IEntity> &e) {
  assert(nullptr != e);

  const IEntity &ent = *e;
  const unsigned id = ent.id();
  const string name = ent.name(), type = ent.type();
  if(byId.find(id) != byId.cend())
    throw domain_error(string(__func__) +
      " - Duplicate entity id: "s + to_string(id));
  if(byName.find(name) != byName.cend())
    throw domain_error(string(__func__) +
      " - Duplicate entity name: `"s + name + "`"s);

  byId[id] = byName[name] = e;
  _idsByTypes[type].insert(id);
  _idsByWeight[ent.weight()].insert(id);
  if( ! ent.startsFromRightBank())
    _idsStartingFromLeftBank.push_back(id);
  else
    _idsStartingFromRightBank.push_back(id);

  entities.push_back(e);

  _ids.insert(id);

  return *this;
}

bool AllEntities::empty() const {
  return entities.empty();
}

size_t AllEntities::count() const {
  return entities.size();
}

const set<unsigned>& AllEntities::ids() const {
  return _ids;
}

const map<string, set<unsigned>>& AllEntities::idsByTypes() const {
  return _idsByTypes;
}

const map<double, set<unsigned>>& AllEntities::idsByWeights() const {
  return _idsByWeight;
}

shared_ptr<const IEntity> AllEntities::operator[](unsigned id) const {
  return byId.at(id);
}

const vector<unsigned>& AllEntities::idsStartingFromLeftBank() const {
  return _idsStartingFromLeftBank;
}

const vector<unsigned>& AllEntities::idsStartingFromRightBank() const {
  return _idsStartingFromRightBank;
}

string AllEntities::toString() const {
  ostringstream oss;
  oss<<"Entities: [ ";
  for(const shared_ptr<const IEntity> &ent : entities)
    oss<<*ent<<", ";
  oss<<"\b\b ]";
  return oss.str();
}

IsolatedEntities::IsolatedEntities(const IsolatedEntities &other) :
    all(other.all), _ids(other._ids), byType(other.byType) {}

IsolatedEntities::IsolatedEntities(IsolatedEntities &&other) :
    all(other.all), _ids(move(other._ids)), byType(move(other.byType)) {
  other.clear();
}

IsolatedEntities& IsolatedEntities::operator=(const IsolatedEntities &other) {
  if(&other != this) {
    if(all.get() != other.all.get())
      throw logic_error(string(__func__) +
        " - Don't assign a group that refers entities from a different scenario!");
    _ids = other._ids;
    byType = other.byType;
  }
  return *this;
}
IsolatedEntities& IsolatedEntities::operator=(IsolatedEntities &&other) {
  if(&other != this) {
    if(all.get() != other.all.get())
      throw logic_error(string(__func__) +
        " - Don't move assign a group that refers entities from a different scenario!");
    _ids = move(other._ids);
    byType = move(other.byType);
  }
  return *this;
}

void IsolatedEntities::clear() {
  _ids.clear();
  byType.clear();
}

IsolatedEntities& IsolatedEntities::operator+=(unsigned id) {
  shared_ptr<const IEntity> ent = (*all)[id];
  if( ! _ids.insert(id).second)
    throw domain_error(string(__func__) +
      " - Duplicate entity id: "s + to_string(id));

  byType[ent->type()].insert(id);

  return *this;
}

IsolatedEntities& IsolatedEntities::operator-=(unsigned id) {
  const string &entType = (*all)[id]->type();
  if(_ids.erase(id) == 0ULL)
    throw domain_error(string(__func__) +
      " - Missing entity id: "s + to_string(id));

  set<unsigned> &forType = byType[entType];
  forType.erase(id);
  if(forType.empty())
    byType.erase(entType);

  return *this;
}

bool IsolatedEntities::empty() const {
  return _ids.empty();
}

size_t IsolatedEntities::count() const {
  return _ids.size();
}

const set<unsigned>& IsolatedEntities::ids() const {
  return _ids;
}

const map<string, set<unsigned>>& IsolatedEntities::idsByTypes() const {
  return byType;
}

bool IsolatedEntities::anyRowCapableEnts(const SymbolsTable &st) const {
  for(unsigned id : _ids)
    if((*all)[id]->canRow(st))
      return true;
  return false;
}

string IsolatedEntities::toString() const {
  if(_ids.empty())
    return "[]";
  ostringstream oss;
  oss<<"[ ";
  for(const unsigned id : _ids)
    oss<<(*all)[id]->name()<<'('<<id<<"), ";
  oss<<"\b\b ]";
  return oss.str();
}

MovingEntities::MovingEntities(const MovingEntities &other) : IsolatedEntities(other) {}

MovingEntities::MovingEntities(MovingEntities &&other) :
  IsolatedEntities(std::move(other)) {}

MovingEntities& MovingEntities::operator=(const MovingEntities &other) {
  IsolatedEntities::operator=(other);
  return *this;
}

MovingEntities& MovingEntities::operator=(MovingEntities &&other) {
  IsolatedEntities::operator=(std::move(other));
  return *this;
}

double MovingEntities::weight() const {
  return accumulate(CBOUNDS(_ids), 0.,
    [this](double prevSum, unsigned id) {
      return prevSum + (*all)[id]->weight();
    });
}

string MovingEntities::toString() const {
  ostringstream oss;
  oss<<IsolatedEntities::toString();
  const double load = weight();
  if(load > 0.)
    oss<<" weighing "<<load;
  return oss.str();
}

BankEntities::BankEntities(const BankEntities &other) : IsolatedEntities(other) {}

BankEntities::BankEntities(BankEntities &&other) :
  IsolatedEntities(std::move(other)) {}

BankEntities& BankEntities::operator=(const BankEntities &other) {
  IsolatedEntities::operator=(other);
  return *this;
}

BankEntities& BankEntities::operator=(BankEntities &&other) {
  IsolatedEntities::operator=(std::move(other));
  return *this;
}

BankEntities& BankEntities::operator+=(const MovingEntities &arrivedEnts) {
  for(const unsigned id : arrivedEnts.ids())
    IsolatedEntities::operator+=(id);
  return *this;
}

BankEntities& BankEntities::operator-=(const MovingEntities &leftEnts) {
  for(const unsigned id : leftEnts.ids())
    IsolatedEntities::operator-=(id);
  return *this;
}

BankEntities BankEntities::operator~() const {
  const set<unsigned> allIds = all->ids(), theseIds = ids();
  vector<unsigned> restIds;
  set_difference(CBOUNDS(allIds), CBOUNDS(theseIds), back_inserter(restIds));
  return BankEntities(all, restIds);
}

size_t BankEntities::differencesCount(const BankEntities &other) const {
  if(this == &other)
    return 0ULL;

  const set<unsigned> theseIds = ids(), otherIds = other.ids();
  vector<unsigned> diffIds;
  set_symmetric_difference(CBOUNDS(otherIds), CBOUNDS(theseIds),
                           back_inserter(diffIds));
  return diffIds.size();
}

} // namespace ent
} // namespace rc

namespace std {

ostream& operator<<(ostream &os, const rc::ent::IEntities &ents) {
  os<<ents.toString();
  return os;
}

} // namespace std

#ifdef UNIT_TESTING

/*
  This include allows recompiling only the Unit tests project when updating the tests.
  It also keeps the count of total code units to recompile to a minimum value.
*/
#define ENTITIES_MANAGER_CPP
#include "../test/entitiesManager.hpp"

#endif // UNIT_TESTING
