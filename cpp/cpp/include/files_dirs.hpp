#ifndef FILES_DIRS_HPP
#define FILES_DIRS_HPP

#include "strings_etc.hpp"
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
// code snipp recursive copy
// try
//{
//    std::filesystem::copy(src, target, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
//}
// catch (std::exception& e)
//{
//    std::cout << e.what();
//}

// code snipp recursive predicate from strackoverflow
// Copy only those files which contain "Sub" in their stem.
// const auto filter = [](const std::filesystem::path& p) -> bool
//{
//    return p.stem().generic_string().find("Sub") != std::string::npos;
//};

// from stackoverflow
// Recursively copies those files and folders from src to target which matches
// predicate, and overwrites existing files in target.

namespace fdirs {

const auto no_ats_filter = [](const std::filesystem::path &p) -> bool {
  return p.stem().generic_string().find("Sub") != std::string::npos;
};

const auto no_measdir_filter = [](const std::filesystem::path &p) -> bool {
  return p.stem().generic_string().find("Sub") != std::string::npos;
};

/// @brief
/// @tparam T in or so
/// @param v vector like 1 3 4 5
/// @return first gap, that would be 2 in this case
template <typename T>
T first_gap(std::vector<T> &v) {
  // Handle the special case of an empty vector.  Return 1.
  if (v.empty())
    return T(1);

  // Sort the vector
  std::sort(v.begin(), v.end());

  // Find the first adjacent pair that differ by more than 1.
  auto i = std::adjacent_find(v.begin(), v.end(), [](int l, int r) { return l + 1 < r; });

  // Handle the special case of no gaps.  Return the last value + 1.
  if (i == v.end())
    --i;

  return 1 + *i;
}

/// \brief scan_runs returns the first free number of run_001 ... run_003, run_004
/// \param station_dir
/// \return here it would be 2
inline size_t scan_runs(const std::filesystem::path &station_dir) {
  std::vector<size_t> iruns;
  for (const auto &entry : std::filesystem::directory_iterator(station_dir)) {
    // std::cout << entry.path() << std::endl;
    if (std::filesystem::is_directory(entry)) {
      auto i = mstr::string2run(entry.path().filename().string());
      if (i != SIZE_MAX)
        iruns.push_back(i);
      else
        return SIZE_MAX;
    }
  }
  return fdirs::first_gap(iruns);
}

/// @brief return the highest run number in the station directory + 1
/// @param station_dir
/// @return
inline size_t scan_runs_high(const std::filesystem::path &station_dir) {
  std::vector<size_t> iruns;
  for (const auto &entry : std::filesystem::directory_iterator(station_dir)) {
    // std::cout << entry.path() << std::endl;
    if (std::filesystem::is_directory(entry)) {
      auto i = mstr::string2run(entry.path().filename().string());
      if (i != SIZE_MAX)
        iruns.push_back(i);
      else
        return SIZE_MAX;
    }
  }
  if (iruns.empty())
    return 1;
  auto last = *std::max_element(iruns.begin(), iruns.end());
  return last + 1;
}

inline std::filesystem::path meta_dir(const std::filesystem::path &dir) {
  std::filesystem::path p;
  for (const auto &e : dir) {
    if (e == "stations")
      p /= "meta";
    else
      p /= e;
  }
  return p;
}

inline void CopyRecursive(const std::filesystem::path &src, const std::filesystem::path &target,
                          const std::function<bool(std::filesystem::path)> &predicate /* or use template */) noexcept {
  try {
    for (const auto &dirEntry : std::filesystem::recursive_directory_iterator(src)) {
      const auto &p = dirEntry.path();
      if (predicate(p)) {
        // Create path in target, if not existing.
        const auto relativeSrc = std::filesystem::relative(p, src);
        const auto targetParentPath = target / relativeSrc.parent_path();
        std::filesystem::create_directories(targetParentPath);

        // Copy to the targetParentPath which we just created.
        std::filesystem::copy(p, targetParentPath, std::filesystem::copy_options::overwrite_existing);
      }
    }
  } catch (std::exception &e) {
    std::cout << e.what();
  }
}
/// @brief saves two OR three column binary; e.g. f and amplitude, f and complex, f, amplitude and phase
/// @tparam T
/// @param filename
/// @param freqs
/// @param amplitudes_or_complex
/// @param extra - e.g. phase if needed, then amplitudes_or_complex should be REAL numbers
template <typename T>
inline void save_vector_to_bin_f(const std::filesystem::path &filename, const std::vector<double> &freqs, const std::vector<T> &amplitudes_or_complex, const std::optional<std::vector<double>> &e_g_phase = std::nullopt) {
  // first check that sizes match
  if (freqs.size() != amplitudes_or_complex.size()) {
    throw std::runtime_error("save_vector_to_bin_f: size mismatch between freqs and amplitudes_or_complex");
  }
  if (e_g_phase.has_value() && (freqs.size() != e_g_phase->size())) {
    throw std::runtime_error("save_vector_to_bin_f: size mismatch between freqs and extra (phase)");
  }
  // check if the top directory exists
  auto top_dir = filename.parent_path();
  if (!std::filesystem::exists(top_dir)) {
    throw std::runtime_error("save_vector_to_bin_f: directory " + top_dir.string() + " does not exist");
  }
  std::ofstream ofs(filename, std::ios::binary);
  if (!ofs.is_open()) {
    throw std::runtime_error("save_vector_to_bin_f: cannot open file " + filename.string() + " for writing");
  }
  // we write the data as binary f, amplitude_or_complex, [phase] one by one
  size_t nfreqs = freqs.size();
  for (size_t i = 0; i < nfreqs; ++i) {
    ofs.write(reinterpret_cast<const char *>(&freqs[i]), sizeof(double));
    ofs.write(reinterpret_cast<const char *>(&amplitudes_or_complex[i]), sizeof(T));
    if (e_g_phase.has_value()) {
      ofs.write(reinterpret_cast<const char *>(&((*e_g_phase)[i])), sizeof(double));
    }
  }
}

template <typename T>
inline void read_bin_f(const std::filesystem::path &filename, std::vector<double> &freqs, std::vector<T> &amplitudes_or_complex, std::optional<std::vector<double>> &e_g_phase) {
  // check if file exists
  if (!std::filesystem::exists(filename)) {
    throw std::runtime_error("read_bin_f: file " + filename.string() + " does not exist");
  }
  std::ifstream ifs(filename, std::ios::binary);
  if (!ifs.is_open()) {
    throw std::runtime_error("read_bin_f: cannot open file " + filename.string() + " for reading");
  }
  // get file size
  ifs.seekg(0, std::ios::end);
  size_t file_size = ifs.tellg();
  ifs.seekg(0, std::ios::beg);

  size_t record_size = sizeof(double) + sizeof(T);
  if (e_g_phase.has_value()) {
    record_size += sizeof(double);
  }
  if (file_size % record_size != 0) {
    throw std::runtime_error("read_bin_f: file size is not a multiple of record size in file " + filename.string());
  }
  size_t nrecords = file_size / record_size;
  freqs.resize(nrecords);
  amplitudes_or_complex.resize(nrecords);
  if (e_g_phase.has_value()) {
    e_g_phase->resize(nrecords);
  }
  for (size_t i = 0; i < nrecords; ++i) {
    ifs.read(reinterpret_cast<char *>(&freqs[i]), sizeof(double));
    ifs.read(reinterpret_cast<char *>(&amplitudes_or_complex[i]), sizeof(T));
    if (e_g_phase.has_value()) {
      ifs.read(reinterpret_cast<char *>(&((*e_g_phase)[i])), sizeof(double));
    }
  }
}

template <typename T>
inline void save_vector_to_ascii_f(const std::filesystem::path &filename, const std::vector<double> &freqs, const std::vector<T> &amplitudes_or_complex, const std::optional<std::vector<double>> &e_g_phase = std::nullopt) {
  if (freqs.size() != amplitudes_or_complex.size()) {
    throw std::runtime_error("save_vector_to_ascii_f: size mismatch between freqs and amplitudes_or_complex");
  }
  if (e_g_phase.has_value() && (freqs.size() != e_g_phase->size())) {
    throw std::runtime_error("save_vector_to_ascii_f: size mismatch between freqs and extra (phase)");
  }
  auto top_dir = filename.parent_path();
  if (!std::filesystem::exists(top_dir)) {
    throw std::runtime_error("save_vector_to_ascii_f: directory " + top_dir.string() + " does not exist");
  }
  std::ofstream ofs(filename);
  if (!ofs.is_open()) {
    throw std::runtime_error("save_vector_to_ascii_f: cannot open file " + filename.string() + " for writing");
  }
  ofs << std::scientific << std::setprecision(8);
  size_t nfreqs = freqs.size();
  for (size_t i = 0; i < nfreqs; ++i) {
    ofs << freqs[i] << " " << amplitudes_or_complex[i];
    if (e_g_phase.has_value()) {
      ofs << " " << (*e_g_phase)[i];
    }
    ofs << "\n";
  }
}

template <typename T>
inline void read_ascii_f_ptr(const std::filesystem::path &filename, std::shared_ptr<std::vector<double>> &freqs, std::shared_ptr<std::vector<T>> &amplitudes_or_complex) {
  if (!std::filesystem::exists(filename)) {
    throw std::runtime_error("read_ascii_f_ptr: file " + filename.string() + " does not exist");
  }
  std::size_t reserve_me = 131072;
  if (freqs)
    freqs->clear();
  else
    freqs = std::make_shared<std::vector<double>>();
  if (amplitudes_or_complex)
    amplitudes_or_complex->clear();
  else
    amplitudes_or_complex = std::make_shared<std::vector<T>>();
  if (!freqs || !amplitudes_or_complex) {
    throw std::runtime_error("read_ascii_f_ptr: failed to allocate memory");
  }
  freqs->reserve(reserve_me);
  amplitudes_or_complex->reserve(reserve_me);
  std::ifstream ifs(filename);
  if (!ifs.is_open()) {
    throw std::runtime_error("read_ascii_f_ptr: cannot open file " + filename.string() + " for reading");
  }
  // before reading, we need to know if we have a two column file (T is DOUBLE) or a three column file (T is std::complex<double>), we can do this by reading the first line and counting the number of columns
  std::string first_line;
  if (!std::getline(ifs, first_line)) {
    throw std::runtime_error("read_ascii_f_ptr: cannot read first line of file " + filename.string());
  }
  std::istringstream iss(first_line);
  std::vector<std::string> columns;
  std::string column;
  while (iss >> column) { // so this will read the first line only
    columns.push_back(column);
  }
  if (columns.size() == 2) {
    // we have a two column file, so T should be double
    if (!std::is_same<T, double>::value) {
      throw std::runtime_error("read_ascii_f_ptr: expected two columns in file " + filename.string() + " but T is not double");
    }
  } else if (columns.size() == 3) {
    // we have a three column file, so T should be std::complex<double>
    if (!std::is_same<T, std::complex<double>>::value) {
      throw std::runtime_error("read_ascii_f_ptr: expected three columns in file " + filename.string() + " but T is not std::complex<double>");
    }
  } else {
    throw std::runtime_error("read_ascii_f_ptr: expected two or three columns in file " + filename.string() + " but got " + std::to_string(columns.size()));
  }
  // now we can read the file line by line and parse the columns. move the first line back to the stream
  ifs.seekg(0);
  double freq;
  T amplitude;
  while (ifs >> freq >> amplitude) {
    freqs->push_back(freq);
    amplitudes_or_complex->push_back(amplitude);
  }
  ifs.close();
  freqs->shrink_to_fit();
  amplitudes_or_complex->shrink_to_fit();
}

/*
template <typename T>
inline void read_ascii_f(const std::filesystem::path &filename, std::vector<double> &freqs, std::vector<T> &amplitudes_or_complex, std::optional<std::vector<double>> &e_g_phase = std::nullopt) {
  if (!std::filesystem::exists(filename)) {
    throw std::runtime_error("read_ascii_f: file " + filename.string() + " does not exist");
  }
  std::ifstream ifs(filename);
  if (!ifs.is_open()) {
    throw std::runtime_error("read_ascii_f: cannot open file " + filename.string() + " for reading");
  }
  freqs.clear();
  amplitudes_or_complex.clear();
  if (e_g_phase.has_value()) {
    e_g_phase->clear();
  }
  size_t reseve_me = 131072;
  double freq;
  T amplitude;
  freqs.reserve(reseve_me);
  amplitudes_or_complex.reserve(reseve_me);
  if (e_g_phase.has_value()) {
    e_g_phase->reserve(reseve_me);
  }
  while (ifs >> freq >> amplitude) {
    freqs.push_back(freq);
    amplitudes_or_complex.push_back(amplitude);
    if (e_g_phase.has_value()) {
      double phase;
      if (!(ifs >> phase)) {
        throw std::runtime_error("read_ascii_f: expected phase value in file " + filename.string());
      }
      e_g_phase->push_back(phase);
    }
  }
}
*/

/*
// https://stackoverflow.com/questions/51431425/how-to-recursively-copy-files-and-directories
#include <filesystem>
#include <functional>
#include <iostream>
namespace fs = std::filesystem;

int main()
{
    const auto root = fs::current_path();
    const auto src = root / "src";
    const auto target = root / "target";

    // Copy only those files which contain "Sub" in their stem.
    const auto filter = [](const fs::path& p) -> bool
    {
        return p.stem().generic_string().find("Sub") != std::string::npos;
    };
    CopyRecursive(src, target, filter);
}
*/

} // namespace fdirs
#endif // FILES_DIRS_HPP
