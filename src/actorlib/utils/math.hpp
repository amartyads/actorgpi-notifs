//
// Created by Bruno Macedo Miguel on 2019-05-11.
//
#ifndef ACTORMPI_MATH_HPP
#define ACTORMPI_MATH_HPP

#include <iostream>

namespace math {
static int compute_hash(std::string const &s) {
  const int p = 53;
  const int m = 1e9 + 9;
  long long hash_value = 0;
  long long p_pow = 1;
  for (char c : s) {
    hash_value = (hash_value + (c - 'a' + 1) * p_pow) % m;
    p_pow = (p_pow * p) % m;
  }
  return abs(
      hash_value); // TODO: abs is hack! think about another hashing technique
}
} // namespace helper

#endif