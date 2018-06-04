/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_ABS_ENTITY
#define H_ABS_ENTITY

#include "symbolsTable.h"

#include <iostream>

#include <boost/logic/tribool.hpp>

namespace rc {
namespace ent {

/// Person / Animal / Object that needs to cross the river.
struct IEntity /*abstract*/ {
  virtual ~IEntity() /*= 0*/ {}

  // Mandatory information
  virtual unsigned id() const = 0;              ///< unique id
  virtual const std::string& name() const = 0;  ///< unique name
  virtual bool startsFromRightBank() const
    = 0; ///< the default starting bank is the left one

  // Optional, defaulted information.
  virtual const std::string& type() const = 0; ///< type of entity; '' if unspecified
  virtual double weight() const = 0; ///< weight of entity; 0 if unspecified

  /**
    By default, the entities cannot row. Even the other ones might take some breaks
    @param st the symbols table
    @return true if the entity is able to row in the provided context
  */
  virtual bool canRow(const SymbolsTable &st) const = 0;

  /**
    @return true when the entity does row; false when it doesn't
      and indeterminate when its ability depends on variables from SymbolsTable
  */
  virtual boost::logic::tribool canRow() const = 0;

  virtual std::string toString() const = 0;
};

} // namespace ent
} // namespace rc

namespace std {

std::ostream& operator<<(std::ostream &os, const rc::ent::IEntity &e);

} // namespace std

#endif // H_ABS_ENTITY not defined
