/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#include "symbolsTable.h"

namespace rc {

/*
When setting the initial state of a scenario,
the 1-based index of the crossing moves (CrossingIndex)
should have an invalid value and 0 is just fine,
as it only needs to be incremented for the first actual crossing move.
*/
const SymbolsTable InitialSymbolsTable{{"CrossingIndex", 0.}};

} // namespace rc
