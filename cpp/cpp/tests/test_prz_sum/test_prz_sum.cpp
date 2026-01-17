#include "channel.hpp"
#include "strings_etc.hpp"
#include <iostream>
#include <vector>

int main() {
  // test_prz_sum
  size_t n = 32;
  std::vector<double> data_1(n);
  std::vector<double> data_2(n);
  // fill data_1 and data_2 with some values
  for (size_t i = 0; i < n; i++) {
    data_1[i] = static_cast<double>(i + 9);
    data_2[i] = static_cast<double>(i * 2 - n);
  }
  // sum the two vectors
  auto sum_1 = std::accumulate(data_1.begin(), data_1.end(), 0.0);
  auto sum_2 = std::accumulate(data_2.begin(), data_2.end(), 0.0);
  std::cout << "Sum of data_1: " << sum_1 << std::endl;
  std::cout << "Sum of data_2: " << sum_2 << std::endl;
  sum_1 /= n;
  sum_2 /= n;
  std::cout << "Multiplication of sums: " << sum_1 * sum_2 << std::endl;
  // fist multiply element-wise and then sum
  double sum_product = 0.0;
  for (size_t i = 0; i < n; i++) {
    sum_product += data_1[i] * data_2[i];
  }
  std::cout << "Sum of element-wise multiplication: " << sum_product / n << std::endl;

  return 0;
}
