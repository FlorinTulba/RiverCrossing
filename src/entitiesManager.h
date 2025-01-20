/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_ENTITIES_MANAGER
#define H_ENTITIES_MANAGER

#include "absEntity.h"
#include "util.h"

#include <concepts>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include <boost/property_tree/ptree.hpp>

namespace rc::ent {

/// Common interface for handling a set of entities
class IEntities {
 public:
  virtual ~IEntities() noexcept = default;

  /// Is this set empty?
  [[nodiscard]] virtual bool empty() const noexcept = 0;

  /// Size of this set
  [[nodiscard]] virtual size_t count() const noexcept = 0;

  /// ordered sequence of id-s of the entities from the set
  [[nodiscard]] virtual const std::set<unsigned>& ids() const noexcept = 0;

  /// ids of the entities from the set grouped by entity-type
  [[nodiscard]] virtual const std::map<std::string, std::set<unsigned>>&
  idsByTypes() const noexcept = 0;

  /// Compares these ids with a provided set of ids
  [[nodiscard]] bool operator==(const std::set<unsigned>& ids_) const noexcept;

  /// Compares this subset against another
  [[nodiscard]] bool operator==(const IEntities& other) const noexcept;

  [[nodiscard]] virtual std::string toString() const = 0;

 protected:
  IEntities() noexcept = default;

  IEntities(const IEntities&) = default;
  IEntities(IEntities&&) noexcept = default;
  IEntities& operator=(const IEntities&) = default;
  IEntities& operator=(IEntities&&) noexcept = default;
};

/// Manager of all the entities from the scenario
class AllEntities : public IEntities {
 public:
#ifdef UNIT_TESTING  // Unit tests finds useful a default ctor
  AllEntities() noexcept = default;
#endif  // UNIT_TESTING

  explicit AllEntities(const boost::property_tree::ptree& entTree);

  /// Are there any entities?
  [[nodiscard]] bool empty() const noexcept override;

  /// Total number of entities
  [[nodiscard]] size_t count() const noexcept override;

  /// id-s for all entities
  [[nodiscard]] const std::set<unsigned>& ids() const noexcept override;

  /// @return the id-s grouped by type
  [[nodiscard]] const std::map<std::string, std::set<unsigned>>& idsByTypes()
      const noexcept override;

  /// @return the id-s grouped by weight
  [[nodiscard]] const std::map<double, std::set<unsigned>>& idsByWeights()
      const noexcept;

  /// @return the entity with the given id, or throws out_of_range if the id is
  /// unknown
  [[nodiscard]] std::shared_ptr<const IEntity> operator[](unsigned id) const;

  /// @return the id-s of entities starting on the left bank
  [[nodiscard]] const std::vector<unsigned>& idsStartingFromLeftBank()
      const noexcept;

  /// @return the id-s of entities starting on the right bank
  [[nodiscard]] const std::vector<unsigned>& idsStartingFromRightBank()
      const noexcept;

  [[nodiscard]] std::string toString() const override;

  PROTECTED :

      AllEntities&
      operator+=(const std::shared_ptr<const IEntity>& e);

  /// The entire entities set
  std::vector<std::shared_ptr<const IEntity>> entities;

  std::set<unsigned> _ids;  ///< id-s for all entities

  /// Id-s grouped by type
  std::map<std::string, std::set<unsigned>> _idsByTypes;

  /// Id-s grouped by weight
  std::map<double, std::set<unsigned>> _idsByWeight;

  /// id-s of entities starting on the left bank
  std::vector<unsigned> _idsStartingFromLeftBank;

  /// id-s of entities starting on the right bank
  std::vector<unsigned> _idsStartingFromRightBank;

  // Helper fields
  std::unordered_map<unsigned, std::shared_ptr<const IEntity>> byId;
  std::unordered_map<std::string, std::shared_ptr<const IEntity>> byName;
};

/// Entities either from a bank or performing the river crossing
class IsolatedEntities : public IEntities {
 public:
  IsolatedEntities(const IsolatedEntities& other) noexcept;
  IsolatedEntities(IsolatedEntities&& other) noexcept;
  IsolatedEntities& operator=(const IsolatedEntities& other);

