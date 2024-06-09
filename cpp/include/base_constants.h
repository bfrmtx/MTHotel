#ifndef BASE_CONSTANTS_H
#define BASE_CONSTANTS_H
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

const double mue0 = 4.0 * M_PI * 1.0E-7;
const double min_fft_wl = 64;
const double treat_as_null = 1E-32;
const double treat_as_out_of_range = 1E+32; //!< EDI file
// vector of strings for channel types Ex, Ey, Hx, Hy, Hz, REx, REy, RHx, RHy, RHz, emap: EEx, EEy; Ez may be needed for sub-marine
// J TX ampere, ... jaw (away from flight direction forward), pitch (up & down nose), roll (over the wings "left/right" up & down))
static const std::vector<std::string> available_channel_types = {"Ex", "Ey", "Hx", "Hy", "Hz", "REx", "REy", "RHx", "RHy", "RHz", "EEx", "EEy", "Ez", "REz", "EEz", "Jx", "Jy", "Jz", "x", "y", "z", "T", "t"};
static const std::vector<std::string> available_ac_spectra_types = {"ExEx", "ExEy", "HxHx", "HxHy", "HxHz", "HyHy", "HyHz", "HzHz"};
static bool is_E(const std::string &channel_type) {
  // check if channel type is E, contains Ex, Ey or Ez as substring
  return channel_type.find("Ex") != std::string::npos || channel_type.find("Ey") != std::string::npos || channel_type.find("Ez") != std::string::npos;
}
static bool is_H(const std::string &channel_type) {
  // check if channel type is H, contains Hx, Hy or Hz as substring
  return channel_type.find("Hx") != std::string::npos || channel_type.find("Hy") != std::string::npos || channel_type.find("Hz") != std::string::npos;
}
#endif // BASE_CONSTANTS_H
