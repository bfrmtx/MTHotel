#ifndef SPC_BASE_HPP
#define SPC_BASE_HPP
#include <algorithm>
#include <complex>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "cal_base.hpp"
#include "channel.hpp"
#include "files_dirs.hpp"
#include "freqs.hpp"
#include "mt_base.hpp"

/// @brief A class template for managing spectra. T is at least a std::vector<T> or std::vector<std::vector<T>>.
///
/// <Ex, null> std::vector<std::vector<std::complex<double>> is the raw unstacked spectra for Ex.
/// <Hx, null> std::vector<std::vector<std::complex<double>> is the raw unstacked spectra for Hx
/// <...> and so on for the other channels.
///
/// YOU MAY HAVE - but this should be for (parzened)smoothed spectra
/// <Ex, Hy> std::vector<std::vector<std::complex<double>> that is a 1:1 multiplication.
///
/// <Ex, Ex> std::vector<double> stacked auto spectra (real)
/// <Ex, Hy> std::vector<double> stacked cross spectra (real) ... complex makes no sense!

/// @def spc<double> stores a std::shared_ptr<std::vector<T>>, e.g. stacked results
template <typename T>
struct is_std_vector : std::false_type {};

/// @def spc<std::vector<std::complex<double>>> stores a std::shared_ptr<std::vector<std::complex<double>>>, so a vector of shared pointers, each pointing to a vector of complex<double>.
/// @brief Template specialization that identifies std::vector types
///
/// This is a template specialization of the `is_std_vector` type trait that matches
/// any instantiation of `std::vector<U, Alloc>` where U is any element type and Alloc
/// is any allocator type.
///
/// @details
/// The primary `is_std_vector` template is typically defined as `std::false_type` for
/// non-vector types. This specialization overrides it to be `std::true_type` specifically
/// for `std::vector` instances.
///
/// This is a SFINAE (Substitution Failure Is Not An Error) technique that enables
/// compile-time type checking. By deriving from `std::true_type`, it provides:
/// - A nested `value` constant equal to `true`
/// - A `type` alias equal to `std::true_type`
///
/// This allows code to distinguish between `std::vector` types and other types at
/// compile time using `is_std_vector<T>::value` in template conditions, `if constexpr`,
/// or as SFINAE constraints.
///
/// @tparam U The element type stored in the vector
/// @tparam Alloc The allocator type used by the vector (defaults to std::allocator<U>)
///
/// @example
/// @code
/// is_std_vector<std::vector<int>>::value        // true
/// is_std_vector<std::vector<double>>::value     // true
/// is_std_vector<int>::value                     // false
/// @endcode
template <typename U, typename Alloc>
struct is_std_vector<std::vector<U, Alloc>> : std::true_type {};

// T can be std::vector<double>, std::vector<std::complex<double>>, std::vector<std::vector<double>>, std::vector<std::vector<std::complex<double>>>
// first channel can never be null!
// vector can never be null!
template <typename T>
/// @class spc_base
/// @brief Template base class for storing and managing spectral data associated with channel pairs.
///
/// @tparam T The type of spectral data stored. Can be:
///           - std::vector<double> for real amplitude spectra (uses .datfa extension)
///           - std::vector<std::complex<double>> for complex spectra (uses .datfc extension)
///           - std::vector<std::vector<T>> for multiple time slices of spectra
///
/// @details This class inherits from std::map where:
///          - Key: pair of channel shared pointers (chan1, chan2)
///            - For single channel spectra: (chan1, nullptr)
///            - For auto spectra: (chan1, chan1)
///            - For cross spectra: (chan1, chan2) where chan1 ≠ chan2
///          - Value: shared pointer to vector of spectral data
///
/// @note Thread-safe: All public methods use read/write locks via std::shared_mutex
/// @note The class supports both stacked (averaged) spectra and time-sliced spectra
/// @note Channel order matters for cross spectra: <Hx,Hy> is treated differently from <Hy,Hx>
///
/// @section usage Usage Examples
/// @code
/// // Create a spectra container for complex data
/// spc_base<std::vector<std::complex<double>>> spectra;
///
/// // Add a spectrum
/// auto chan1 = std::make_shared<channel>("Ex", false, false);
/// auto data = std::make_shared<std::vector<std::complex<double>>>(1024);
/// spectra.add_spectra(chan1, nullptr, data);
///
/// // Generate auto and cross spectra channel pairs
/// auto auto_pairs = spectra.generate_auto_spectra_channels();
/// auto cross_pairs = spectra.generate_cross_spectra_channels();
/// @endcode
///
/// @see channel
/// @see fftw_freqs
class spc_base : public std::map<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>, std::shared_ptr<std::vector<T>>> {
public:
  spc_base() : std::map<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>, std::shared_ptr<std::vector<T>>>() {
    // if is type of T is double ascii extension is .datfa
    // if is type of T is std::complex<double> ascii extension is .datfc
    // else we have no extension (for example if T = std::vector<double> or std::vector<std::complex<double>>)
    //
    if constexpr (std::is_same_v<T, double>) {
      ascii_extension = ".datfa";
    } else if constexpr (std::is_same_v<T, std::complex<double>>) {
      ascii_extension = ".datfc";
    } else {
      ascii_extension = std::filesystem::path("");
    }
  }
  ~spc_base() = default;