  /// @throws logic_error if entities to be moved are from a different scenario
  IsolatedEntities& operator=(IsolatedEntities&& other);

  /// @return a new subset with the provided ids_ from the original pool
  template <class IdsCont>
    requires requires { typename IdsCont::const_iterator; }
  IsolatedEntities& operator=(const IdsCont& ids_) {
    clear();
    for (const unsigned id : ids_)
      operator+=(id);
    return *this;
  }

  /// @return a new subset with the provided ids_ from the original pool
  IsolatedEntities& operator=(const std::initializer_list<unsigned>& ids_);

  ~IsolatedEntities() noexcept override = default;

  /// Empties the subset of ids. Leaves the choice pool in place
  virtual void clear() noexcept;

  /// Appends to the subset the entity with the given id
  IsolatedEntities& operator+=(unsigned id);

  /// Removes from the subset the entity with the given id
  IsolatedEntities& operator-=(unsigned id);

  [[nodiscard]] bool empty() const noexcept override;  ///< is this set empty?

  /// Count of entities from this subset of the pool
  [[nodiscard]] size_t count() const noexcept override;

  /// The ids of a subset of all entities
  [[nodiscard]] const std::set<unsigned>& ids() const noexcept override;

  /// @return the id-s subset grouped by type
  [[nodiscard]] const std::map<std::string, std::set<unsigned>>& idsByTypes()
      const noexcept override;

  /// Are there any entities capable to row within the context specified by st?
  [[nodiscard]] bool anyRowCapableEnts(const SymbolsTable& st) const;

  [[nodiscard]] std::string toString() const override;

  PROTECTED :

      template <class IdsCont = std::vector<unsigned>>
      explicit IsolatedEntities(const std::shared_ptr<const AllEntities>& all_,
                                const IdsCont& ids_ = {})
      : all(all_) {
    for (const unsigned id : ids_)
      operator+=(id);
  }

  /// All entities from the scenario
  gsl::not_null<std::shared_ptr<const AllEntities>> all;

  std::set<unsigned> _ids;  ///< the ids of a subset of all entities

  /// the subset of entities ids grouped by type
  std::map<std::string, std::set<unsigned>> byType;
};

/// Interface for the extensions for each group of entities moving to the other
/// bank
class IMovingEntitiesExt {
 public:
  virtual ~IMovingEntitiesExt() noexcept = default;

  IMovingEntitiesExt(const IMovingEntitiesExt&) = delete;
  IMovingEntitiesExt(IMovingEntitiesExt&&) = delete;
  void operator=(const IMovingEntitiesExt&) = delete;
  void operator=(IMovingEntitiesExt&&) = delete;

  /// Selecting a new group of entities for moving to the other bank
  virtual void newGroup(const std::set<unsigned>&) = 0;

  /// Adds a new entity to the group from the raft/bridge.
  /// @throw out_of_range for invalid id
  virtual void addEntity(unsigned /*id*/) {}

  /// Removes an existing entity from the raft/bridge
  /// @throw out_of_range for invalid id
  virtual void removeEntity(unsigned /*id*/) {}

  /**
  Some extensions might want to change the content of the Symbols Table
  after a move is performed
  */
  virtual void addMovePostProcessing(SymbolsTable&) const noexcept {}

  /**
  Some extensions might want to change the content of the Symbols Table
  after a move is removed
  */
  virtual void removeMovePostProcessing(SymbolsTable&) const noexcept {}

  /// @return a clone of these extensions
  [[nodiscard]] virtual std::unique_ptr<IMovingEntitiesExt> clone()
      const noexcept = 0;

  /**
  Display either only suffix (most of them), or only prefix extensions.
  It needs to be called before (with param false) and after (with param true)
  the information of the moving entities
  */
  [[nodiscard]] virtual std::string toString(bool = true) const noexcept = 0;

 protected:
  IMovingEntitiesExt() noexcept = default;
};

/// Neutral MovingEntities extension
class DefMovingEntitiesExt final : public IMovingEntitiesExt {
 public:
  DefMovingEntitiesExt() noexcept = default;
  ~DefMovingEntitiesExt() noexcept override = default;

