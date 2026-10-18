#ifndef PST_SPECTRA_HPP
#define PST_SPECTRA_HPP

#include "BS_thread_pool.hpp"
#include "channel.hpp"

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct pst_sensor_id {
  std::string model;
  uint64_t serial = 0;

  bool operator<(const pst_sensor_id &rhs) const;
  bool operator==(const pst_sensor_id &rhs) const = default;
  std::string label() const;
};

struct pst_spectrum_band {
  pst_sensor_id sensor;
  std::string station;
  size_t run = 0;
  double sample_rate = 0.0;
  size_t window_count = 0;
  std::vector<double> frequencies;
  std::vector<double> amplitudes;
};

struct pst_sensor_spectrum {
  pst_sensor_id sensor;
  std::set<std::string> stations;
  size_t total_windows = 0;
  std::vector<double> frequencies;
  std::vector<double> amplitudes;
};

class pst_spectra_processor {
public:
  explicit pst_spectra_processor(std::shared_ptr<BS::thread_pool<BS::tp::none>> pool);

  std::vector<pst_sensor_spectrum> process(const std::vector<std::shared_ptr<channel>> &selected_channels) const;

  static void validate_parallel_test(const std::vector<std::shared_ptr<channel>> &channels);
  static std::vector<pst_sensor_spectrum> merge_bands(const std::vector<pst_spectrum_band> &bands);

private:
  static pst_sensor_id sensor_id(const std::shared_ptr<channel> &chan);
  static pst_spectrum_band process_channel(const std::shared_ptr<channel> &chan);

  std::shared_ptr<BS::thread_pool<BS::tp::none>> pool;
};

#endif // PST_SPECTRA_HPP
