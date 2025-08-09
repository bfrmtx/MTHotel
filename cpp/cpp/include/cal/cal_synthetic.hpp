#ifndef CAL_SYNTHETIC_HPP
#define CAL_SYNTHETIC_HPP

#include <cfloat>
#include <complex>
#include <numbers>
#include <vector>

#include "mt_base.hpp"

// for a synthetic frequency list f can be generated
// f =  ( double(i) * (f_sample/fwl) );

/*!
 * @file cal_synthetic.h
 * @brief  Synthetic transfer functions; not normalized; complex<double> ; mV as unit
 */

static std::vector<std::complex<double>> gen_trf_mfs06e(const std::vector<double> &freqs, const ChopperStatus &chopper) {
  std::complex<double> im(0.0, 1.0);
  size_t i = 0;
  std::vector<std::complex<double>> cal(freqs.size());
  if (chopper == ChopperStatus::on) {
    for (const auto &f : freqs) {
      std::complex<double> p1 = (f / 4.0) * im;
      std::complex<double> p2 = (f / 9645.0) * im;
      std::complex<double> p4 = (f / 23897.0) * im;
      // old  value 0.8 for V -> 800 mV
      // so at 0.1 Hz the old value 0.2 V / (nT *Hz) -> 20 mV / nT @ 0.1 Hz
      cal[i++] = 800.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4)));
    }
  } else if (chopper == ChopperStatus::off) {
    for (const auto &f : freqs) {
      std::complex<double> p1 = (f / 4.0) * im;
      std::complex<double> p2 = (f / 9645.0) * im;
      std::complex<double> p3 = (f / 0.720) * im;
      // old  value 25000, is corrected here
      std::complex<double> p4 = (f / 23897.0) * im;
      // old  value 0.8 for V -> 800 mV
      cal[i++] = 800.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (p3 / (1. + p3)) * (1. / (1. + p4)));
    }
  }
  return cal;
}

static std::vector<std::complex<double>> gen_trf_mfs12e(const std::vector<double> &freqs, const ChopperStatus &chopper) {
  std::complex<double> im(0.0, 1.0);
  size_t i = 0;
  std::vector<std::complex<double>> cal(freqs.size());
  for (const auto &f : freqs) {
    std::complex<double> p1 = (f / 16.0) * im;
    std::complex<double> p2 = (f / 9645.0) * im;
    std::complex<double> p4 = (f / 42287.0) * im;
    // old  value 0.8 for V -> 800 mV
    // so at 0.1 Hz the old value 0.2 V / (nT *Hz) -> 20 mV / nT @ 0.1 Hz
    cal[i++] = 800.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4)));
  }
  return cal;
}

static std::vector<std::complex<double>> gen_trf_mfs07e(const std::vector<double> &freqs, const ChopperStatus &chopper) {
  std::complex<double> im(0.0, 1.0);
  size_t i = 0;
  std::vector<std::complex<double>> cal(freqs.size());
  if (chopper == ChopperStatus::on) {
    for (const auto &f : freqs) {
      std::complex<double> p1 = (f / 32.0) * im;
      std::complex<double> p2 = (f / 45150.) * im;
      std::complex<double> p4 = (f / 49735.0) * im;
      cal[i++] = 640.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4)));
    }
  } else if (chopper == ChopperStatus::off) {
    for (const auto &f : freqs) {
      std::complex<double> p1 = (f / 32.0) * im;
      std::complex<double> p2 = (f / 45150.) * im;
      std::complex<double> p3 = (f / 0.720) * im;
      std::complex<double> p4 = (f / 49735.0) * im;
      cal[i++] = 640.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (p3 / (1. + p3)) * (1. / (1. + p4)));
    }
  }
  return cal;
}

static std::vector<std::complex<double>> gen_trf_mfs07(const std::vector<double> &freqs, const ChopperStatus &chopper) {
  std::complex<double> im(0.0, 1.0);
  size_t i = 0;
  std::vector<std::complex<double>> cal(freqs.size());
  if (chopper == ChopperStatus::on) {
    for (const auto &f : freqs) {
      std::complex<double> p1 = (f / 32.) * im;
      std::complex<double> p2 = (f / 45000.) * im;
      std::complex<double> p4 = (f / 28300.0) * im;
      cal[i++] = 640.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4)));
    }
  } else if (chopper == ChopperStatus::off) {
    for (const auto &f : freqs) {
      std::complex<double> p1 = (f / 32.0) * im;
      std::complex<double> p2 = (f / 45000.) * im;
      std::complex<double> p3 = (f / 0.720) * im;
      std::complex<double> p4 = (f / 28300.0) * im;
      cal[i++] = 640.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (p3 / (1. + p3)) * (1. / (1. + p4)));
    }
  }

  return cal;
}

