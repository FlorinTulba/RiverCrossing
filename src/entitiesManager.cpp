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
// NOLINTNEXTLINE(misc-include-cleaner) : Contains tests
#include "entitiesManager.hpp"  // IWYU pragma: keep

#endif  // UNIT_TESTING

#include "absEntity.h"
#include "entitiesManager.h"
#include "entity.h"
#include "symbolsTable.h"
#include "util.h"

#include <cassert>
#include <cstddef>

#include <algorithm>
#include <format>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <gsl/assert>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>


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
  if (entTree.count("") == 0ULL)
    throw domain_error{format(
        "{} - The entities section should be an array of 3 or more entities!",
        HERE.function_name())};
  for (const auto& entPair : entTree)
    if (entPair.first.empty())
      operator+=(make_shared<const Entity>(entPair.second));
    else
      throw domain_error{format(
          "{} - The entities section should be an array of 3 or more entities!",
          HERE.function_name())};

  if (size(entities) < 3ULL)
    throw domain_error{format("{} - Please specify at least 3 entities!",
                              HERE.function_name())};

  const bool anyEntitiesWithoutType{_idsByTypes.contains("")};
  const bool singleType{size(_idsByTypes) == 1ULL};
  if ((singleType && !anyEntitiesWithoutType) ||
      (!singleType && anyEntitiesWithoutType))
    throw domain_error{
        format("{} - Either none or all entities must have the type specified! "
               "When the type is specified, there must be at least 2 types!",
               HERE.function_name())};

  const bool anyEntitiesWithoutWeight{_idsByWeight.contains(0.)};
  const bool singleWeight{size(_idsByWeight) == 1ULL};
  if (!singleWeight && anyEntitiesWithoutWeight)
    throw domain_error{format(
        "{} - Either none or all entities must have the weight specified!",
        HERE.function_name())};

  if (_idsStartingFromLeftBank.empty())
    throw domain_error{
        format("{} - By convention, the first river crossing starts from the "
               "left bank, "
               "but all provided entities are initially on the right bank!",
               HERE.function_name())};

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
        format("{} - There is nobody able to row on the starting left bank!",
               HERE.function_name())};
}

