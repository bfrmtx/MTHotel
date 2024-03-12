#ifndef CAL_SYNTHETIC_H
#define CAL_SYNTHETIC_H

#include <complex>
#include <numbers>
#include <vector>

#include "mt_base.h"

// for a synthetic frequency list f can be generated
// f =  ( double(i) * (f_sample/fwl) );

/*!
@file cal_synthetic.h
@brief  Synthetic transfer functions; not normalized; complex<double> ; mV as unit

*/

static std::vector<std::complex<double>> gen_trf_mfs06e(const std::vector<double> &freqs, const ChopperStatus &chopper) {
  std::complex<double> im(0.0, 1.0);
  size_t i = 0;
  std::vector<std::complex<double>> cal(freqs.size());
  if (chopper == ChopperStatus::on) {
    for (const auto &f : freqs) {
      std::complex<double> p1 = (f / 4.0) * im;
      std::complex<double> p2 = (f / 8192.0) * im;
      std::complex<double> p4 = (f / 28300.0) * im;
      // old  value 0.8 for V -> 800 mV
      // so at 0.1 Hz the old value 0.2 V / (nT *Hz) -> 20 mV / nT @ 0.1 Hz
      cal[i++] = 800.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4)));
    }
  } else if (chopper == ChopperStatus::off) {
    for (const auto &f : freqs) {
      std::complex<double> p1 = (f / 4.0) * im;
      std::complex<double> p2 = (f / 8192.0) * im;
      std::complex<double> p3 = (f / 0.720) * im;
      // old  value 25000, is corrected here
      std::complex<double> p4 = (f / 28300.0) * im;
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
  // if (chopper == ChopperStatus::on) {
  for (const auto &f : freqs) {
    std::complex<double> p1 = (f / 16.0) * im;
    std::complex<double> p2 = (f / 8192.0) * im;
    std::complex<double> p4 = (f / 28300.0) * im;
    // old  value 0.8 for V -> 800 mV
    // so at 0.1 Hz the old value 0.2 V / (nT *Hz) -> 20 mV / nT @ 0.1 Hz
    cal[i++] = 800.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4)));
  }
  // }
  return cal;
}

