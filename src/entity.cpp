/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#include "entity.h"
#include "absConfigConstraint.h"
#include "configParser.h"
#include "util.h"

#include <string>
#include <sstream>
#include <cassert>

using namespace std;
using namespace boost::property_tree;

namespace {

/**
  @return the semantic from canRowExpr
  @throw domain_error if canRowExpr is incorrect
*/
shared_ptr<const rc::cond::LogicalExpr> canRowSemantic(const string &canRowExpr) {
  shared_ptr<const rc::cond::LogicalExpr> semantic =
    rc::grammar::parseCanRowExpr(canRowExpr);
  return CP_EX_MSG(semantic, domain_error,
                 "CanRow parsing error! See the cause above.");
}

} // anonymous namespace

namespace rc {
namespace ent {

Entity::Entity(unsigned id_, const string &name_,
			const string &type_/* = ""*/,
			bool startsFromRightBank_/* = false*/,
			const string &canRowExpr/* = "false"*/,
			double weight_/* = 0.*/) :
		_name(name_), _type(type_), _canRow(canRowSemantic(canRowExpr)),
		_weight(weight_), _id(id_),
		_startsFromRightBank(startsFromRightBank_) {
	if(_weight < 0.)
		throw invalid_argument(string(__func__) +
			" - Please don't specify negative weights!");
}

Entity::Entity(const ptree &ent) :
		_type(ent.get("Type", ""s)) {
	try {
		// get<unsigned> fails to signal negative values
		const int readId = ent.get<int>("Id");
		if(readId < 0)
			throw domain_error(string(__func__) +
				" - Entity id-s cannot be negative!");

		_id = (unsigned)readId;
		_name = ent.get<string>("Name");
	} catch(const ptree_bad_path &ex) {
		throw domain_error(string(__func__) +
			" - Missing mandatory entity property! "s + ex.what());
	} catch(const ptree_bad_data &ex) {
		throw domain_error(string(__func__) +
			" - Invalid type of entity property! "s + ex.what());
	}

	try {
		const auto startsFromRightBankTree =
			ent.get_child_optional("StartsFromRightBank");
		if(startsFromRightBankTree)
			_startsFromRightBank = startsFromRightBankTree->get_value<bool>();
		const auto weightTree = ent.get_child_optional("Weight");
		if(weightTree) {
			_weight = weightTree->get_value<double>();
			if(_weight <= 0.)
				throw domain_error(string(__func__) +
					" - Please don't specify 0 or negative values for weight!");
		}
	} catch(const ptree_bad_data &ex) {
		throw domain_error(string(__func__) +
			" - Invalid type of entity property! "s + ex.what());
	}

	string canRowExpr = ent.get("CanRow", ""s);
	if(canRowExpr.empty())
		canRowExpr = ent.get("CanTackleBridgeCrossing", "false"s);
	else if( ! ent.get("CanTackleBridgeCrossing", ""s).empty())
		throw domain_error(string(__func__) +
			"Only one from the keys {CanRow, CanTackleBridgeCrossing} can appear. "
			"Please correct entity with id="s + to_string(_id)
			);
	_canRow = canRowSemantic(canRowExpr);
}

unsigned Entity::id() const {
	return _id;
}

const string& Entity::name() const {
	return _name;
}

bool Entity::startsFromRightBank() const {
	return _startsFromRightBank;
}

bool Entity::canRow(const SymbolsTable &st) const {
	assert(_canRow);
	return _canRow->eval(st);
}

boost::logic::tribool Entity::canRow() const {
	assert(_canRow);
	using namespace boost::logic;
	if( ! _canRow->constValue())
		return tribool(indeterminate);

	return *_canRow->constValue();
}

const string& Entity::type() const {
	return _type;
}

double Entity::weight() const {
	return _weight;
}

string Entity::toString() const {
	ostringstream oss;
	oss<<"Entity "<<_id<<" {Name: `"<<_name<<'`';
	if( ! _type.empty())
		oss<<", Type: `"<<_type<<'`';
	if(_weight > 0.)
		oss<<", Weight: "<<_weight;
	if(_startsFromRightBank)
		oss<<", StartsFromRightBank: true";
	if(_canRow->toString().compare("false") != 0)
		oss<<", CanRow: `"<<*_canRow<<'`';
	oss<<'}';
	return oss.str();
}

} // namespace ent
} // namespace rc

namespace std {

ostream& operator<<(ostream &os, const rc::ent::IEntity &e) {
  os<<e.toString();
  return os;
}

} // namespace std

#ifdef UNIT_TESTING

/*
	This include allows recompiling only the Unit tests project when updating the tests.
	It also keeps the count of total code units to recompile to a minimum value.
*/
#define ENTITY_CPP
#include "../test/entity.hpp"

#endif // UNIT_TESTING defined