  // ADD OR MOVE SPECTRA with V E C T O R   D A T A

  /// @brief wrapper for @ref add_spectra_no_lock that acquires the mutex for writing
  ///
  void add_spectra(const std::shared_ptr<channel> &chan1, const std::shared_ptr<channel> &chan2, std::shared_ptr<std::vector<T>> spectra, const bool move_spc = false) {
    std::unique_lock lock(mutex_);
    add_spectra_no_lock(chan1, chan2, spectra, move_spc);
  }

  /// @brief  overload for two strings
  /// @param name_in_1 creates channel 1
  /// @param is_remote creates channel 1
  /// @param is_emap creates channel 1
  /// @param name_in_2 creates channel 2 if not EMPTY
  /// @param is_remote_2 creates channel 2 if not EMPTY
  /// @param is_emap_2 creates channel 2 if not EMPTY
  /// @param spectra shared pointer to the spectra vector
  /// @param move_spc move instead of copy
  void add_spectra(const std::string &name_in_1, const bool is_remote, const bool is_emap, const std::string &name_in_2, const bool is_remote_2, const bool is_emap_2, std::shared_ptr<std::vector<T>> spectra, const bool move_spc = false) {
    std::unique_lock lock(mutex_);
    auto chan1 = std::make_shared<channel>(name_in_1, is_remote, is_emap);
    std::shared_ptr<channel> chan2 = nullptr;
    if (!name_in_2.empty()) {
      chan2 = std::make_shared<channel>(name_in_2, is_remote_2, is_emap_2);
    }
    add_spectra_no_lock(chan1, chan2, spectra, move_spc);
  }

  /// @brief moves a spectra object OUT OFF A CHANNEL; is is e.g. of type std::vector<std::complex<double>>.
  /// @param chan1
  void move_spectra(const std::shared_ptr<channel> &chan1) {
    std::unique_lock lock(mutex_);
    if (chan1 == nullptr) {
      throw std::runtime_error("spc_base::move_spectra: chan1 is nullptr");
    }
    if (chan1->spc.size() == 0) {
      throw std::runtime_error("spc_base::move_spectra: chan1 has no spectra to move");
    }
    // now we move the spectra from the channel into a std::shared_ptr<std::vector<T>>; we need to move the spectra, so we create a new shared pointer and move the vector into it; we also need to move the bandwidth
    std::shared_ptr<std::vector<T>> spectra = std::make_shared<std::vector<T>>(std::move(chan1->spc));
    add_spectra_no_lock(chan1, nullptr, spectra, true); // we move
  }

  // P R E P A R E   S P E C T R A   and CREATE an empty vector to be filled from outside