  DefMovingEntitiesExt(const DefMovingEntitiesExt&) = delete;
  DefMovingEntitiesExt(DefMovingEntitiesExt&&) = delete;
  void operator=(const DefMovingEntitiesExt&) = delete;
  void operator=(DefMovingEntitiesExt&&) = delete;

  /// Selecting a new group of entities for moving to the other bank
  void newGroup(const std::set<unsigned>&) noexcept final {}

  /// @return a clone of these extensions
  [[nodiscard]] std::unique_ptr<IMovingEntitiesExt> clone()
      const noexcept final;

  /**
  Display either only suffix (most of them), or only prefix extensions.
  It needs to be called before (with param false) and after (with param true)
  the information of the moving entities
  */
  [[nodiscard]] std::string toString(bool = true) const noexcept final {
    return {};
  }
};

/**
Base class for extensions of moving entities.
Some of the new virtual methods are abstract and must be implemented
by every derived class.
*/
class AbsMovingEntitiesExt
    : public IMovingEntitiesExt,

      // provides `selectExt` static method
      public rc::DecoratorManager<AbsMovingEntitiesExt, IMovingEntitiesExt> {
  // for accessing nextExt
  friend class rc::DecoratorManager<AbsMovingEntitiesExt, IMovingEntitiesExt>;

 public:
  AbsMovingEntitiesExt(const AbsMovingEntitiesExt&) = delete;
  AbsMovingEntitiesExt(AbsMovingEntitiesExt&&) = delete;
  void operator=(const AbsMovingEntitiesExt&) = delete;
  void operator=(AbsMovingEntitiesExt&&) = delete;

  /// Selecting a new group of entities for moving to the other bank
  void newGroup(const std::set<unsigned>& ids) final;

  /// Adds a new entity to the group from the raft/bridge
  /// @throw out_of_range for invalid id
  void addEntity(unsigned id) final;

  /// Removes an existing entity from the raft/bridge
  /// @throw out_of_range for invalid id
  void removeEntity(unsigned id) final;

  /**
  Some extensions might want to change the content of the Symbols Table
  after a move is performed
  */
  void addMovePostProcessing(SymbolsTable& SymTb) const noexcept final;

  /**
  Some extensions might want to change the content of the Symbols Table
  after a move is removed
  */
  void removeMovePostProcessing(SymbolsTable& SymTb) const noexcept final;

  /// @return a clone of these extensions
  [[nodiscard]] std::unique_ptr<IMovingEntitiesExt> clone()
      const noexcept final;

  /**
  Display either only suffix (most of them), or only prefix extensions.
  It needs to be called before (with param false) and after (with param true)
  the information of the moving entities
  */
  [[nodiscard]] std::string toString(
      bool suffixesInsteadOfPrefixes /* = true*/) const noexcept final;

  PROTECTED :

      AbsMovingEntitiesExt(
          const std::shared_ptr<const AllEntities>& all_,
          std::unique_ptr<IMovingEntitiesExt> nextExt_) noexcept;

  /**
  Some extensions might want to change the content of the Symbols Table
  after a move is performed
  */
  virtual void _addMovePostProcessing(SymbolsTable&) const noexcept {}

  /**
  Some extensions might want to change the content of the Symbols Table
  after a move is removed
  */
  virtual void _removeMovePostProcessing(SymbolsTable&) const noexcept {}

  /// Selecting a new group of entities for moving to the other bank
  virtual void _newGroup(const std::set<unsigned>&) = 0;

  /// Adds a new entity to the group from the raft/bridge
  /// @throw out_of_range for invalid id
  virtual void _addEntity(unsigned /*id*/) {}

  /// Removes an existing entity from the raft/bridge
  /// @throw out_of_range for invalid id
  virtual void _removeEntity(unsigned /*id*/) {}

  /// @return a clone of these extensions
  [[nodiscard]] virtual std::unique_ptr<IMovingEntitiesExt> _clone(
      std::unique_ptr<IMovingEntitiesExt> cloneOfNextExt) const noexcept = 0;

  /**
  Display either only suffix (most of them), or only prefix extensions.
  It needs to be called before (with param false) and after (with param true)
  the information of the moving entities
  */
  [[nodiscard]] virtual std::string _toString(bool = true) const { return {}; }

  std::shared_ptr<const AllEntities> all;
  std::unique_ptr<IMovingEntitiesExt> nextExt;
};

/// Entities traversing the river on the raft / over the bridge
class MovingEntities : public IsolatedEntities {
 public:
  template <class IdsCont = std::vector<unsigned>>
  explicit MovingEntities(const std::shared_ptr<const AllEntities>& all_,
                          const IdsCont& ids_ = {},
                          std::unique_ptr<IMovingEntitiesExt> extension_ =
                              std::make_unique<DefMovingEntitiesExt>())
      : IsolatedEntities{all_, ids_},
        extension{gsl::not_null<IMovingEntitiesExt*>(extension_.release())} {
    extension->newGroup(_ids);
  }

