#include "channel.hpp"
#include "pst_spectra.hpp"

#include <cmath>
#include <complex>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

bool almost_equal(const double lhs, const double rhs, const double tolerance = 1e-12) {
  return std::abs(lhs - rhs) <= tolerance;
}

void require(const bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_calibrated_mean_magnitude_stack() {
  auto frequencies = std::make_shared<fftw_freqs>(8.0, 8, 8);
  channel chan;
  chan.cal = std::make_shared<calibration>("MFS-07e", 796);
  chan.cal->f = frequencies->get_frequencies();
  chan.cal->a.assign(chan.cal->f.size(), 2.0);
  chan.cal->p.assign(chan.cal->f.size(), 0.0);

  std::vector<std::complex<double>> first(chan.cal->f.size(), {2.0, 0.0});
  std::vector<std::complex<double>> second(chan.cal->f.size(), {-2.0, 0.0});
  chan.qspc.push(first);
  chan.qspc.push(second);

  const auto stacked = chan.atss_simple_stack_all_calibrated(frequencies);
  const double expected = frequencies->get_wincal();
  require(stacked.size() == chan.cal->f.size(), "stacked frequency count mismatch");
  for (const auto value : stacked) {
    require(almost_equal(value, expected), "calibrated mean magnitude mismatch");
  }
  require(frequencies->get_raw_stacks() == 2, "raw stack count mismatch");
  require(chan.qspc.empty(), "spectra queue was not consumed");
}

void test_uncalibrated_complex_stack() {
  auto frequencies = std::make_shared<fftw_freqs>(8.0, 8, 8);
  channel chan;
  const auto frequency_count = frequencies->get_frequencies().size();
  const std::complex<double> first_value{2.0, -1.0};
  const std::complex<double> second_value{4.0, 3.0};
  chan.qspc.push(std::vector<std::complex<double>>(frequency_count, first_value));
  chan.qspc.push(std::vector<std::complex<double>>(frequency_count, second_value));

  const auto stacked = chan.atss_simple_stack_all(frequencies);
  const auto expected = (first_value + second_value) * (frequencies->get_wincal() / 2.0);
  require(stacked.size() == frequency_count, "raw stacked frequency count mismatch");
  for (const auto value : stacked) {
    require(std::abs(value - expected) < 1e-12, "raw complex stack mismatch");
  }
  require(frequencies->get_raw_stacks() == 2, "raw stack count mismatch");
  require(chan.qspc.empty(), "raw spectra queue was not consumed");
}

void test_empty_stack_rejected() {
  auto frequencies = std::make_shared<fftw_freqs>(8.0, 8, 8);
  channel chan;
  chan.cal = std::make_shared<calibration>("MFS-07e", 796);
  chan.cal->f = frequencies->get_frequencies();
  chan.cal->a.assign(chan.cal->f.size(), 1.0);
  chan.cal->p.assign(chan.cal->f.size(), 0.0);

  bool threw = false;
  try {
    static_cast<void>(chan.atss_simple_stack_all_calibrated(frequencies));
  } catch (const std::runtime_error &) {
    threw = true;
  }
  require(threw, "empty spectra queue was accepted");
}

void test_weighted_band_merge_is_order_independent() {
  const pst_sensor_id sensor{"MFS-07e", 796};
  const pst_spectrum_band first{sensor, "station", 1, 128.0, 2, {1.0, 2.0}, {10.0, 20.0}};
  const pst_spectrum_band second{sensor, "station", 2, 512.0, 6, {2.0, 3.0}, {40.0, 60.0}};

  const auto forward = pst_spectra_processor::merge_bands({first, second});
  const auto reverse = pst_spectra_processor::merge_bands({second, first});
  require(forward.size() == 1 && reverse.size() == 1, "sensor bands were not merged");
  require(forward.front().frequencies == std::vector<double>({1.0, 2.0, 3.0}), "merged frequencies are not sorted and unique");
  require(forward.front().frequencies == reverse.front().frequencies, "merge frequency order depends on input order");
  require(forward.front().amplitudes == reverse.front().amplitudes, "merge amplitudes depend on input order");
  require(almost_equal(forward.front().amplitudes[1], 35.0), "overlap was not weighted by FFT window count");
}

std::vector<std::shared_ptr<channel>> load_station_channels(const std::filesystem::path &station_directory) {
  std::vector<std::shared_ptr<channel>> channels;
  for (const auto &entry : std::filesystem::recursive_directory_iterator(station_directory)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      channels.push_back(std::make_shared<channel>(entry.path()));
    }
  }
  return channels;
}

void process_station_directories(const int argc, char **argv) {
  std::vector<std::shared_ptr<channel>> channels;
  for (int i = 1; i < argc; ++i) {
    auto station_channels = load_station_channels(argv[i]);
    channels.insert(channels.end(), station_channels.begin(), station_channels.end());
  }
  auto pool = std::make_shared<BS::thread_pool<BS::tp::none>>();
  pst_spectra_processor processor(pool);
  const auto spectra = processor.process(channels);
  for (const auto &spectrum : spectra) {
    std::cout << spectrum.sensor.label() << ": " << spectrum.frequencies.size() << " points, "
              << spectrum.total_windows << " windows\n";
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    test_calibrated_mean_magnitude_stack();
    test_uncalibrated_complex_stack();
    test_empty_stack_rejected();
    test_weighted_band_merge_is_order_independent();
    if (argc > 1) {
      process_station_directories(argc, argv);
    }
  } catch (const std::exception &error) {
    std::cerr << "test_pst_spectra failed: " << error.what() << '\n';
    return 1;
  }
  std::cout << "test_pst_spectra passed\n";
  return 0;
}
