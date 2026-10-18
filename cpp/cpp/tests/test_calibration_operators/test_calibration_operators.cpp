#include "cal_base.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace {

using compound_operator = calibration &(calibration::*)(const calibration &);

void require(const bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

calibration make_calibration() {
  calibration result("MFS-06e", 123, ChopperStatus::on);
  result.f = {0.1, 1.0, 10.0};
  result.a = {2.0, 3.0, 4.0};
  result.p = {170.0, -170.0, 0.0};
  result.board = "LF";
  result.system = "ADU-08e";
  result.R = 100.0;
  result.C = 1.0e-9;
  result.sampling_rate = 128.0;
  result.file_path_in = "sensor_123.json";
  result.file_path_out = "combined_123.json";
  result.Operator = "test";
  return result;
}

calibration make_rhs() {
  calibration result = make_calibration();
  result.a = {5.0, 6.0, 8.0};
  result.p = {30.0, -40.0, 90.0};
  return result;
}

void require_metadata_unchanged(const calibration &actual, const calibration &expected) {
  require(actual.sensor == expected.sensor && actual.serial == expected.serial && actual.chopper == expected.chopper &&
              actual.units_frequency == expected.units_frequency && actual.units_amplitude == expected.units_amplitude &&
              actual.units_phase == expected.units_phase && actual.datetime == expected.datetime &&
              actual.Operator == expected.Operator && actual.ct == expected.ct &&
              actual.board == expected.board && actual.system == expected.system &&
              actual.R == expected.R && actual.C == expected.C && actual.sampling_rate == expected.sampling_rate &&
              actual.file_path_in == expected.file_path_in && actual.file_path_out == expected.file_path_out,
          "arithmetic changed calibration metadata");
}

void require_unchanged(const calibration &actual, const calibration &expected) {
  require(actual.f == expected.f && actual.a == expected.a && actual.p == expected.p,
          "arithmetic unexpectedly changed calibration data");
  require_metadata_unchanged(actual, expected);
}

void require_rejected(calibration lhs, const calibration &rhs, const compound_operator operation) {
  const calibration before = lhs;
  bool threw = false;
  try {
    (lhs.*operation)(rhs);
  } catch (const std::runtime_error &error) {
    threw = true;
    require(std::string(error.what()).find("operator") != std::string::npos, "error did not identify the operator");
  }
  require(threw, "invalid calibration arithmetic was accepted");
  require_unchanged(lhs, before);
}

void require_complex_equivalent(const calibration &actual, const calibration &lhs, const calibration &rhs, const bool divide) {
  std::vector<double> frequency;
  std::vector<std::complex<double>> result, left, right;
  actual.get_cplx_cal(frequency, result);
  lhs.get_cplx_cal(frequency, left);
  rhs.get_cplx_cal(frequency, right);
  for (size_t i = 0; i < result.size(); ++i) {
    const std::complex<double> expected = divide ? left[i] / right[i] : left[i] * right[i];
    require(std::abs(result[i] - expected) <= 1.0e-12 * std::max(1.0, std::abs(expected)),
            "amplitude/phase arithmetic disagrees with complex arithmetic");
  }
}

void test_arithmetic() {
  const calibration original = make_calibration();
  const calibration rhs = make_rhs();
  const calibration rhs_before = rhs;

  calibration product = original;
  product.backup();
  require(&(product *= rhs) == &product, "operator*= did not return the left operand");
  require(product.a == std::vector<double>({10.0, 18.0, 32.0}), "incorrect product amplitudes");
  require(product.p == std::vector<double>({200.0, -210.0, 90.0}), "incorrect or wrapped product phases");
  require(product.f == original.f, "operator*= changed frequency vector");
  require_metadata_unchanged(product, original);
  require_complex_equivalent(product, original, rhs, false);
  product.restore();
  require_unchanged(product, original);

  calibration quotient = original;
  quotient.backup();
  require(&(quotient /= rhs) == &quotient, "operator/= did not return the left operand");
  require(quotient.a == std::vector<double>({0.4, 0.5, 0.5}), "incorrect quotient amplitudes");
  require(quotient.p == std::vector<double>({140.0, -130.0, -90.0}), "incorrect quotient phases");
  require(quotient.f == original.f, "operator/= changed frequency vector");
  require_metadata_unchanged(quotient, original);
  require_complex_equivalent(quotient, original, rhs, true);
  quotient.restore();
  require_unchanged(quotient, original);

  calibration chained = original;
  require(&((chained *= rhs) /= rhs) == &chained, "compound operators cannot be chained");
  require_unchanged(chained, original);
  require_unchanged(rhs, rhs_before);
}

void test_invalid_data() {
  const calibration valid = make_calibration();
  for (const compound_operator operation : {&calibration::operator*=, &calibration::operator/=}) {
    require_rejected(calibration(), calibration(), operation);
    for (const auto member : {&calibration::a, &calibration::p}) {
      calibration invalid = valid;
      (invalid.*member).clear();
      require_rejected(invalid, valid, operation);
      require_rejected(valid, invalid, operation);

      invalid = valid;
      (invalid.*member).pop_back();
      require_rejected(invalid, valid, operation);
      require_rejected(valid, invalid, operation);

      invalid = valid;
      (invalid.*member).push_back(1.0);
      require_rejected(invalid, valid, operation);
      require_rejected(valid, invalid, operation);
    }

    calibration invalid = valid;
    invalid.f.clear();
    require_rejected(invalid, invalid, operation);

    invalid = valid;
    invalid.f[1] = std::nextafter(invalid.f[1], 2.0);
    require_rejected(valid, invalid, operation);

    invalid = valid;
    invalid.f[1] = 2.0;
    require_rejected(valid, invalid, operation);

    invalid = valid;
    invalid.f.pop_back();
    require_rejected(valid, invalid, operation);

    invalid = valid;
    std::reverse(invalid.f.begin(), invalid.f.end());
    require_rejected(valid, invalid, operation);
  }
}

void test_zero_and_self_operations() {
  const calibration original = make_calibration();
  for (const double zero : {0.0, -0.0}) {
    calibration rhs = make_rhs();
    rhs.a.back() = zero;
    require_rejected(original, rhs, &calibration::operator/=);
  }

  calibration zero = original;
  zero.a = {0.0, 0.0, 0.0};
  calibration result = original;
  result *= zero;
  require(result.a == zero.a, "multiplication by zero amplitude was rejected");
  zero /= original;
  require(zero.a == std::vector<double>({0.0, 0.0, 0.0}) &&
              zero.p == std::vector<double>({0.0, 0.0, 0.0}),
          "zero numerator division failed");

  calibration self = original;
  self *= self;
  require(self.a == std::vector<double>({4.0, 9.0, 16.0}) &&
              self.p == std::vector<double>({340.0, -340.0, 0.0}),
          "self multiplication failed");
  self = original;
  self /= self;
  require(self.a == std::vector<double>({1.0, 1.0, 1.0}) &&
              self.p == std::vector<double>({0.0, 0.0, 0.0}),
          "self division failed");
}

} // namespace

int main() {
  try {
    test_arithmetic();
    test_invalid_data();
    test_zero_and_self_operations();
    std::cout << "calibration operator tests passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
