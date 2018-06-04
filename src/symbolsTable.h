/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_SYMBOLS_TABLE
#define H_SYMBOLS_TABLE

#include <unordered_map>
#include <string>

namespace rc {

/// Type of the symbols table - numeric values associated to string keys
typedef std::unordered_map<std::string, double> SymbolsTable;

/// Content of the SymbolsTable at the beginning
extern const SymbolsTable InitialSymbolsTable;

} // namespace rc

#endif // H_SYMBOLS_TABLE not defined
