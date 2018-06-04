/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_ENTITY
#define H_ENTITY

#include "absEntity.h"

#include <memory>

#include <boost/property_tree/ptree.hpp>

namespace rc {

namespace cond {

struct LogicalExpr; // forward declaration

} // namespace cond

namespace ent {

/// Person / Animal / Object that needs to cross the river.
class Entity : public IEntity {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

	std::string _name;	///< name of the entity - mandatory
	std::string _type;	///< type of the entity - optional

	/// Whether the entity can row / move by itself to the other bank
	std::shared_ptr<const cond::LogicalExpr> _canRow;

	double _weight = 0.;///< weight of the entity - optional
	unsigned _id = 0U;	///< id of the entity - mandatory

	/// Provides the initial location of this entity. It needs to get on the opposite bank
	bool _startsFromRightBank = false;

public:
  /// @throw domain_error for invalid canRowExpr
  /// @throw invalid_argument for invalid weight_
	Entity(unsigned id_, const std::string &name_,
		const std::string &type_ = "",
		bool startsFromRightBank_ = false,
		const std::string &canRowExpr = "false",
		double weight_ = 0.);

  /// @throw domain_error for invalid pt content
	Entity(const boost::property_tree::ptree &pt);

	unsigned id() const override final;							///< unique id
	const std::string& name() const override final;	///< unique name

	/// the default starting bank is the left one
	bool startsFromRightBank() const override;

  /**
    By default, the entities cannot row. Even the other ones might take some breaks
    @param st the symbols table
    @return true if the entity is able to row in the provided context
  */
	bool canRow(const SymbolsTable &st) const override;

  /**
    @return true when the entity does row; false when it doesn't
      and indeterminate when its ability depends on variables from SymbolsTable
  */
  boost::logic::tribool canRow() const override;

	const std::string& type() const override;	///< type of entity; '' if unspecified
	double weight() const override;	///< weight of entity; 0 if unspecified

	virtual std::string toString() const override;
};

} // namespace ent
} // namespace rc

#endif // H_ENTITY not defined
