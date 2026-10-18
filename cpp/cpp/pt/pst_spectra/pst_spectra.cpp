#include "pst_spectra.hpp"

#include <cmath>
#include <future>
#include <map>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <utility>

bool pst_sensor_id::operator<(const pst_sensor_id &rhs) const {
  return std::tie(this->model, this->serial) < std::tie(rhs.model, rhs.serial);
}

std::string pst_sensor_id::label() const {
  return this->model + " " + std::to_string(this->serial);
}

pst_spectra_processor::pst_spectra_processor(std::shared_ptr<BS::thread_pool<BS::tp::none>> pool) : pool(std::move(pool)) {
  if (this->pool == nullptr) {
    throw std::runtime_error("pst_spectra_processor :: thread pool is nullptr");
  }
}

pst_sensor_id pst_spectra_processor::sensor_id(const std::shared_ptr<channel> &chan) {
  if (chan == nullptr) {
    throw std::runtime_error("pst_spectra_processor :: empty channel");
  }
  if (chan->cal != nullptr && !chan->cal->sensor.empty()) {
    return {chan->cal->sensor, chan->cal->serial};
  }
  return {chan->get_system() + " " + chan->get_channel_type(), chan->get_serial()};
}

void pst_spectra_processor::validate_parallel_test(const std::vector<std::shared_ptr<channel>> &channels) {
  using interval_key = std::tuple<double, std::string, size_t>;

  std::set<pst_sensor_id> sensors;
  std::set<std::string> source_files;
  std::map<interval_key, std::set<pst_sensor_id>> interval_sensors;
  std::optional<bool> magnetic;

  for (const auto &chan : channels) {
    if (chan == nullptr) {
      throw std::runtime_error("PST selection contains an empty channel");
    }
    const bool is_magnetic = chan->is_type("H");
    const bool is_electric = chan->is_type("E");
    if (!is_magnetic && !is_electric) {
      throw std::runtime_error("PST selection must contain only electric or only magnetic channels");
    }
    if (magnetic.has_value() && *magnetic != is_magnetic) {
      throw std::runtime_error("PST selection must not mix electric and magnetic channels");
    }
    magnetic = is_magnetic;
    if (is_magnetic && (chan->cal == nullptr || chan->cal->sensor.empty())) {
      throw std::runtime_error("PST channel has no embedded sensor calibration: " + chan->get_json_filepath().string());
    }
    const double sample_rate = chan->get_sample_rate();
    if (sample_rate < 1.0 || std::abs(sample_rate - std::round(sample_rate)) > 1e-9) {
      throw std::runtime_error("PST requires an integral sampling rate of at least 1 Hz");
    }
    const size_t sample_count = chan->samples();
    if (sample_count == 0) {
      throw std::runtime_error("PST channel contains no samples: " + chan->get_atss_filepath().string());
    }

    const auto source = chan->get_json_filepath().string();
    if (!source_files.insert(source).second) {
      throw std::runtime_error("PST selection contains a duplicate channel: " + source);
    }

    const auto sensor = sensor_id(chan);
    sensors.insert(sensor);
    const interval_key interval{sample_rate, chan->start_datetime(), sample_count};
    if (!interval_sensors[interval].insert(sensor).second) {
      throw std::runtime_error("PST interval contains the same physical sensor more than once: " + sensor.label());
    }
  }

  if (sensors.size() < 2) {
    throw std::runtime_error("Select at least two electric or magnetic sensors");
  }
  for (const auto &[interval, present_sensors] : interval_sensors) {
    if (present_sensors != sensors) {
      throw std::runtime_error("Selected channels are not isochronous across every PST sensor");
    }
  }
}