// all fluxgates do not have a chopper
// Raklin Geomag-01
static std::vector<std::complex<double>> gen_trf_fgs02(const std::vector<double> &freqs) {
  std::vector<std::complex<double>> cal(freqs.size());
  for (size_t i = 0; i < freqs.size(); ++i) {
    cal[i] = std::complex<double>(7.5000E-01, 0.0);
  }
  return cal;
}

// bartington mag-03, low noise, 100 000 nT DEFAULT !
static std::vector<std::complex<double>> gen_trf_fgs03e(const std::vector<double> &freqs) {
  std::vector<std::complex<double>> cal(freqs.size());
  for (size_t i = 0; i < freqs.size(); ++i) {
    cal[i] = std::complex<double>(1.0000E-01, 0.0);
  }
  return cal;
}

// bartington mag-04, low noise, 70 000 nT, never sold yet

static std::vector<std::complex<double>> gen_trf_fgs05e(const std::vector<double> &freqs) {

  std::vector<std::complex<double>> cal(freqs.size());
  for (size_t i = 0; i < freqs.size(); ++i) {
    cal[i] = std::complex<double>(1.4300E-01, 0.0);
  }

  return cal;
}
// should reach E = 50mV / nT at 10 kHz
static std::vector<std::complex<double>> gen_trf_shft02e(const std::vector<double> &freqs) {
  std::complex<double> im(0.0, 1.0);
  size_t i = 0;
  std::vector<std::complex<double>> cal(freqs.size());
  for (const auto &f : freqs) {
    std::complex<double> p1 = (f / 3.0E5) * im;
    cal[i++] = 50.0 * (1. / (1. + p1));
  }
  return cal;
}

// ******************* B O A R D S **************************************************************************

/*!
 * \brief gen_trf_adb_08e_hf generate transfer function for the HF board to be folded with the sensor
 * \param f
 * \param high_pass
 * \param gain 1 used
 * \return cal; multiply with EXTRA gain in case you have not included that in LSB (bridget tests, external pre-amp)
 */
static std::vector<std::complex<double>> gen_trf_adb_08e_hf(const std::vector<double> &freqs, const ADU &hp_filter, const ADU &gain_1) {

  // input resistivity does not play a role
  std::complex<double> im(0.0, 1.0);
  std::vector<std::complex<double>> cal(freqs.size());
  size_t i = 0;
  double gain_one = double(gain_1);
  for (const auto f : freqs) {
    std::complex<double> p1 = (f / 338.0E3) * im;
    std::complex<double> p2 = ((f * gain_one) / 100.0E6) * im;
    std::complex<double> p3 = (f / 1.59E6) * im;

    if (hp_filter == ADU::HF_HP_500Hz) {
      std::complex<double> p4 = (f / 482.0) * im;
      cal[i++] = (1. / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p3)) * (p4 / (1. + p4));
    } else if (hp_filter == ADU::HF_HP_1Hz) { // 1 Hz has been dropped for 08e but some airborne systems may have it
      std::complex<double> p4 = (f / 1.0) * im;
      cal[i++] = (1. / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p3)) * (p4 / (1. + p4));
    } else {
      cal[i++] = (1. / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p3));
    }
  }

  return cal;
}

/*!
 * \brief gen_trf_adb_07e_hf generate transfer function for the HF board to be folded with the sensor
 * \param f
 * \param high_pass
 * \param gain_1
 * \param gain_2

 */
static std::vector<std::complex<double>> gen_trf_adb_07e_hf(const std::vector<double> &freqs, const ADU &hp_filter, const ADU &gain_1, const ADU &gain_2) {

  std::complex<double> im(0.0, 1.0);
  std::vector<std::complex<double>> cal(freqs.size());
  size_t i = 0;
  for (const auto f : freqs) {

    std::complex<double> p1 = (f / 7.23E6) * im;

    std::complex<double> trf = std::complex<double>(1.0, 0.0);

    // avoid rounding errors
    if (gain_1 != ADU::gain_1_1) {
      trf *= (1. / (1. + p1));
    }
    if (gain_2 != ADU::gain_2_1) {
      trf *= (1. / (1. + p1));
    }
    // we do NOT have 500 Hz filter here as we have in 08e
    if (hp_filter == ADU::HF_HP_1Hz) {
      std::complex<double> p3 = (f / 1.0) * im;
      trf *= (p3 / (1. + p3));
    }

    cal[i++] = trf;
  }

  return cal;
}