static std::vector<std::complex<double>> gen_trf_mfs07e(const std::vector<double> &freqs, const ChopperStatus &chopper) {
  std::complex<double> im(0.0, 1.0);
  size_t i = 0;
  std::vector<std::complex<double>> cal(freqs.size());
  if (chopper == ChopperStatus::on) {
    for (const auto &f : freqs) {
      std::complex<double> p1 = (f / 32.0) * im;
      std::complex<double> p2 = (f / 40000.) * im;
      std::complex<double> p4 = (f / 50000.0) * im;
      cal[i++] = 640.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4)));
    }
  } else if (chopper == ChopperStatus::off) {
    for (const auto &f : freqs) {
      std::complex<double> p1 = (f / 32.0) * im;
      std::complex<double> p2 = (f / 40000.) * im;
      std::complex<double> p3 = (f / 0.720) * im;
      std::complex<double> p4 = (f / 50000.0) * im;
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
static std::vector<std::complex<double>> gen_trf_adb_08e_hf(const std::vector<double> &freqs, const ADU &hp_filter, const double &gain_1 = 1.0) {

  // input resistivity doe not play a role
  std::complex<double> im(0.0, 1.0);
  std::vector<std::complex<double>> cal(freqs.size());
  size_t i = 0;
  for (const auto f : freqs) {
    std::complex<double> p1 = (f / 338.0E3) * im;
    std::complex<double> p2 = ((f * gain_1) / 100.0E6) * im;
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
static std::vector<std::complex<double>> gen_trf_adb_07e_hf(const std::vector<double> &freqs, const ADU &hp_filter, const double &gain_1 = 1.0, const double &gain_2 = 1.0) {

  std::complex<double> im(0.0, 1.0);
  std::vector<std::complex<double>> cal(freqs.size());
  size_t i = 0;
  for (const auto f : freqs) {

    std::complex<double> p1 = (f / 7.7E6) * im;
    // std::complex<double> p2 = (f / 7.7E6) * im;  p1 == p2 ?

    std::complex<double> trf = std::complex<double>(1.0, 0.0);

    // avoid rounding errors
    if (gain_1 > 1.1) {
      trf *= (1. / (1. + p1));
    }
    if (gain_2 > 1.1) {
      trf *= (1. / (1. + p1));
    }
    if (hp_filter == ADU::HF_HP_1Hz) {
      std::complex<double> p3 = (f / 1.0) * im;
      trf *= (p3 / (1. + p3));
    }

    cal[i++] = trf;
  }

  return cal;
}

/*!
 * \brief gen_trf_adb_08e_lf ; multiply later with gain_1 and gain_2 in case
 * \param freqs
 * \param radio_filter
 * \param lp_filter
 * \param input_div
 * \param resistance
 * \param gain_2
 * \return
 */
static std::vector<std::complex<double>> gen_trf_adb_08e_lf(const std::vector<double> &freqs, const ADU &radio_filter, const ADU &lp_filter, const ADU &input_div,
                                                            const double &resistance, const double &gain_1 = 1.0) {

  std::complex<double> im(0.0, 1.0);
  std::vector<std::complex<double>> cal(freqs.size());
  size_t i = 0;
  for (auto &f : freqs) {
    std::complex<double> p1 = (f / 318.0E3) * im;
    std::complex<double> p2 = ((f * gain_1) / 2.0E6) * im;

    std::complex<double> p4;

    if ((radio_filter == ADU::LF_RF_1) && (input_div == ADU::div_8))
      p4 = (f / 30.0E3) * im;
    else if ((radio_filter == ADU::LF_RF_2) && (input_div == ADU::div_8))
      p4 = (f / 10.5E3) * im;
    else if ((radio_filter == ADU::LF_RF_1) && (input_div == ADU::div_1))
      p4 = (f / (0.159 / ((resistance + 200.) * 470.0E-12))) * im;
    else if ((radio_filter == ADU::LF_RF_2) && (input_div == ADU::div_1))
      p4 = (f / (0.159 / ((resistance + 200.) * 7.27E-9))) * im;

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

// im ADC 1,2,4,8,16,32,64 only direct mode
static std::vector<std::complex<double>> gen_trf_adb_10e_lf(const std::vector<double> &freqs, const ADU &input_div,
                                                            const double &resistance, const double &gain_1 = 1.0) {

  std::complex<double> im(0.0, 1.0);
  std::vector<std::complex<double>> cal(freqs.size());
  size_t i = 0;
  for (auto &f : freqs) {
    std::complex<double> p1 = (f / 318.0E3) * im;
    std::complex<double> p2;

    if (input_div == ADU::div_8)
      p2 = (f / 7.8E3) * im;
    else if (input_div == ADU::div_1)
      p2 = (f / (0.159 / ((resistance + 200.) * 6.8E-9))) * im;

    std::complex<double> trf = (1. / (1. + p1)) * (1. / (1. + p2));

    cal[i++] = trf;
  }
  return cal;
}

static std::vector<std::complex<double>> gen_trf_adb_07e_lf(const std::vector<double> &freqs, const ADU &radio_filter, const ADU &lp_filter,
                                                            const ADU &input_div, const double &resistance) {

  std::complex<double> im(0.0, 1.0);

  std::vector<std::complex<double>> cal(freqs.size());
  size_t i = 0;
  for (auto &f : freqs) {
    std::complex<double> p1 = (f / 4.0E3) * im;
    std::complex<double> p2 = (f / 21.2E3) * im; //  Gain 2 here?
    std::complex<double> p4;

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

static std::vector<std::complex<double>> gen_trf_adb_07e_mf(const std::vector<double> &freqs, const double &sample_freq,
                                                            const ADU &radio_filter, const ADU &lp_filter, const ADU &hp_filter, const ADU &input_div,
                                                            const double &resistance) {

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

static std::complex<double> gen_trf_adb_07e_1Hz(const double &f, std::complex<double> &p1) {
  // 1 Hz HP from ADB board
  // im should be  std::complex<double> im(0.0,1.0);

  //    Michael´s ADU hat einen 1 Hz Hochpass - ganz normal.
  //    Der hat einen Einfluss bei Frequenzen kleiner 10 Hz. Besonders in dewr
  //    Phase, bei kleinen Frequenzen auch in der Amplitude. Es handelt sich
  //    hier um ein 1 poliges Filter. Die Formel ist P/(1-P) mit P=j*f/1Hz,
  //    also hier P=j*f.
  //    Die Absenkung der Amplitude ist bei 1Hz 0.707, die Phase dann +45°.
  //    Der Phaseneinfluss bei 10 Hz beträgt dann 90°-atan(10Hz/1Hz)= +5,71°.
  //    Natürlich dreht auch die Spule die Phase. Dies wird aber bei richtig
  //    angewendeter Kalibrierfunktion berücksichtigt. Er muss natürlich die
  //    für Chopper off verwenden.

  std::complex<double> im(0.0, 1.0);
  p1 = (f * im);
  return (p1 / (1. + p1));
}

#endif // CAL_SYNTHETIC_H
