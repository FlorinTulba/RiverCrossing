/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_ABS_ENTITY
#define H_ABS_ENTITY

#include "symbolsTable.h"

#include <iostream>

#include <boost/logic/tribool.hpp>

namespace rc::ent {

/// Person / Animal / Object that needs to cross the river.
class IEntity {
 public:
  virtual ~IEntity() noexcept = default;

  IEntity(const IEntity&) = delete;
  IEntity(IEntity&&) = delete;
  void operator=(const IEntity&) = delete;
  void operator=(IEntity&&) = delete;

  // Mandatory information
  virtual unsigned id() const noexcept = 0;              ///< unique id
  virtual const std::string& name() const noexcept = 0;  ///< unique name

  /// The default starting bank is the left one
  virtual bool startsFromRightBank() const noexcept = 0;

  // Optional, defaulted information.

  /// Type of entity; '' if unspecified
  virtual const std::string& type() const noexcept = 0;
  virtual double weight()
      const noexcept = 0;  ///< weight of entity; 0 if unspecified

  /**
    By default, the entities cannot row. Even the other ones might take some
    breaks
    @param st the symbols table
    @return true if the entity is able to row in the provided context
  */
  virtual bool canRow(const SymbolsTable& st) const = 0;

  /**
    @return true when the entity does row; false when it doesn't
      and indeterminate when its ability depends on variables from SymbolsTable
  */
  virtual boost::logic::tribool canRow() const noexcept = 0;

  virtual std::string toString() const = 0;

 protected:
  IEntity() noexcept = default;
};

}  // namespace rc::ent

namespace std {

std::ostream& operator<<(std::ostream& os, const rc::ent::IEntity& e);

}  // namespace std

#endif  // H_ABS_ENTITY not defined