/*!
 * \brief gen_trf_adb_08e_lf ;
 * \param freqs
 * \param radio_filter
 * \param lp_filter
 * \param input_div
 * \param resistance
 * \param gain_2
 * \return
 */
static std::vector<std::complex<double>> gen_trf_adb_08e_lf(const std::vector<double> &freqs, const ADU &radio_filter, const ADU &lp_filter, const ADU &input_div, const double &resistance, const ADU &direct_mode, const ADU &gain_1) {

  std::complex<double> im(0.0, 1.0);
  std::vector<std::complex<double>> cal(freqs.size());
  size_t i = 0;
  for (auto &f : freqs) {
    std::complex<double> p1 = (f / 318.0E3) * im;
    double gain_one = double(gain_1);

    std::complex<double> p2 = ((f * gain_one) / 2.0E6) * im;

    std::complex<double> p4;

    if ((radio_filter == ADU::LF_RF_1) && (input_div == ADU::div_8)) // e.g. coils
      p4 = (f / 30.0E3) * im;
    else if ((radio_filter == ADU::LF_RF_2) && (input_div == ADU::div_8)) // default for coils
      p4 = (f / 10.5E3) * im;
    else if ((radio_filter == ADU::LF_RF_1) && (input_div == ADU::div_1)) // medium contact resistance < 1500 Ohm
      p4 = 2.0 * M_PI * f * (resistance + 200.) * 470.0E-12 * im;
    else if ((radio_filter == ADU::LF_RF_2) && (input_div == ADU::div_1)) // high contact resistance > 1500 Ohm
      p4 = 2.0 * M_PI * f * (resistance + 200.) * 7.27E-9 * im;

    // the fixed "gain 3" of 2  is invisible in the time series an calibrated into the LSB

    std::complex<double> trf = (1. / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4));

    if (lp_filter == ADU::LF_LP_4Hz) {
      std::complex<double> p3 = (f / 4.) * im;
      trf *= (1. / (1. + std::numbers::sqrt2 * p3 + (p3 * p3)));
    }
    cal[i++] = trf;
  }
  return cal;
}

// numerical approximation in order to find  a double value in a vector
inline std::pair<double, std::complex<double>> find_trf_adb_08e_lf(const double &find_me, const bool ampl_true_phase_false, const std::vector<double> &freqs, const ADU &radio_filter, const ADU &lp_filter, const ADU &input_div, const double &resistance, const ADU &direct_mode, const ADU &gain_1) {
  // first we need to generate the transfer function and check if find_me is in the range
  auto cal = gen_trf_adb_08e_lf(freqs, radio_filter, lp_filter, input_div, resistance, direct_mode, gain_1);
  std::vector<double> ampl_or_phase(cal.size()); // the data is monotonic, so we can use a binary search, but can be ascending or descending

  if (ampl_true_phase_false) {
    for (size_t i = 0; i < cal.size(); ++i) {
      ampl_or_phase[i] = std::abs(cal[i]);
    }
  } else {
    for (size_t i = 0; i < cal.size(); ++i) {
      ampl_or_phase[i] = std::arg(cal[i]);
    }
  }
  // if vector contains find_me we return the value
  if (std::find(ampl_or_phase.begin(), ampl_or_phase.end(), find_me) != ampl_or_phase.end()) {
    // now we need to find the index of the value
    auto it = std::find(ampl_or_phase.begin(), ampl_or_phase.end(), find_me);
    size_t index = std::distance(ampl_or_phase.begin(), it);
    return std::make_pair(freqs[index], cal[index]);
  }

  // now we need to check if the data is ascending or descending, then insert protocol
  double z_first_begin = ampl_or_phase.front(); // fist element of z
  double z_fist_end = ampl_or_phase.back();     // last element of z
  // now we need to check if the data is ascending or descending, then insert protocol
  bool ascending = false;
  if (z_first_begin < z_fist_end) {
    ascending = true;
  } else {
    ascending = false;
  }

  // now we need the index of a lesser and greater value
  size_t i = 0;
  size_t j = 0;
  for (i = 0; i < ampl_or_phase.size(); ++i) {
    if (ampl_or_phase[i] > find_me) {
      break;
    }
  }
  for (j = 0; j < ampl_or_phase.size(); ++j) {
    if (ampl_or_phase[j] < find_me) {
      break;
    }
  }
  // return if the value is not in the range
  if (i == 0 && j == 0) {
    return std::make_pair(0.0, std::complex<double>(DBL_MAX, DBL_MAX));
  }
  // now if i == 0 and j < ampl_or_phase.size(), and j > i, we check if the previous value is greater than find_me
  if (i == 0 && j < ampl_or_phase.size()) {
    if (ampl_or_phase[j - 1] < find_me) {
      return std::make_pair(0.0, std::complex<double>(DBL_MAX, DBL_MAX));
    } else {
      i = j - 1;
    }
  }

  // now we generate a new frequency vector between i and j with 100 points
  size_t new_size = 100;
  std::vector<double> new_freqs(new_size);
  // linear interpolation of the frequency vector between i and j
  for (size_t k = 0; k < new_freqs.size(); ++k) {
    new_freqs[k] = freqs[i] + (freqs[j] - freqs[i]) * double(k) / double(new_size);
  }
  // now we generate a new transfer function for the new frequency vector
  cal = gen_trf_adb_08e_lf(new_freqs, radio_filter, lp_filter, input_div, resistance, direct_mode, gain_1);
  ampl_or_phase.resize(new_size);
  if (ampl_true_phase_false) {
    for (size_t i = 0; i < cal.size(); ++i) {
      ampl_or_phase[i] = std::abs(cal[i]);
    }
  } else {
    for (size_t i = 0; i < cal.size(); ++i) {
      ampl_or_phase[i] = std::arg(cal[i]);
    }
  }
  // now find the closest value in the ampl_or_phase vector
  double min_diff = DBL_MAX;
  size_t min_index = 0;
  for (size_t k = 0; k < ampl_or_phase.size(); ++k) {
    double diff = std::abs(ampl_or_phase[k] - find_me);
    if (diff < min_diff) {
      min_diff = diff;
      min_index = k;
    }
  }
  // if the difference is too big we return DBL_MAX, else if less than 1% we return the value we have passed as argument
  if (min_diff < 0.01) {
    return std::make_pair(new_freqs[min_index], find_me);
  }
  return std::make_pair(0.0, std::complex<double>(DBL_MAX, DBL_MAX));
}