AllEntities& AllEntities::operator+=(const shared_ptr<const IEntity>& e) {
  assert(e);

  const IEntity& ent{*e};
  const unsigned id{ent.id()};
  const string name{ent.name()}, type{ent.type()};
  if (byId.contains(id))
    throw domain_error{
        format("{} - Duplicate entity id: {}", HERE.function_name(), id)};
  if (byName.contains(name))
    throw domain_error{
        format("{} - Duplicate entity name: `{}`", HERE.function_name(), name)};

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

void AllEntities::formatTo(FmtCtxIt& outIt) const {
  outIt = format_to(
      outIt, "Entities: [ {} ]",
      ContView{entities, ", ", [](const auto& pEnt) noexcept -> const IEntity& {
                 return *pEnt;
               }});
}

bool IsolatedEntities::refersToSameScenario(
    const IsolatedEntities& other) const noexcept {
  return all.get() == other.all.get();
}

IsolatedEntities::IsolatedEntities(const IsolatedEntities& other) noexcept
    : all{other.all},
      _ids{makeCopyNoexcept(other._ids)},
      byType{makeCopyNoexcept(other.byType)} {}

IsolatedEntities::IsolatedEntities(IsolatedEntities&& other) noexcept
    : all{other.all},
      _ids{makeMoveNoexcept(std::move(other._ids))},
      byType{makeMoveNoexcept(std::move(other.byType))} {}

IsolatedEntities& IsolatedEntities::operator=(const IsolatedEntities& other) {
  if (&other != this) {
    if (!refersToSameScenario(other))
      throw logic_error{
          format("{} - Don't assign a group that refers entities from a "
                 "different scenario!",
                 HERE.function_name())};
    _ids = other._ids;
    byType = other.byType;
  }
  return *this;
}

IsolatedEntities& IsolatedEntities::operator=(
    const initializer_list<unsigned>& ids_) {
  operator=(vector(ids_));
  return *this;
}

void IsolatedEntities::clear() noexcept {
  _ids.clear();
  byType.clear();
}

IsolatedEntities& IsolatedEntities::operator+=(unsigned id) {
  shared_ptr<const IEntity> ent{
      (*all)
          [id]};  // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                  // : Checked [] - uses at()
  if (!_ids.insert(id).second)
    throw domain_error{
        format("{} - Duplicate entity id: {}", HERE.function_name(), id)};

  byType[ent->type()].insert(id);

  return *this;
}

IsolatedEntities& IsolatedEntities::operator-=(unsigned id) {
  const string& entType{
      (*all)
          [id]  // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                // : Checked [] - uses at()
              ->type()};
  if (_ids.erase(id) == 0ULL)
    throw domain_error{
        format("{} - Missing entity id: {}", HERE.function_name(), id)};

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
  return ranges::any_of(_ids, [this, &st](unsigned id) {
    return (*all)
        [id]  // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
              // : Checked [] - uses at()
            ->canRow(st);
  });
}

void IsolatedEntities::formatTo(FmtCtxIt& outIt) const {
  if (_ids.empty()) {
    outIt = format_to(outIt, "[]");
    return;
  }

  outIt = format_to(
      outIt, "[ {} ]",
      ContView{
          _ids, ", ", [this](unsigned id) {
            return format(
                "{}({})",
                (*all)
                    [id]  // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                          // : Checked [] - uses at()
                        ->name(),
                id);
          }});
}

unique_ptr<IMovingEntitiesExt> DefMovingEntitiesExt::clone() const noexcept {
  return make_unique<DefMovingEntitiesExt>();
}

AbsMovingEntitiesExt::AbsMovingEntitiesExt(
    const shared_ptr<const AllEntities>& all_,
    unique_ptr<IMovingEntitiesExt> nextExt_) noexcept
    : all{all_},
      nextExt{std::move(nextExt_)} {
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

void AbsMovingEntitiesExt::formatPrefixTo(FmtCtxIt& outIt) const {
  _formatPrefixTo(outIt);
  nextExt->formatPrefixTo(outIt);
}

void AbsMovingEntitiesExt::formatSuffixTo(FmtCtxIt& outIt) const {
  _formatSuffixTo(outIt);
  nextExt->formatSuffixTo(outIt);
}

MovingEntities::MovingEntities(const MovingEntities& other) noexcept
    : IsolatedEntities{other},
      extension{other.extension->clone()} {}

MovingEntities& MovingEntities::operator=(const MovingEntities& other) {
  if (&other != this) {
    IsolatedEntities::operator=(other);
    extension = other.extension->clone();
  }
  return *this;
}

MovingEntities& MovingEntities::operator=(
    const initializer_list<unsigned>& ids_) {
  operator=(vector(ids_));
  return *this;
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
  makeNoexcept([this] { extension->newGroup({}); });
}

void MovingEntities::formatTo(FmtCtxIt& outIt) const {
  InfoWrapper iw{*extension, outIt};  // extension wrapper
  IsolatedEntities::formatTo(outIt);
  // the dtor of iw uses & updates outIt
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
  return makeNoexcept([this] {
    vector<unsigned> restIds;
    ranges::set_difference(all->ids(), ids(), back_inserter(restIds));
    return BankEntities{all, restIds};
  });
}

size_t BankEntities::differencesCount(
    const BankEntities& other) const noexcept {
  if (this == &other)
    return 0ULL;

  return makeNoexcept([this, &other] {
    vector<unsigned> diffIds;
    ranges::set_symmetric_difference(other.ids(), ids(),
                                     back_inserter(diffIds));
    return size(diffIds);
  });
}

}  // namespace rc::ent
