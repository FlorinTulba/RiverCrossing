/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#ifndef H_NAN_CONCERNS
#define H_NAN_CONCERNS

/*
isnan(NaN) might evaluate to false due to compilation flags.
Substituting isnan() by the code below might be a solution:

  constexpr double nanV{std::numeric_limits<T>::quiet_NaN()};
  return !std::memcmp(&v, &nanV, sizeof(T));

Gcc/Clang specific
==================
The relevant compilation flags are:
'-Ofast' (GCC specific) - sets the '-ffast-math' flag implicitly.
'-ffast-math' => Math ops are performed as if Inf and NaN values don't exist.
  It will define '__FAST_MATH__' = '__FINITE_MATH_ONLY__' = 1
  only if none of the other flags are provided.

  When '-ffast-math' isn't provided, there will be only a define:
    '__FINITE_MATH_ONLY__' = 0

'-fno-finite-math-only' => NaN and Inf must be considered within operations OR
'-fhonor-nans' (Clang specific) => NaN must be considered within operations
  Any of these flags will undefine '__FAST_MATH__' and will define
    '__FINITE_MATH_ONLY__' = 0

The command:
  <C++compiler> -dM -e - < /dev/null
provides the defines used by GCC/Clang during compilation.

Assigning numeric_limits<T>::quiet_NaN() is ignored by Clang (>=19?)
when compiling with '-ffast-math' without '-fhonor-nans'
Surrounding with '#pragma clang optimize off / on' has no effect.
The operation 0./0. might force the NaN assignment.

MSVC specific
=============
Same concerns as above about NaN values for the MSVC compiler, which defines
'_M_FP_FAST' only when '/fp:fast' is set.
'/fp:precise' or '/fp:strict' are ok.

However, '#pragma float_control' might change floating-point values handling
*/
#if defined(__FAST_MATH__) || defined(_M_FP_FAST)

#error \
    "NaN values might not be handled correctly \
by Gcc/Clang/MSVC under current compilation flags. \
For Gcc/Clang either compile without '-ffast-math'/'-Ofast' \
or add '-fno-finite-math-only' (or '-fhonor-nans' in Clang)! \
For MSVC either change '/fp:fast' into '/fp:precise' \
or insert '#pragma float_control(precise, on)' where NaNs are used!"

#endif  // __FAST_MATH__ or _M_FP_FAST defined

#endif  // H_NAN_CONCERNS