pst_spectrum_band pst_spectra_processor::process_channel(const std::shared_ptr<channel> &chan) {
  const auto frequencies = chan->fft_freqs->get_frequencies();
  const bool has_calibration = chan->cal != nullptr && !chan->cal->is_empty();
  if (has_calibration) {
    chan->cal->restore();
    chan->cal->interpolate(frequencies);
  }
  if (chan->is_type("H")) {
    chan->cal->gen_cal_sensor(frequencies);
    chan->cal->join_lower_theo_and_measured_interpolated();
  }
  chan->read_all_fftw(false, nullptr);

  pst_spectrum_band band;
  band.sensor = sensor_id(chan);
  band.station = chan->get_station_name().string();
  band.run = chan->get_run();
  band.sample_rate = chan->get_sample_rate();
  band.frequencies = frequencies;
  if (has_calibration || chan->is_type("H")) {
    band.amplitudes = chan->atss_simple_stack_all_calibrated(chan->fft_freqs);
  } else {
    const auto stacked = chan->atss_simple_stack_all(chan->fft_freqs);
    band.amplitudes.reserve(stacked.size());
    for (const auto &value : stacked) {
      band.amplitudes.push_back(std::abs(value));
    }
  }
  band.window_count = chan->fft_freqs->get_raw_stacks();
  return band;
}

std::vector<pst_sensor_spectrum> pst_spectra_processor::merge_bands(const std::vector<pst_spectrum_band> &bands) {
  struct weighted_value {
    double sum = 0.0;
    size_t count = 0;
  };
  struct sensor_accumulator {
    std::set<std::string> stations;
    size_t total_windows = 0;
    std::map<double, weighted_value> frequencies;
  };

  std::map<pst_sensor_id, sensor_accumulator> accumulators;
  for (const auto &band : bands) {
    if (band.window_count == 0 || band.frequencies.size() != band.amplitudes.size()) {
      throw std::runtime_error("Invalid PST spectrum band for " + band.sensor.label());
    }
    auto &sensor = accumulators[band.sensor];
    sensor.stations.insert(band.station);
    sensor.total_windows += band.window_count;
    for (size_t i = 0; i < band.frequencies.size(); ++i) {
      const double frequency = band.frequencies[i];
      const double amplitude = band.amplitudes[i];
      if (!std::isfinite(frequency) || !std::isfinite(amplitude) || frequency <= 0.0 || amplitude <= 0.0) {
        continue;
      }
      auto &value = sensor.frequencies[frequency];
      value.sum += amplitude * static_cast<double>(band.window_count);
      value.count += band.window_count;
    }
  }

  std::vector<pst_sensor_spectrum> result;
  result.reserve(accumulators.size());
  for (auto &[sensor_id, accumulator] : accumulators) {
    pst_sensor_spectrum spectrum;
    spectrum.sensor = sensor_id;
    spectrum.stations = std::move(accumulator.stations);
    spectrum.total_windows = accumulator.total_windows;
    spectrum.frequencies.reserve(accumulator.frequencies.size());
    spectrum.amplitudes.reserve(accumulator.frequencies.size());
    for (const auto &[frequency, value] : accumulator.frequencies) {
      spectrum.frequencies.push_back(frequency);
      spectrum.amplitudes.push_back(value.sum / static_cast<double>(value.count));
    }
    if (!spectrum.frequencies.empty()) {
      result.push_back(std::move(spectrum));
    }
  }
  return result;
}

std::vector<pst_sensor_spectrum> pst_spectra_processor::process(const std::vector<std::shared_ptr<channel>> &selected_channels) const {
  validate_parallel_test(selected_channels);

  for (const auto &chan : selected_channels) {
    const size_t window_length = static_cast<size_t>(std::llround(chan->get_sample_rate()));
    chan->init_fftw(nullptr, false, window_length, window_length);
    chan->fft_freqs->auto_range(0.05, 0.5);
  }

  std::vector<std::future<pst_spectrum_band>> futures;
  futures.reserve(selected_channels.size());
  for (const auto &chan : selected_channels) {
    futures.push_back(this->pool->submit_task([chan]() { return process_channel(chan); }));
  }

  std::vector<pst_spectrum_band> bands;
  bands.reserve(futures.size());
  for (auto &future : futures) {
    bands.push_back(future.get());
  }
  return merge_bands(bands);
}