// im ADC 1,2,4,8,16,32,64 only direct mode
static std::vector<std::complex<double>> gen_trf_adb_10e_lf(const std::vector<double> &freqs, const ADU &input_div, const double &resistance) {
  std::complex<double> im(0.0, 1.0);
  std::vector<std::complex<double>> cal(freqs.size());
  size_t i = 0;
  for (auto &f : freqs) {
    std::complex<double> p1 = (f / 318.0E3) * im;
    std::complex<double> p4;

    if (input_div == ADU::div_8)
      p4 = (f / 7.8E3) * im;
    else if (input_div == ADU::div_1)
      p4 = 2.0 * M_PI * f * (resistance + 200.) * 6.8E-9 * im;

    std::complex<double> trf = (1. / (1. + p1)) * (1. / (1. + p4));

    cal[i++] = trf;
  }
  return cal;
}

static std::vector<std::complex<double>> gen_trf_adb_07e_lf(const std::vector<double> &freqs, const ADU &radio_filter, const ADU &lp_filter, const ADU &input_div, const double &resistance, const ADU &gain_1) {
  std::complex<double> im(0.0, 1.0);

  std::vector<std::complex<double>> cal(freqs.size());
  size_t i = 0;
  for (auto &f : freqs) {
    std::complex<double> p1 = (f / 4.0E3) * im;
    std::complex<double> p2 = (f / 21.2E3) * im; //  Gain 2 here?
    std::complex<double> p4;

    if ((radio_filter == ADU::LF_RF_1) && (input_div == ADU::div_1))
      p4 = 2.0 * M_PI * f * (resistance + 200.) * 2.2E-11 * im;
    else if ((radio_filter == ADU::LF_RF_2) && (input_div == ADU::div_1))
      p4 = 2.0 * M_PI * f * (resistance + 200.) * 1.22E-10 * im;
    else if ((radio_filter == ADU::LF_RF_3) && (input_div == ADU::div_1))
      p4 = 2.0 * M_PI * f * (resistance + 200.) * 6.822E-9 * im;
    else if ((radio_filter == ADU::LF_RF_4) && (input_div == ADU::div_1))
      p4 = 2.0 * M_PI * f * (resistance + 200.) * 6.922E-9 * im;

    // missing div_8

    //!< @todo RADIO FILTER missing
    // Radio Filter not respected yet  ADU08 setting
    //    if      ((radio_filter == ADU::LF_RF_1) && (input_div == ADU::div_8)) p4 = (f / 30.0E3) * im;
    //    else if ((radio_filter == ADU::LF_RF_2) && (input_div == ADU::div_8)) p4 = (f / 10.5E3) * im;
    //    else if ((radio_filter == ADU::LF_RF_1) && (input_div == ADU::div_1)) p4 = (f / ( 0.159 / ((resistance + 200.) * 470.0E-12) ) ) * im;
    //    else if ((radio_filter == ADU::LF_RF_2) && (input_div == ADU::div_1)) p4 = (f / ( 0.159 / ((resistance + 200.) * 7.27E-9) ) ) * im;

    // rf_3 rf_4

    std::complex<double> trf = (1. / (1. + p1)) * (1. / (1. + p2));

    if (lp_filter == ADU::LF_LP_4Hz) {
      std::complex<double> p3 = (f / 4.) * im;
      trf *= (1. / (1. + std::numbers::sqrt2 * p3 + (p3 * p3)));
    }
    cal[i++] = trf;
  }

  return cal;
}

