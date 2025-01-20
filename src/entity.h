/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_ENTITY
#define H_ENTITY

#include "absConfigConstraint.h"

#include <boost/property_tree/ptree.hpp>

namespace rc::ent {

/// Person / Animal / Object that needs to cross the river.
class Entity : public IEntity {
 public:
  /// @throw domain_error for invalid canRowExpr
  /// @throw invalid_argument for invalid weight_
  Entity(unsigned id_,
         const std::string& name_,
         const std::string& type_ = "",
         bool startsFromRightBank_ = false,
         const std::string& canRowExpr = "false",
         double weight_ = 0.);

  /// @throw domain_error for invalid pt content
  explicit Entity(const boost::property_tree::ptree& pt);
  ~Entity() noexcept override = default;

  Entity(const Entity&) = delete;
  Entity(Entity&&) = delete;
  void operator=(const Entity&) = delete;
  void operator=(Entity&&) = delete;

  [[nodiscard]] unsigned id() const noexcept final;  ///< unique id

  /// Unique name
  [[nodiscard]] const std::string& name() const noexcept final;

  /// the default starting bank is the left one
  [[nodiscard]] bool startsFromRightBank() const noexcept override;

  /**
  By default, the entities cannot row. Even the other ones might take some
  breaks
  @param st the symbols table
  @return true if the entity is able to row in the provided context
  */
  [[nodiscard]] bool canRow(const SymbolsTable& st) const override;

  /**
   @return true when the entity does row; false when it doesn't and
   indeterminate when its ability depends on variables from SymbolsTable
  */
  [[nodiscard]] boost::logic::tribool canRow() const noexcept override;

  /// Type of entity; '' if unspecified
  [[nodiscard]] const std::string& type() const noexcept override;

  /// Weight of entity; 0 if unspecified
  [[nodiscard]] double weight() const noexcept override;

  [[nodiscard]] std::string toString() const override;

  PROTECTED :

      /// Name of the entity - mandatory
      std::string _name;
  std::string _type;  ///< type of the entity - optional

  /// Whether the entity can row / move by itself to the other bank
  std::shared_ptr<const cond::LogicalExpr> _canRow;

  double _weight{};  ///< weight of the entity - optional
  unsigned _id{};    ///< id of the entity - mandatory

  /// Provides the initial location of this entity. It needs to get on the
  /// opposite bank
  bool _startsFromRightBank{};
};

}  // namespace rc::ent

#endif  // H_ENTITY not defined
