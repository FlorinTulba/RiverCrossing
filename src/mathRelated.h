/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#ifndef H_MATH_RELATED
#define H_MATH_RELATED

#include <limits>
#include <cstring>

namespace rc {

constexpr double Eps = 1e-6;

// std::isnan(double) fails in g++ when compiled with -ffast-math:
// https://stackoverflow.com/questions/570669/checking-if-a-double-or-float-is-nan-in-c
template<typename T>
constexpr bool isNaN(T v) {
  constexpr double nanV = std::numeric_limits<T>::quiet_NaN();
  return (0 == std::memcmp(&v, &nanV, sizeof(T)));
}

} // namespace rc

#endif // H_MATH_RELATED not defined
