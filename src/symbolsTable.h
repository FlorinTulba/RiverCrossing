/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_SYMBOLS_TABLE
#define H_SYMBOLS_TABLE

#include <string>
#include <unordered_map>

namespace rc {

/// Type of the symbols table - numeric values associated to string keys
using SymbolsTable = std::unordered_map<std::string, double>;

/**
When setting the initial state of a scenario,
the 1-based index of the crossing moves (CrossingIndex)
should have an invalid value and 0 is just fine,
as it only needs to be incremented for the first actual crossing move.
*/
const SymbolsTable& InitialSymbolsTable() noexcept;

}  // namespace rc

#endif  // H_SYMBOLS_TABLE not defined
