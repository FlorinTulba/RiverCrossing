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
#include "entity.hpp"  // IWYU pragma: keep

#endif  // UNIT_TESTING defined

#include "absConfigConstraint.h"
#include "configParser.h"
#include "entity.h"
#include "symbolsTable.h"
#include "util.h"

#include <cassert>

#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <gsl/pointers>
#include <gsl/util>

#include <boost/logic/tribool.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>

using namespace std;
using namespace boost::property_tree;

namespace {

/**
  @return the semantic from canRowExpr
  @throw domain_error if canRowExpr is incorrect
*/
gsl::not_null<shared_ptr<const rc::cond::LogicalExpr>> canRowSemantic(
    const string& canRowExpr) {
  shared_ptr<const rc::cond::LogicalExpr> semantic{
      rc::grammar::parseCanRowExpr(canRowExpr)};
  return rc::throwIfNull<domain_error>(
      semantic, "CanRow parsing error! See the cause above.");
}

}  // anonymous namespace

namespace rc::ent {

Entity::Entity(unsigned id_,
               string name_,
               string type_ /* = ""*/,
               bool startsFromRightBank_ /* = false*/,
               const string& canRowExpr /* = "false"*/,
               double weight_ /* = 0.*/)
    : _name{std::move(name_)},
      _type{std::move(type_)},
      _canRow{canRowSemantic(canRowExpr)},
      _weight{weight_},
      _id{id_},
      _startsFromRightBank{startsFromRightBank_} {
  if (_weight < 0.)
    throw invalid_argument{format("{} - Please don't specify negative weights!",
                                  HERE.function_name())};
  assert(_canRow);
}

Entity::Entity(const ptree& ent) : _type{ent.get("Type", string{})} {
  try {
    // get<unsigned> fails to signal negative values
    const int readId{ent.get<int>("Id")};
    if (readId < 0)
      throw domain_error{
          format("{} - Entity id-s cannot be negative!", HERE.function_name())};

    _id = gsl::narrow_cast<unsigned>(readId);
    _name = ent.get<string>("Name");
  } catch (const ptree_bad_path& ex) {
    throw domain_error{format("{} - Missing mandatory entity property! {}",
                              HERE.function_name(), ex.what())};
  } catch (const ptree_bad_data& ex) {
    throw domain_error{format("{} - Invalid type of entity property! {}",
                              HERE.function_name(), ex.what())};
  }

  try {
    const auto startsFromRightBankTree =
        ent.get_child_optional("StartsFromRightBank");
    if (startsFromRightBankTree)
      _startsFromRightBank = startsFromRightBankTree->get_value<bool>();
    const auto weightTree = ent.get_child_optional("Weight");
    if (weightTree) {
      _weight = weightTree->get_value<double>();
      if (_weight <= 0.)
        throw domain_error{
            format("{} - Please don't specify 0 or negative values for weight!",
                   HERE.function_name())};
    }
  } catch (const ptree_bad_data& ex) {
    throw domain_error{format("{} - Invalid type of entity property! {}",
                              HERE.function_name(), ex.what())};
  }

  string canRowExpr{ent.get("CanRow", string{})};
  if (canRowExpr.empty())
    canRowExpr = ent.get("CanTackleBridgeCrossing", string{"false"});
  else if (!ent.get("CanTackleBridgeCrossing", string{}).empty())
    throw domain_error{format(
        "{} - Only one from the keys {{CanRow, CanTackleBridgeCrossing}} "
        "can appear. "
        "Please correct entity with id={}",
        HERE.function_name(), _id)};
  _canRow = canRowSemantic(canRowExpr);
  assert(_canRow);
}

unsigned Entity::id() const noexcept {
  return _id;
}

const string& Entity::name() const noexcept {
  return _name;
}

bool Entity::startsFromRightBank() const noexcept {
  return _startsFromRightBank;
}

bool Entity::canRow(const SymbolsTable& st) const {
  return _canRow->eval(st);
}

boost::logic::tribool Entity::canRow() const noexcept {
  using namespace boost::logic;
  const auto& constValOpt{_canRow->constValue()};
  if (!constValOpt.has_value())
    return indeterminate;

  return *constValOpt;
}

const string& Entity::type() const noexcept {
  return _type;
}

double Entity::weight() const noexcept {
  return _weight;
}

void Entity::formatTo(FmtCtxIt& outIt) const {
  outIt = format_to(outIt, "Entity {} {{Name: `{}`", _id, _name);

  if (!_type.empty())
    outIt = format_to(outIt, ", Type: `{}`", _type);

  if (_weight > 0.)
    outIt = format_to(outIt, ", Weight: `{}`", _weight);

  if (_startsFromRightBank)
    outIt = format_to(outIt, ", StartsFromRightBank: true");

  if (format("{}", *_canRow) != "false")
    outIt = format_to(outIt, ", CanRow: `{}`", *_canRow);

  outIt = format_to(outIt, "}}");
}

}  // namespace rc::ent