  /// @brief prepares an auto (self-paired) or cross (two different channels) spectrum; CREATES an EMPTY VECTOR
  /// @param chan_pair (same, same) for auto, (same, different) for cross; ORDER DOES MATTER!!!
  /// @details in the sense of math the order may not matter, when finding a mane <Hx,Hy> is different from <Hy,Hx>.
  /// @details this is most likely a target to be filled as a result of multiplication or stacking.
  void prepare_ac_cross_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> chan_pair) {
    std::unique_lock lock(mutex_);
    auto chan1 = chan_pair.first;
    auto chan2 = chan_pair.second;
    // check for null pointers
    if (chan1 == nullptr) {
      throw std::runtime_error("spc_base::prepare_ac_cross_spectra: channel 1 is nullptr");
    }
    if (chan2 == nullptr) {
      throw std::runtime_error("spc_base::prepare_ac_cross_spectra: channel 2 is nullptr");
    }
    if (can_add_no_lock(chan1, chan2)) {
      this->emplace(chan_pair, std::make_shared<std::vector<T>>()); // we create an empty vector for the spectra to be filled from outside
    } else {
      throw std::runtime_error("spc_base::prepare_ac_cross_spectra: a spectrum with the same channel(s) already exists");
    }
  }

  /// @brief get a vector of all possible auto spectra channels, that is all channels self-paired
  /// @details this is most likely a source spectra
  /// @param verbose
  /// @return
  std::vector<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> generate_auto_spectra_channels(const bool verbose = false) const {
    std::shared_lock lock(mutex_);
    // get all unique channels from ch_map and may self-paired
    std::vector<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> auto_spectra_channels;
    for (const auto &entry : *this) {
      const auto &existing_chan1 = entry.first.first;
      const auto &existing_chan2 = entry.first.second;
      if (existing_chan1 == nullptr) {
        throw std::runtime_error("spc_base::generate_auto_spectra_channels: channel 1 is nullptr");
      }
      // second must be nullptr for auto spectra, otherwise we skip it
      if (existing_chan2 != nullptr) {
        continue;
      }
      auto_spectra_channels.emplace_back(std::make_pair(existing_chan1, existing_chan1)); // we self-pair the channel for auto spectra
      if (verbose) {
        std::cout << "spc_base::generate_auto_spectra_channels: prepared auto spectra " << existing_chan1->get_channel_type(true) << ", " << existing_chan1->get_channel_type(true) << std::endl;
      }
    }
    return auto_spectra_channels;
  }

  /// @brief get a vector of all possible cross spectra channels, that is all combinations of two different channels; we do not include self-paired channels; ORDER DOES MATTER!!!
  /// @details in the sense of math the order may not matter, when finding a mane <Hx,Hy> is different from <Hy,Hx>.
  /// @details this is most likely a source spectra
  /// @param verbose
  /// @return
  std::vector<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> generate_cross_spectra_channels(const bool verbose = false) const {
    std::shared_lock lock(mutex_);
    // get all unique channels from ch_map and may  cross paired, NOT self-paired
    std::vector<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> cross_spectra_channels;
    std::vector<std::shared_ptr<channel>> unique_channels;
    for (const auto &entry : *this) {
      if (entry.first.second == nullptr) {
        unique_channels.push_back(entry.first.first);
      }
    }

    for (size_t i = 0; i < unique_channels.size() - 1; ++i) {
      for (size_t j = i + 1; j < unique_channels.size(); ++j) {
        cross_spectra_channels.emplace_back(std::make_pair(unique_channels[i], unique_channels[j]));
        if (verbose) {
          std::cout << "spc_base::generate_cross_spectra_channels: prepared cross spectra "
                    << unique_channels[i]->get_channel_type(true) << ", "
                    << unique_channels[j]->get_channel_type(true) << std::endl;
        }
      }
    }
    return cross_spectra_channels;
  }

  // find etc

  std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> find_channel(const std::string &channel_type, const bool is_remote = false, const bool is_emap = false) const {
    std::shared_lock lock(mutex_);
    for (const auto &entry : *this) {
      const auto &existing_chan1 = entry.first.first;
      // we must always have a first channel.
      if (existing_chan1 == nullptr) {
        throw std::runtime_error("spc_base::find_channel: channel 1 is nullptr");
      }
      const auto &existing_chan2 = entry.first.second;
      if (existing_chan2 != nullptr) {
        continue; // we only look for single channels, so second must be nullptr
      }
      if (existing_chan1->get_channel_type() == channel_type && existing_chan1->is_remote == is_remote && existing_chan1->is_emap == is_emap) {
        return std::make_pair(existing_chan1, nullptr); // we return the channel and nullptr for the second channel, because we are looking for single channels
      }
    }
    return std::make_pair(nullptr, nullptr); // not found
  }

  std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> find_channel(const std::string &channel_type, const bool is_remote, const bool is_emap, const std::string &channel_type_2, const bool is_remote_2, const bool is_emap_2) const {
    std::shared_lock lock(mutex_);
    for (const auto &entry : *this) {
      const auto &existing_chan1 = entry.first.first;
      if (existing_chan1 == nullptr) {
        throw std::runtime_error("spc_base::find_channel: channel 1 is nullptr");
      }
      const auto &existing_chan2 = entry.first.second;
      if (existing_chan2 == nullptr) {
        throw std::runtime_error("spc_base::find_channel: channel 2 is nullptr");
      }
      if (existing_chan1->get_channel_type() == channel_type && existing_chan1->is_remote == is_remote && existing_chan1->is_emap == is_emap) {
        if (existing_chan2->get_channel_type() == channel_type_2 && existing_chan2->is_remote == is_remote_2 && existing_chan2->is_emap == is_emap_2) {
          return std::make_pair(existing_chan1, existing_chan2); // we return both channels, they are both needed for the spectrum
        }
      }
    }
    return std::make_pair(nullptr, nullptr); // not found
  }

  // ******************************************************  R E T R I E V I N G   S P E C T R A

  std::vector<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> get_all_channel_pairs() const {
    std::shared_lock lock(mutex_);
    std::vector<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> channel_pairs;
    for (const auto &entry : *this) {
      channel_pairs.push_back(entry.first);
    }
    if (channel_pairs.empty()) {
      std::cerr << "spc_base::get_all_channel_pairs: no spectra found, returning empty vector" << std::endl;
    }
    return channel_pairs;
  }

  /// @brief Retrieve the spectra vector for a given channel pair. It also can be <Ex, null> for single channel spectra
  /// @param chan_pair Pair of channels to search for.
  /// @return A shared pointer to the spectra vector.
  std::shared_ptr<std::vector<T>> get_spectra(std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> chan_pair) const {
    std::shared_lock lock(mutex_);
    return get_spectra_no_lock(chan_pair);
  }
  std::vector<T> get_spectra_vec(std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> chan_pair) const {
    std::shared_lock lock(mutex_);
    // Only reject vector<vector<...>>, allow everything else (simple types or single-level vectors)
    if constexpr (is_std_vector<T>::value) {
      using inner_type = typename T::value_type;
      if constexpr (is_std_vector<inner_type>::value) {
        throw std::runtime_error("spc_base::get_spectra_vec: T cannot be vector<vector<...>>");
      }
    }
    auto spc_ptr = get_spectra_no_lock(chan_pair);
    if (spc_ptr != nullptr) {
      return *spc_ptr;
    } else {
      throw std::runtime_error("spc_base::get_spectra_vec: spectrum with the given channel pair not found");
    }
    return std::vector<T>(); // should never reach here
  }

  // D E L E T I N G   S P E C T R A
  void delete_spectra(std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> chan_pair) {
    std::unique_lock lock(mutex_);
    auto it = this->find(chan_pair);
    if (it != this->end()) {
      this->erase(it);
    } else {
      std::cerr << "spc_base::delete_spectra: spectrum with the given channel pair not found, cannot delete" << std::endl;
    }
  }

  /// @brief Check if the type T is a vector. This is true if T is std::vector<U> for some U, but not if T is std::vector<std::vector<U>>.
  /// @return true if T is a vector of non-vector type, false otherwise, a simple vector<T>
  bool is_vector() const {
    std::shared_lock lock(mutex_);
    return is_vector_no_lock();
  }

  /// @brief Check if the type T is a vector of vectors. This is true if T is std::vector<std::vector<U>> for some U.
  /// @return true if T is a vector of vectors, false otherwise.
  bool is_vector_of_vectors() const {
    std::shared_lock lock(mutex_);
    return is_vector_of_vectors_no_lock();
  }

  /// @brief a) get the amount of spectra types like <Ex,Ey> ... <Hy,Hz>, b) Get the size of the stacked spectra, like 1024 or 4096 which is also internally paired to the frequencies.
  /// @details use the first number also for unstacked raw spectra ... the convention is also like <Ex, null> or <Hx, Hz>. Take the FIRST number only!
  /// @return a pair of the size of the map (number of spectra types) and the size of the vector (number of frequencies)
  std::pair<size_t, size_t> get_size_stacked() const {
    std::shared_lock lock(mutex_);
    if (this->empty()) {
      return std::make_pair(0, 0);
    }
    // this function works only for a standard vector.
    if (is_vector_of_vectors_no_lock()) {
      throw std::runtime_error("spc_base::get_size_stacked: T is a vector of vectors, use get_size_slices instead");
    }
    return std::make_pair(this->size(), this->begin()->second->size());
  }

  /// @brief a) get the size of the outer vector, that is for example 60 stacks for 60 time segments, and inner size, that is for example 1024 frequencies; throw an exception if the data type is not a vector of vectors
  /// @return number of outer slices (nstacks) and inner size (number of frequencies)
  std::pair<size_t, size_t> get_size_slices() const {
    std::shared_lock lock(mutex_);
    if (this->empty()) {
      return std::make_pair(0, 0);
    }
    // check if T is a vector of vectors, that is std::vector<std::vector<T>>
    // we start to check if this is a vector at all
    if (is_vector_no_lock()) {
      throw std::runtime_error("spc_base::get_size_slices: T is a vector, not a vector of vectors, use get_size_stacked instead");
    }
    // so for example 60 slices of 1024.
    return std::make_pair(this->begin()->second->size(), this->begin()->second->at(0).size());
  }

  std::shared_ptr<fftw_freqs> load_from_rundir(const std::filesystem::path &top_dir, const std::string &expected_extension) {
    std::unique_lock lock(mutex_);
    // typically, we would have data inside the run_NNN directory:
    // 033_ADU-11e_C002_THx_131072Hz.json
    // 033_ADU-11e_C002_THx_131072Hz__033_ADU-11e_C002_THx_131072Hz.datfa
    // 033_ADU-11e_C003_THy_131072Hz.json
    // 033_ADU-11e_C003_THy_131072Hz__033_ADU-11e_C003_THy_131072Hz.datfa
    // 033_ADU-11e_C004_THz_131072Hz.json
    // 033_ADU-11e_C004_THz_131072Hz__033_ADU-11e_C004_THz_131072Hz.datfa

    if (!std::filesystem::exists(top_dir) || !std::filesystem::is_directory(top_dir)) {
      throw std::runtime_error("spc_base::load_from_rundir: directory " + top_dir.string() + " does not exist or is not a directory");
    }
    this->ascii_extension = expected_extension;
    std::shared_ptr<fftw_freqs> freqs_read;

    for (const auto &entry : std::filesystem::directory_iterator(top_dir)) {
      if (entry.is_regular_file()) {
        const auto &path = entry.path();
        if (path.extension() == expected_extension) {
          // we need to parse the filename to get the channel information; we expect the filename to be in the format: chan1__chan2.extension for cross spectra and auto spectra (chan1__chan1.extension)
          // single spectra are not supported yet.
          std::string filename = path.stem().string(); // get the filename without extension
          // keep the data file
          auto data_file = path;
          size_t delim_pos = filename.find("__");
          if (delim_pos == std::string::npos) {
            throw std::runtime_error("spc_base::load_from_rundir: filename " + filename + " does not contain the expected delimiter '__'");
          }
          std::string chan1_str = filename.substr(0, delim_pos);
          std::string chan2_str = filename.substr(delim_pos + 2);
          // we need to find the channels in the existing map, if they do not exist, so we need to load the channel information from the corresponding json files; we expect the json files to be in the format: chan1.json and chan2.json
          auto json_1 = std::filesystem::path(top_dir) / (chan1_str + ".json");
          auto json_2 = std::filesystem::path(top_dir) / (chan2_str + ".json");
          if (!std::filesystem::exists(json_1) || !std::filesystem::is_regular_file(json_1)) {
            throw std::runtime_error("spc_base::load_from_rundir: json file " + json_1.string() + " does not exist or is not a regular file");
          }
          if (!std::filesystem::exists(json_2) || !std::filesystem::is_regular_file(json_2)) {
            throw std::runtime_error("spc_base::load_from_rundir: json file " + json_2.string() + " does not exist or is not a regular file");
          }
          auto chan1 = std::make_shared<channel>(json_1);
          auto chan2 = std::make_shared<channel>(json_2);
          if (this->can_add_no_lock(chan1, chan2)) {
            // we can add the spectrum, so we load the data from the ascii file and add it to the map
            std::shared_ptr<std::vector<double>> spectra_f;
            std::shared_ptr<std::vector<T>> spectra_d;
            fdirs::read_ascii_f_ptr<T>(data_file, spectra_f, spectra_d);
            this->emplace(std::make_pair(chan1, chan2), spectra_d);
          } else {
            throw std::runtime_error("spc_base::load_from_rundir: a spectrum with the same channel pair already exists, cannot load " + filename);
          }
          // read the fftw_freqs from the json file; we expect the json file to be in the format: fft_freqs.json
          auto fft_freqs_file = std::filesystem::path(top_dir) / "fft_freqs.json";
          if (!std::filesystem::exists(fft_freqs_file) || !std::filesystem::is_regular_file(fft_freqs_file)) {
            throw std::runtime_error("spc_base::load_from_rundir: fft_freqs json file " + fft_freqs_file.string() + " does not exist or is not a regular file");
          }
          if (freqs_read == nullptr) {
            freqs_read = std::make_shared<fftw_freqs>(fft_freqs_file);
          }
        }
      }
    }
    return freqs_read;
  }
  void info() const {
    std::shared_lock lock(mutex_);
    std::cout << "spc_base::info: number of spectra types: " << this->size() << std::endl;
    for (const auto &entry : *this) {
      const auto &chan1 = entry.first.first;
      const auto &chan2 = entry.first.second;
      std::cout << "  spectrum type: <" << chan1->get_channel_type(true) << ", " << (chan2 != nullptr ? chan2->get_channel_type(true) : "null") << ">, size: " << entry.second->size() << chan1->get_sample_rate_str() << std::endl;
    }
  }

  /// @brief save e.g. stacked amplitude spectra to ascii files.
  /// @param top_dir e.g. the run_NNN directory BELOW the station directory
  /// @param create create e.g. the run_NNN directory if it does not exist. Normally, the directory should already exist, because that is a tree.
  void save_ascii(const std::filesystem::path &top_dir, const bool create = false) const {
    std::shared_lock lock(mutex_);
    if (is_vector_of_vectors_no_lock()) {
      throw std::runtime_error("spc_base::save_ascii: T is a vector of vectors, cannot save as ascii");
    }
    if (this->empty()) {
      std::cerr << "spc_base::save_ascii: no spectra to save" << std::endl;
      return;
    }
    if (this->ascii_extension.empty()) {
      throw std::runtime_error("spc_base::save_ascii: ascii extension is empty, cannot save");
    }
    if (create) {
      std::filesystem::create_directories(top_dir);
    }
    if (!std::filesystem::exists(top_dir) || !std::filesystem::is_directory(top_dir)) {
      throw std::runtime_error("spc_base::save_ascii: directory " + top_dir.string() + " does not exist / can not be created");
    }
    for (const auto &entry : *this) {
      std::filesystem::path filename = top_dir;
      const auto &chan1 = entry.first.first;
      if (chan1 == nullptr) {
        throw std::runtime_error("spc_base::save_ascii: channel 1 is nullptr");
      }
      if (chan1->fft_freqs == nullptr) {
        throw std::runtime_error("spc_base::save_ascii: channel 1 has no fft_freqs, cannot save");
      }
      if (entry.second == nullptr) {
        throw std::runtime_error("spc_base::save_ascii: spectra vector is nullptr, cannot save");
      }
      const auto &chan2 = entry.first.second;
      std::shared_ptr<fftw_freqs> fft_freqs;
      if (chan1 != nullptr) {
        chan1->set_dir(top_dir);
        chan1->write_header();
        filename /= chan1->get_filepath_wo_ext().filename();
        if (fft_freqs == nullptr) {
          fft_freqs = chan1->fft_freqs;
        }
      }
      if (chan2 != nullptr) {
        chan2->set_dir(top_dir);
        if (chan1 != chan2) {
          chan2->write_header();
        }
        filename += "__" + chan2->get_filepath_wo_ext().filename().string();
      }
      filename += this->ascii_extension; // e.g. .datfa or .datfc
      std::filesystem::path out_file = top_dir / filename;
      auto freqs = fft_freqs->get_frequencies();
      // we save the spectra vector to a file, we need to dereference the shared pointer to get the vector
      fdirs::save_vector_to_ascii_f(out_file, freqs, *entry.second);
      // we also need to save the fft_freqs to a json.
      // since the freqs are all the same, we call it fft_freq.json
      std::filesystem::path fft_freq_file = top_dir / "fft_freqs.json";
      fft_freqs->save(fft_freq_file);
    }
  }
  /// @brief Set the ASCII file extension for saving spectra.
  /// @param extension The file extension to set (e.g., "datfa" or "datfc")
  void set_ascii_extension(const std::string &extension) {
    std::unique_lock lock(mutex_);
    if (extension.empty()) {
      this->ascii_extension.clear();
      return;
    }
    if (extension.front() == '.') {
      this->ascii_extension = extension;
    } else {
      this->ascii_extension = "." + extension;
    }
  }

  std::string get_ascii_extension() const {
    std::shared_lock lock(mutex_);
    return this->ascii_extension.string();
  }

  bool can_add(const std::shared_ptr<channel> &chan1, const std::shared_ptr<channel> &chan2) const {
    std::shared_lock lock(mutex_);
    return can_add_no_lock(chan1, chan2);
  }