  MovingEntities(const MovingEntities& other) noexcept;
  MovingEntities(MovingEntities&& other) noexcept;
  MovingEntities& operator=(const MovingEntities& other);

  /// @throws logic_error if entities to be moved are from a different scenario
  MovingEntities& operator=(MovingEntities&& other) noexcept(!UT);

  /// @return a new subset with the provided ids_ from the original pool
  template <class IdsCont>
  MovingEntities& operator=(const IdsCont& ids_) {
    /*
    Calls:
    - virtual extension->newGroup({})
    - non-virtual operator+=(id), so no extension->addEntity(id)
    */
    IsolatedEntities::operator=(ids_);

    // no redundant previous extension->addEntity(id) calls
    extension->newGroup(_ids);
    return *this;
  }

  /// @return a new subset with the provided ids_ from the original pool
  MovingEntities& operator=(const std::initializer_list<unsigned>& ids_);

  /// Appends to the subset the entity with the given id
  MovingEntities& operator+=(unsigned id);

  /// Removes from the subset the entity with the given id
  MovingEntities& operator-=(unsigned id);

  ~MovingEntities() noexcept override = default;

  /// Empties the subset of ids. Leaves the choice pool in place
  void clear() noexcept override;

  [[nodiscard]] gsl::not_null<const IMovingEntitiesExt*> getExtension()
      const noexcept {
    return extension.get();
  }

  [[nodiscard]] std::string toString() const override;

  PROTECTED :

      std::unique_ptr<IMovingEntitiesExt>
          extension;
};

/// Entities from either bank
class BankEntities : public IsolatedEntities {
 public:
  template <class IdsCont = std::vector<unsigned>>
  explicit BankEntities(const std::shared_ptr<const AllEntities>& all_,
                        const IdsCont& ids_ = {})
      : IsolatedEntities{all_, ids_} {}

  BankEntities(const BankEntities& other) noexcept;
  BankEntities(BankEntities&& other) noexcept;
  BankEntities& operator=(const BankEntities& other);

  /// @throws logic_error if entities to be moved are from a different scenario
  BankEntities& operator=(BankEntities&& other) noexcept(!UT);

  ~BankEntities() noexcept override = default;

  /// @return a new subset with the provided ids_ from the original pool
  template <class IdsCont>
  BankEntities& operator=(const IdsCont& ids_) {
    IsolatedEntities::operator=(ids_);
    return *this;
  }

  /// @return a new subset with the provided ids_ from the original pool
  BankEntities& operator=(const std::initializer_list<unsigned>& ids_);

  /// @return the updated subset which contains now also arrivedEnts
  BankEntities& operator+=(const MovingEntities& arrivedEnts);

  /// @return the updated subset from which leftEnts were removed
  BankEntities& operator-=(const MovingEntities& leftEnts);

  /// @return the complementary subset of entities from the pool
  BankEntities operator~() const noexcept;

  /// @return size of symmetric difference between these and other's ids
  [[nodiscard]] size_t differencesCount(
      const BankEntities& other) const noexcept;
};

}  // namespace rc::ent

namespace std {

std::ostream& operator<<(std::ostream& os, const rc::ent::IEntities& ents);

}  // namespace std

#endif  // H_ENTITIES_MANAGER not defined
