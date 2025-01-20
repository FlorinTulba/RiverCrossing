/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_WARNINGS
#define H_WARNINGS

// Warning ids that may be suppressed during compilation

/**
 * Ctor of Move takes a unique_ptr by value and triggers this warning
 * when braced-initialized.
 */
#define WARN_MSVC_BRACED_INIT_LIST_ORDER 4868

/**
 * When the ctor is private and not a friend of make_unique, new & delete are ok
 */
#define WARN_MSVC_EXPLICIT_NEW_DELETE 26409

#endif  // !H_WARNINGS
