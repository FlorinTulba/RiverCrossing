/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#include "precompiled.h"
// This keeps precompiled.h first; Otherwise header sorting might move it

#include "util.h"

namespace rc {

const std::filesystem::path projectFolder() noexcept {
  using namespace std::filesystem;

  path dir{absolute(".")};

  for (;;) {
    if (!dir.filename().compare(path{"RiverCrossing"}))
      return dir;

    if (!dir.has_parent_path())
      break;

    dir = dir.parent_path();
  }

  return {};
}

}  // namespace rc
