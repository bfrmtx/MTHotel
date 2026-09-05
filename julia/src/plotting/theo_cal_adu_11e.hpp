#include <complex>
#include <vector>

/// returns the calibration amplitude for the given frequencies, sampling
/// rate,cutoff frequency, and order
/// spectrum shall be divided by these calibration factors to correct for the
/// lowpass response
class adu11e_board_cal {
public:
  explicit adu11e_board_cal(const double sampling_rate) {
    this->sampling_rate = sampling_rate;
    this->fc = 0.0;
    this->n = 0.0;
    if (this->sampling_rate <= 0) {
      throw std::invalid_argument("sampling_rate must be positive");
    }
    if (this->sampling_rate == 8192) {
      this->fc = 3094.0;
      this->n = 1.05;
    } else if (this->sampling_rate == 16384) {
      this->fc = 6161.0;
      this->n = 1.02;
    } else if (this->sampling_rate == 32768) {
      this->fc = 12129.0;
      this->n = 1.06;
    } else if (this->sampling_rate == 65536) {
      this->fc = 23237.0;
      this->n = 1.03;
    }
    if (this->sampling_rate == 131072) {
      this->fc = 40124.0;
      this->n = 1.01;
    } else {
      throw std::invalid_argument("unsupported sampling_rate");
    }
  }

  ~adu11e_board_cal() = default;

  std::vector<double> cal_ampl(const std::vector<double> &freqs) {
    std::vector<double> ampl;
    ampl.reserve(freqs.size());
    for (const auto &f : freqs) {
      // matches the super-Gaussian model used to fit fc/n: exp(-(f/fc)^(2n))
      ampl.push_back(std::exp(-std::pow(f / this->fc, 2.0 * this->n)));
    }
    return ampl;
  }

private:
  double sampling_rate;
  double fc;
  double n;
};