static std::vector<std::complex<double>> gen_trf_adb_07e_mf(const std::vector<double> &freqs, const double &sample_freq, const ADU &radio_filter, const ADU &lp_filter, const ADU &hp_filter, const ADU &input_div, const double &resistance) {
  std::complex<double> im(0.0, 1.0);
  std::complex<double> p1;

  std::vector<std::complex<double>> cal(freqs.size());
  size_t i = 0;
  for (auto &f : freqs) {

    if (sample_freq > (65536 - 1))
      p1 = (f / 48.1E3) * im;
    else if (sample_freq > (16384 - 1))
      p1 = (f / 15.9E3) * im;
    else if (sample_freq > (4096 - 1))
      p1 = (f / 3.7E3) * im;
    else if (sample_freq > (128 - 1))
      p1 = (f / 159.) * im;

    std::complex<double> trf = (1. / (1. + p1));

    //!< @todo RADIO FILTER missing

    // Radio Filter not respected yet  ADU08 setting
    //    if      ((radio_filter == ADU::LF_RF_1) && (input_div == ADU::div_8)) p4 = (f / 30.0E3) * im;
    //    else if ((radio_filter == ADU::LF_RF_2) && (input_div == ADU::div_8)) p4 = (f / 10.5E3) * im;
    //    else if ((radio_filter == ADU::LF_RF_1) && (input_div == ADU::div_1)) p4 = (f / ( 0.159 / ((resistance + 200.) * 470.0E-12) ) ) * im;
    //    else if ((radio_filter == ADU::LF_RF_2) && (input_div == ADU::div_1)) p4 = (f / ( 0.159 / ((resistance + 200.) * 7.27E-9) ) ) * im;

    // rf_3 rf_4
    if (lp_filter == ADU::LF_LP_4Hz) {
      std::complex<double> p3 = (f / 4.) * im;
      trf *= (1. / (1. + std::numbers::sqrt2 * p3 + (p3 * p3)));
    }
    // **************************************************** 482 or 500 **************************************************
    if (hp_filter == ADU::HF_HP_500Hz) {
      std::complex<double> p4 = (f / 482.0) * im;
      trf *= (p4 / (1. + p4));
    }

    cal[i++] = trf;
  }

  return cal;
}

// a wrapper for all systems, and all boards

static std::vector<std::complex<double>> gen_system_cal(const std::vector<double> &freqs, const std::string &system, const double &resistance, const ADU &board, ADU &direct_mode, ADU &radio_filter, ADU &lp_filter, ADU &hp_filter, ADU &input_div, ADU &gain_1, ADU &gain_2) {
  std::vector<std::complex<double>> cal(freqs.size());
  if (system == "ADU-08e") {
    if (board == ADU::LF) {
      cal = gen_trf_adb_08e_lf(freqs, radio_filter, lp_filter, input_div, resistance, direct_mode, gain_1);
    } else if (board == ADU::HF) {
      cal = gen_trf_adb_08e_hf(freqs, hp_filter, gain_1);
    }
  } else if (system == "ADU-07e") {
    if (board == ADU::LF) {
      cal = gen_trf_adb_07e_lf(freqs, radio_filter, lp_filter, input_div, resistance, gain_1);
    } else if (board == ADU::HF) {
      cal = gen_trf_adb_07e_hf(freqs, hp_filter, gain_1, gain_2);
    }
  } else if (system == "ADU-10e") {
    if (board == ADU::LF) {
      cal = gen_trf_adb_10e_lf(freqs, input_div, resistance);
    }
  }
  return cal;
}

#endif // CAL_SYNTHETIC_HPP
