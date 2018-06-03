/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 @2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_ENTITIES_MANAGER
#define H_ENTITIES_MANAGER

#include "absEntity.h"
#include "util.h"

#include <memory>
#include <set>
#include <map>
#include <unordered_map>
#include <vector>
#include <cassert>

#include <boost/property_tree/ptree.hpp>

namespace rc {
namespace ent {

/// Common interface for handling a set of entities
struct IEntities /*abstract*/ {
  virtual ~IEntities() /*= 0*/ {}

  virtual bool empty() const = 0; ///< is this set empty?
  virtual size_t count() const = 0; ///< size of this set

  /// ordered sequence of id-s of the entities from the set
  virtual const std::set<unsigned>& ids() const = 0;

  /// ids of the entities from the set grouped by entity-type
  virtual const std::map<std::string, std::set<unsigned>>& idsByTypes() const = 0;

  /// Compares these ids with a provided set of ids
  bool operator==(const std::set<unsigned> &ids_) const;
  bool operator!=(const std::set<unsigned> &ids_) const;

  /// Compares this subset against another
  bool operator==(const IEntities &other) const;
  bool operator!=(const IEntities &other) const;

  virtual std::string toString() const = 0;
};

/// Manager of all the entities from the scenario
class AllEntities : public IEntities {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
  AllEntities() {}

    #else // keep fields protected otherwise
protected:
    #endif

  std::vector<std::shared_ptr<const IEntity>> entities; ///< the entire entities set

  std::set<unsigned> _ids; ///< id-s for all entities
  std::map<std::string, std::set<unsigned>> _idsByTypes; ///< id-s grouped by type
  std::map<double, std::set<unsigned>> _idsByWeight; ///< id-s grouped by weight

  /// id-s of entities starting on the left bank
  std::vector<unsigned> _idsStartingFromLeftBank;

  /// id-s of entities starting on the right bank
  std::vector<unsigned> _idsStartingFromRightBank;

  // Helper fields
  std::unordered_map<unsigned, std::shared_ptr<const IEntity>> byId;
  std::unordered_map<std::string, std::shared_ptr<const IEntity>> byName;

  AllEntities& operator+=(const std::shared_ptr<const IEntity> &e);

public:
  AllEntities(const boost::property_tree::ptree &entTree);

  bool empty() const override; ///< are there any entities?
  size_t count() const override; ///< total number of entities

  const std::set<unsigned>& ids() const override; ///< id-s for all entities

  /// @return the id-s grouped by type
  const std::map<std::string, std::set<unsigned>>& idsByTypes() const override;

  /// @return the id-s grouped by weight
  const std::map<double, std::set<unsigned>>& idsByWeights() const;

  /// @return the entity with the given id, or throws out_of_range if the id is unknown
  std::shared_ptr<const IEntity> operator[](unsigned id) const;

  /// @return the id-s of entities starting on the left bank
  const std::vector<unsigned>& idsStartingFromLeftBank() const;

  /// @return the id-s of entities starting on the right bank
  const std::vector<unsigned>& idsStartingFromRightBank() const;

  std::string toString() const override;
};

/// Entities either from a bank or performing the river crossing
class IsolatedEntities /*abstract*/ : public IEntities {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  std::shared_ptr<const AllEntities> all; ///< all entities from the scenario
  std::set<unsigned> _ids; ///< the ids of a subset of all entities

  /// the subset of entities ids grouped by type
  std::map<std::string, std::set<unsigned>> byType;

  template<class IdsCont = std::vector<unsigned>>
  IsolatedEntities(const std::shared_ptr<const AllEntities> &all_,
                   const IdsCont &ids_ = {}) :
      all(VP(all_)) {
    for(const unsigned id : ids_)
      operator+=(id);
  }

public:
  IsolatedEntities(const IsolatedEntities &other);
  IsolatedEntities(IsolatedEntities &&other);
  IsolatedEntities& operator=(const IsolatedEntities &other);
  IsolatedEntities& operator=(IsolatedEntities &&other);

  /// @return a new subset with the provided ids_ from the original pool
  template<class IdsCont, typename IdsCont::const_iterator** = nullptr>
  IsolatedEntities& operator=(const IdsCont &ids_) {
    clear();
    for(const unsigned id : ids_)
      operator+=(id);
    return *this;
  }

  void clear(); ///< empties the subset of ids. Leaves the choice pool in place

  /// Appends to the subset the entity with the given id
  IsolatedEntities& operator+=(unsigned id);

  /// Removes from the subset the entity with the given id
  IsolatedEntities& operator-=(unsigned id);

  bool empty() const override; ///< is this set empty?
  size_t count() const override; ///< count of entities from this subset of the pool

  const std::set<unsigned>& ids() const override; ///< the ids of a subset of all entities

  /// @return the id-s subset grouped by type
  const std::map<std::string, std::set<unsigned>>& idsByTypes() const override;

  /// Are there any entities capable to row within the context specified by st?
  bool anyRowCapableEnts(const SymbolsTable &st) const;

  std::string toString() const override;
};

/// Entities traversing the river on the raft / over the bridge
class MovingEntities : public IsolatedEntities {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

public:
  template<class IdsCont = std::vector<unsigned>>
  MovingEntities(const std::shared_ptr<const AllEntities> &all_,
                 const IdsCont &ids_ = {}) :
      IsolatedEntities(all_, ids_) {}

  MovingEntities(const MovingEntities &other);
  MovingEntities(MovingEntities &&other);
  MovingEntities& operator=(const MovingEntities &other);
  MovingEntities& operator=(MovingEntities &&other);

  /// @return a new subset with the provided ids_ from the original pool
  template<class IdsCont>
  MovingEntities& operator=(const IdsCont &ids_) {
    IsolatedEntities::operator=(ids_);
    return *this;
  }

  double weight() const; ///< total weight of this subset of entities

  std::string toString() const override;
};

/// Entities from either bank
class BankEntities : public IsolatedEntities {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

public:
  template<class IdsCont = std::vector<unsigned>>
  BankEntities(const std::shared_ptr<const AllEntities> &all_,
               const IdsCont &ids_ = {}) :
      IsolatedEntities(all_, ids_) {}

  BankEntities(const BankEntities &other);
  BankEntities(BankEntities &&other);
  BankEntities& operator=(const BankEntities &other);
  BankEntities& operator=(BankEntities &&other);

  /// @return a new subset with the provided ids_ from the original pool
  template<class IdsCont>
  BankEntities& operator=(const IdsCont &ids_) {
    IsolatedEntities::operator=(ids_);
    return *this;
  }

  /// @return the updated subset which contains now also arrivedEnts
  BankEntities& operator+=(const MovingEntities &arrivedEnts);

  /// @return the updated subset from which leftEnts were removed
  BankEntities& operator-=(const MovingEntities &leftEnts);

  /// @return the complementary subset of entities from the pool
  BankEntities operator~() const;

  /// @return size of symmetric difference between these and other's ids
  size_t differencesCount(const BankEntities &other) const;
};

} // namespace ent
} // namespace rc

namespace std {

std::ostream& operator<<(std::ostream &os, const rc::ent::IEntities &ents);

} // namespace std

#endif // H_ENTITIES_MANAGER not defined