private:
  mutable std::shared_mutex mutex_;      ///< Mutex for thread-safe access to the spectra map.
  std::filesystem::path ascii_extension; ///< The file extension for ASCII files (e.g., ".datfa" or ".datfc").

  /// @brief Add spectra to the map without acquiring the mutex. This function assumes that the caller has already acquired the appropriate lock (unique_lock for writing).
  /// @param chan1 The first channel.
  /// @param chan2 The second channel.
  /// @param spectra The spectra to add.
  /// @param move_spc Whether to move the spectra. If true, the input shared pointer will be reset after moving the spectra into the map.
  /// @throws std::runtime_error if chan1 is nullptr, if spectra is not a shared pointer to a vector, or if a spectrum with the same channel(s) already exists in the map.
  void add_spectra_no_lock(const std::shared_ptr<channel> &chan1, const std::shared_ptr<channel> &chan2, std::shared_ptr<std::vector<T>> spectra, const bool move_spc) {
    if (chan1 == nullptr) {
      throw std::runtime_error("spc_base::add_spectra: chan1 is nullptr");
    }
    if (spectra != nullptr && !is_std_vector<T>::value) {
      throw std::runtime_error("spc_base::add_spectra: spectra is not a shared pointer to a vector");
    }
    if (can_add_no_lock(chan1, chan2)) {
      this->emplace(std::make_pair(chan1, chan2), spectra);
      if (spectra != nullptr) {
        if (move_spc) {
          spectra.reset(); // we moved the spectra, so we reset the input pointer
        }
      }
    } else {
      throw std::runtime_error("spc_base::add_spectra: a spectrum with the same channel(s) already exists");
    }
  }

  bool can_add_no_lock(const std::shared_ptr<channel> &chan1, const std::shared_ptr<channel> &chan2) const {
    if (chan1 == nullptr) {
      throw std::runtime_error("spc_base::can_add: chan1 is nullptr");
    }
    // iterate over the map to check if a channel with the same type, is_remote and is_emap already exists
    for (const auto &entry : *this) {
      const auto &existing_chan1 = entry.first.first;
      const auto &existing_chan2 = entry.first.second;
      // case one: chan1 is the same as existing_chan1 (compare channel type, is_remote and is_emap)
      // AND chan2 and existing_chan2 are both null or both not null and the same (compare channel type, is_remote and is_emap)
      if (chan1->channel_type == existing_chan1->channel_type && chan1->is_remote == existing_chan1->is_remote && chan1->is_emap == existing_chan1->is_emap) {
        if ((chan2 == nullptr && existing_chan2 == nullptr) ||
            (chan2 != nullptr && existing_chan2 != nullptr && chan2->channel_type == existing_chan2->channel_type && chan2->is_remote == existing_chan2->is_remote && chan2->is_emap == existing_chan2->is_emap)) {
          return false;
        }
      }
    }
    return true;
  }

  std::shared_ptr<std::vector<T>> get_spectra_no_lock(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
    auto it = this->find(chan_pair);
    if (it != this->end()) {
      return it->second;
    }
    throw std::runtime_error("spc_base::get_spectra: spectrum with the given channel pair not found");
  }

  bool is_vector_no_lock() const {
    if (!is_std_vector<T>::value) { // is a vector in general.
      return false;
    }
    using inner_type = typename T::value_type; // vector<T> has a value_type that is T. and the inner type is NOT a vector.
    return !is_std_vector<inner_type>::value;
  }

  bool is_vector_of_vectors_no_lock() const {
    if (!is_std_vector<T>::value) { // is a vector in general.
      return false;
    }
    if constexpr (is_std_vector<T>::value) {
      using inner_type = typename T::value_type; // get again the inner type
      return is_std_vector<inner_type>::value;   // is the inner type also a vector? if yes, then this is a vector of vectors.
    }
    return false; // should never reach here
  }

  /// @brief find a spectrum by channel pair
  /// @param chan_pair pair of channels to search for
  /// @return const_iterator to the map entry, or end() if not found
  typename std::map<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>, std::shared_ptr<std::vector<T>>>::const_iterator find(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
    for (auto it = this->begin(); it != this->end(); ++it) {
      const auto &existing_chan1 = it->first.first;
      const auto &existing_chan2 = it->first.second;
      if (existing_chan1 == nullptr) {
        return this->end();
      }
      if (existing_chan1->channel_type == chan_pair.first->channel_type && existing_chan1->is_remote == chan_pair.first->is_remote && existing_chan1->is_emap == chan_pair.first->is_emap) {
        if ((chan_pair.second == nullptr && existing_chan2 == nullptr) ||
            (chan_pair.second != nullptr && existing_chan2 != nullptr && existing_chan2->channel_type == chan_pair.second->channel_type && existing_chan2->is_remote == chan_pair.second->is_remote && existing_chan2->is_emap == chan_pair.second->is_emap)) {
          return it;
        }
      }
    }
    return this->end();
  }
};

// Utility functions for channel pair classification
inline bool is_auto_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) {
  // Auto spectrum: both channels point to the same object
  return (chan_pair.first != nullptr && chan_pair.second != nullptr && chan_pair.first == chan_pair.second);
}

inline bool is_cross_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) {
  // Cross spectrum: two different channels
  return (chan_pair.first != nullptr && chan_pair.second != nullptr && chan_pair.first != chan_pair.second);
}

inline bool is_single_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) {
  // Single spectrum: only one channel (second is nullptr)
  return (chan_pair.first != nullptr && chan_pair.second == nullptr);
}

#endif // SPC_BASE_HPP