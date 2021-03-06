//  This file is part of Empirical, https://github.com/mercere99/Empirical/
//  Copyright (C) Michigan State University, 2016-2017.
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  This file contains useful mathematical functions (that are constexpr when possible.)


#ifndef EMP_MATH_H
#define EMP_MATH_H

#include <cmath>
#include "const.h"

namespace emp {

  /// % is actually remainder; this is a proper modulus command that handles negative #'s correctly
  inline constexpr int Mod(int in_val, int mod_val) {
    return (in_val < 0) ? (in_val % mod_val + mod_val) : (in_val % mod_val);
  }

  /// Regular Mod doesn't work on doubles.  Build one that does!
  // @CAO Make constexpr?
  inline double Mod(double in_val, double mod_val) {
    const double remain = std::remainder(in_val, mod_val);
    return (remain < 0.0) ? (remain + mod_val) : remain;
  }

  /// Run both min and max on a value to put it into a desired range.
  template <typename TYPE> constexpr TYPE ToRange(const TYPE & value, const TYPE & in_min, const TYPE & in_max) {
    return (value < in_min) ? in_min : ((value > in_max) ? in_max : value);
  }

  /// Min of only one element is that element itself!
  template <typename T> constexpr T Min(T in1) { return in1; }

  /// Min of multiple elements is solved recursively.
  template <typename T, typename... Ts>
  constexpr T Min(T in1, T in2, Ts... extras) {
    T cur_result = Min(in2, extras...);
    return (in1 < cur_result) ? in1 : cur_result;
  }


  /// Max of only one element is that element itself!
  template <typename T> constexpr T Max(T in1) { return in1; }

  /// Max of multiple elements is solved recursively.
  template <typename T, typename... Ts>
  constexpr T Max(T in1, T in2, Ts... extras) {
    T cur_result = Max(in2, extras...);
    return (in1 < cur_result) ? cur_result : in1;
  }

  /// MinRef works like Min, but never copies any inputs; always treats as references.
  /// MinRef of only one element returns reference to that element itself!
  template <typename T> constexpr const T & MinRef(const T& in1) { return in1; }

  /// MinRef of multiple elements returns reference to minimum value.
  template <typename T, typename... Ts>
  constexpr const T & MinRef(const T& in1, const T& in2, const Ts&... extras) {
    const T & cur_result = MinRef(in2, extras...);
    return (in1 < cur_result) ? in1 : cur_result;
  }


  /// MaxRef works like Max, but never copies any inputs; always treats as references.
  /// MaxRef of only one element returns reference to that element itself!
  template <typename T> constexpr const T & MaxRef(const T& in1) { return in1; }

  /// MaxRef of multiple elements returns reference to maximum value.
  template <typename T, typename... Ts>
  constexpr const T & MaxRef(const T& in1, const T& in2, const Ts&... extras) {
    const T & cur_result = MaxRef(in2, extras...);
    return (in1 < cur_result) ? cur_result : in1;
  }


  namespace {
    // A compile-time log calculator for values [1,2)
    static constexpr double Log2_base(double x) {
      return log2_chart_1_2[(int)((x-1.0)*1024)];
      // return InterpolateTable(log2_chart_1_2, x-1.0, 1024);
    }

    // A compile-time log calculator for values < 1
    static constexpr double Log2_frac(double x) {
      return (x >= 1.0) ? Log2_base(x) : (Log2_frac(x*2.0) - 1.0);
    }

    // A compile-time log calculator for values >= 2
    static constexpr double Log2_pos(double x) {
      return (x < 2.0) ? Log2_base(x) : (Log2_pos(x/2.0) + 1.0);
    }

  }

  /// @endcond

  /// Compile-time log base 2 calculator.
  static constexpr double Log2(double x) {
    return (x < 1.0) ? Log2_frac(x) : Log2_pos(x);
  }

  /// Compile-time log calculator
  static constexpr double Log(double x, double base=10.0) { return Log2(x) / Log2(base); }
  /// Compile-time natural log calculator
  static constexpr double Ln(double x) { return Log(x, emp::E); }   // Natural Log...
  /// Compile-time log base 10 calculator.
  static constexpr double Log10(double x) { return Log(x, 10.0); }

  namespace {
    static constexpr double Pow2_lt1(double exp, int id=0) {
      return (id==32) ? 1.0 :
        ( (exp > 0.5) ? (pow2_chart_bits[id]*Pow2_lt1(exp*2.0-1.0,id+1)) : Pow2_lt1(exp*2.0,id+1) );
    }

    static constexpr double Pow2_impl(double exp) {
      return (exp >= 1.0) ? (2.0*Pow2_impl(exp-1.0)) : Pow2_lt1(exp);
    }
  }

  static constexpr double Pow2(double exp) {
    return (exp < 0.0) ? (1.0/Pow2_impl(-exp)) : Pow2_impl(exp);
  }

  template <typename TYPE>
  static constexpr TYPE IntPow(TYPE base, TYPE exp) {
    return exp < 1 ? 1 : (base * IntPow(base, exp-1));
  }

  static constexpr double Pow(double base, double exp) {
    // Normally, convert to a base of 2 and then use Pow2.
    // If base is negative, we don't want to deal with imaginary numbers, so use IntPow.
    return (base > 0) ? Pow2(Log2(base) * exp) : IntPow(base,exp);
  }

  // A fast (O(log p)) integer-power command.
  // static constexpr int Pow(int base, int p) {
  //   return (p <= 0) ? 1 : (base * Pow(base, p-1));
  // }

  static constexpr double Exp(double exp) {
    return Pow2(Log2(emp::E) * exp);  // convert to a base of e.
  }


  /// A compile-time int-log calculator (aka, significant bits)
  template <typename TYPE>
  static constexpr int IntLog2(TYPE x) { return x <= 1 ? 0 : (IntLog2(x/2) + 1); }

  /// A compile-time bit counter.
  template <typename TYPE>
  static constexpr int CountOnes(TYPE x) { return x == 0 ? 0 : (CountOnes(x/2) + (x&1)); }

  /// Quick bit-mask generators...
  template <typename TYPE>
  static constexpr TYPE MaskLow(size_t num_bits) {
    return (num_bits == 8*sizeof(TYPE)) ? ((TYPE)-1) : ((((TYPE)1) << num_bits) - 1);
  }

  template <typename TYPE>
  static constexpr TYPE MaskHigh(size_t num_bits) {
    return MaskLow<TYPE>(num_bits) << (8*sizeof(TYPE)-num_bits);
  }


}

#endif
