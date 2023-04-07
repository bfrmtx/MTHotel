#ifndef SYNTHETIC_H
#define SYNTHETIC_H

#include <complex>
#include "../../include/mt_base.h"

// for a sythetic frequency list f can be generated
//f =  ( double(i) * (f_sample/fwl) );

std::complex<double> gen_trf_mfs06e_chopper_on_template(const double &f) {
    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1 = (f / 4.) * im;
    std::complex<double> p2 = (f / 8192.) * im;
    std::complex<double> p4 = (f / 28300.0) * im;
    return  0.8 * (  (p1 / ( 1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4))  );
}

std::complex<double> gen_trf_mfs06e_chopper_off_template(const double &f) {
    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1 = (f / 4.) * im;
    std::complex<double> p2 = (f / 8192.) * im;
    std::complex<double> p3 = (f / 0.72) * im;
    std::complex<double> p4 = (f / 25000.0) * im;
    return 0.8 * (  (p1 / ( 1. + p1)) * (1. / (1. + p2)) * (p3 / (1. + p3)) * (1. / (1. + p4)) );
}

std::complex<double> gen_trf_mfs07e_chopper_on_template(const double &f) {
    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1 = (f / 32.) * im;
    std::complex<double> p2 = (f / 40000.) * im;
    std::complex<double> p4 = (f / 50000.0) * im;
    return 0.64 * (  (p1 / ( 1. + p1)) * (1. / (1. + p2))  * (1. / (1. + p4)) );
}

std::complex<double> gen_trf_mfs07e_chopper_off_template(const double &f) {
    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1 = (f / 32.) * im;
    std::complex<double> p2 = (f / 40000.) * im;
    std::complex<double> p3 = (f / 0.72) * im;
    std::complex<double> p4 = (f / 50000.0) * im;
    return 0.64 * (  (p1 / ( 1. + p1)) * (1. / (1. + p2)) * (p3 / (1. + p3))  * (1. / (1. + p4)) );
}

std::complex<double> gen_trf_mfs07_chopper_on_template(const double &f) {
    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1 = (f / 32.) * im;
    std::complex<double> p2 = (f / 45000.) * im;
    std::complex<double> p4 = (f / 28300.0) * im;
    return 0.64 * (  (p1 / ( 1. + p1)) * (1. / (1. + p2))  * (1. / (1. + p4)) );
}

std::complex<double> gen_trf_mfs07_chopper_off_template(const double &f) {
    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1 = (f / 32.0) * im;
    std::complex<double> p2 = (f / 45000.) * im;
    std::complex<double> p3 = (f / 0.720) * im;
    std::complex<double> p4 = (f / 28300.0) * im;
    return 0.64 * (  (p1 / ( 1. + p1)) * (1. / (1. + p2)) * (p3 / (1. + p3))  * (1. / (1. + p4)) );
}

// all fluxgates do not have a chopper
// Raklin Geomag-01
std::complex<double> gen_trf_fgs02_template() {

    return std::complex<double>(7.5000E-04, 0.0);
}

// bartington mag-03, low noise, 100 000 nT DEFAULT !

std::complex<double> gen_trf_fgs03e_template() {

    return std::complex<double>(1.0000E-04, 0.0);
}

// bartington mag-04, low noise, 70 000 nT, never sold yet

std::complex<double> gen_trf_fgs04e_template(std::complex<double> &trf) {

    return std::complex<double>(1.4300E-04, 0.0);
}



/*!
 * \brief gen_trf_adb_08e_hf_xcalib calibration for using with external measurments where gains can appear
 * \param f
 * \param high_pass
 * \param gain_1
 */
std::complex<double> gen_trf_adb_08e_hf_xcalib(const double &f, const ADU &hp_filter, const double &gain_1) {
    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1 = (f / 338.0E3) * im;
    std::complex<double> p2 = ((f * gain_1) / 100.0E6) * im;
    std::complex<double> p3 = (f / 1.59E6) *im;

    if (hp_filter == ADU::adu08e_hp500hz_on) {
        std::complex<double> p4 = (f / 482.0) * im;
        return gain_1 *  (1. / (1. + p1)) *  (1. / (1. + p2)) * (1. / (1. + p3)) *  (p4 / ( 1. + p4));
    }
    else if (hp_filter == ADU::adu08e_hp1hz_on) {
        std::complex<double> p4 = (f / 1.0) * im;
        return gain_1 *  (1. / (1. + p1)) *  (1. / (1. + p2)) * (1. / (1. + p3)) *  (p4 / ( 1. + p4)) ;
    }
    else {
        return gain_1 *  (1. / (1. + p1)) *  (1. / (1. + p2)) * (1. / (1. + p3));
    }
}

/*!
 * \brief gen_trf_adb_08e_hf generate transfer function for the HF board to be folded with the sensor
 * \param f
 * \param high_pass
 */
 std::complex<double> gen_trf_adb_08e_hf(const double &f, const ADU &hp_filter) {
    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1 = (f / 338.0E3) * im;
    std::complex<double> p2 = (f / 100.0E6) * im;
    std::complex<double> p3 = (f / 1.59E6) *im;

    if (hp_filter == ADU::adu08e_hp500hz_on) {
        std::complex<double> p4 = (f / 482.0) * im;
        return (1. / (1. + p1)) *  (1. / (1. + p2)) * (1. / (1. + p3)) *  (p4 / ( 1. + p4)) ;
    }
    else if (hp_filter == ADU::adu08e_hp1hz_on) {
        std::complex<double> p4 = (f / 1.0) * im;
        return (1. / (1. + p1)) *  (1. / (1. + p2)) * (1. / (1. + p3)) *  (p4 / ( 1. + p4)) ;
    }
    else {
        return (1. / (1. + p1)) *  (1. / (1. + p2)) * (1. / (1. + p3));
    }
}

/*!
 * \brief gen_trf_adb_07e_hf generate transfer function for the HF board to be folded with the sensor
 * \param f
 * \param high_pass
 * \param gain_1
 * \param gain_2

 */
 std::complex<double> gen_trf_adb_07e_hf(const double &f, const ADU &hp_filter, const double &gain_1, const double &gain_2) {
    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1 = (f / 7.7E6) * im;
    std::complex<double> p2 = (f / 7.7E6) * im;

    std::complex<double> f_1 = std::complex<double>(1.0, 0.0);
    std::complex<double> f_2 = std::complex<double>(1.0, 0.0);
    std::complex<double> f_3 = std::complex<double>(1.0, 0.0);

    if (gain_1 > 1.1) {
        f_1 = (1. / (1. + p1));
    }
    if (gain_2 > 1.1) {
        f_1 = (1. / (1. + p2));
    }

    if (hp_filter == ADU::adu07e_hp1hz_on) {
        std::complex<double> p3 = (f / 1.0) * im;
        f_3 = (p3 / ( 1. + p3)) ;
    }

    return f_1 * f_2 * f_3;
}



/*!
 * \brief gen_trf_adb_08e_lf_xcalib  calibration for using with external measurments where gains can appear
 * \param f
 * \param gain_1
 * \param gain_2
 * \param resistance
 * \param input_div
 * \param radio_filter
 * \param lp_filter
 */
 std::complex<double> gen_trf_adb_08e_lf_xcalib(const double &f,
                                      const double &gain_1, const double &gain_2, const double &resistance,
                                      const ADU &input_div, const ADU &radio_filter, const ADU &lp_filter) {

    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1 = (f / 318.0E3) * im;
    std::complex<double> p2 = ((f * gain_2) / 2.0E6) * im;

    std::complex<double> p4;

    if      ((radio_filter == ADU::adu08e_rf_1) && (input_div == ADU::div_8)) p4 = (f / 30.0E3) * im;
    else if ((radio_filter == ADU::adu08e_rf_2) && (input_div == ADU::div_8)) p4 = (f / 10.5E3) * im;
    else if ((radio_filter == ADU::adu08e_rf_1) && (input_div == ADU::div_1)) p4 = (f / ( 0.159 / ((resistance + 200.) * 470.0E-12) ) ) * im;
    else if ((radio_filter == ADU::adu08e_rf_2) && (input_div == ADU::div_1)) p4 = (f / ( 0.159 / ((resistance + 200.) * 7.27E-9) ) ) * im;



    // the fixed gain of 2  is invisible in the time series an calibrated in the LSB
    std::complex<double> trf = gain_1 * gain_2 * (1. / (1. + p1)) *  (1. / (1. + p2)) *  (1. / ( 1. + p4));

    //    }
    if (lp_filter == ADU::adu08e_lp4hz_on) {
        std::complex<double> p3 = (f / 4.) * im;
        trf *= (1. / (1. + 1.41421356237 * p3 + (p3 * p3) ));

    }
    return trf;
}

/*!
 * \brief gen_trf_adb_08e_lf generate transfer function for the LF board to be folded with the sensor
 * \param f
 * \param resistance
 * \param input_div
 * \param radio_filter
 * \param lp_filter
 */
 std::complex<double> gen_trf_adb_08e_lf(const double &f, const double &resistance,
                               const ADU &input_div, const ADU &radio_filter, const ADU &lp_filter) {
    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1 = (f / 318.0E3) * im;
    std::complex<double> p2 = (f / 2.0E6) * im;
    std::complex<double> p4;


    if      ((radio_filter == ADU::adu08e_rf_1) && (input_div == ADU::div_8)) p4 = (f / 30.0E3) * im;
    else if ((radio_filter == ADU::adu08e_rf_2) && (input_div == ADU::div_8)) p4 = (f / 10.5E3) * im;
    else if ((radio_filter == ADU::adu08e_rf_1) && (input_div == ADU::div_1)) p4 = (f / ( 0.159 / ((resistance + 200.) * 470.0E-12) ) ) * im;
    else if ((radio_filter == ADU::adu08e_rf_2) && (input_div == ADU::div_1)) p4 = (f / ( 0.159 / ((resistance + 200.) * 7.27E-9) ) ) * im;

    // the fixed gain of 2  is invisible in the time series an calibrated in the LSB
    std::complex<double> trf = (1. / (1. + p1)) *  (1. / (1. + p2)) *  (1. / ( 1. + p4));


    if (lp_filter == ADU::adu08e_lp4hz_on) {
        std::complex<double> p3 = (f / 4.) * im;
        trf *= (1. / (1. + 1.41421356237 * p3 + (p3 * p3) ));

    }

    return trf;
}


/*!
 * \brief gen_trf_adb_07e_lf generate transfer function for the LF board to be folded with the sensor
 * \param f
 * \param p1
 * \param p2
 * \param p3
 * \param p4
 * \param resistance
 * \param input_div
 * \param radio_filter
 * \param lp_filter
 * \param im
 * \param trf
 */
 /*
inline void gen_trf_adb_07e_lf(const double &f, std::complex<double> &p1, std::complex<double> &p2, std::complex<double> &p3, std::complex<double> &p4,
                               const double &resistance,
                               const ADU &input_div, const ADU &radio_filter, const ADU &lp_filter,
                               const std::complex<double> &im, std::complex<double> &trf) {
    std::complex<double> im (0.0, 1.0);
    p1 = (f / 4.0E3) * im;
    p2 = (f / 21.2E3) * im;

    trf = (1. / (1. + p2)) *  (1. / (1. + p2));

    //!< @todo RADIO FILTER missing
    // Radio Filter not respected yet
    //    if      ((radio_filter == ADU::adu07e_rf_1) && (input_div == ADU::div_8)) p4 = (f / 30.0E3) * im;
    //    else if ((radio_filter == ADU::adu07e_rf_2) && (input_div == ADU::div_8)) p4 = (f / 10.5E3) * im;
    //    else if ((radio_filter == ADU::adu07e_rf_1) && (input_div == ADU::div_1)) p4 = (f / ( 0.159 / ((resistance + 200.) * 470.0E-12) ) ) * im;
    //    else if ((radio_filter == ADU::adu07e_rf_2) && (input_div == ADU::div_1)) p4 = (f / ( 0.159 / ((resistance + 200.) * 7.27E-9) ) ) * im;

    if (lp_filter == ADU::adu07e_lp4hz_on) {
        p3 = (f / 4.) * im;
        trf *= (1. / (1. + 1.41421356237 * p3 + (p3 * p3) ));

    }
}
*/

/*

std::complex<double> gen_trf_adb_07e_mf(const double &f, const double &sample_freq, , std::complex<double> &p2, std::complex<double> &p3, std::complex<double> &p4,
                               const double &resistance, const ADU &input_div,
                               const ADU &radio_filter, const ADU &lp_filter, const ADU &hp_filter,
                                ) {
    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1;
    if (sample_freq > (65536 - 1) )      p1 = (f / 48.1E3) * im;
    else if (sample_freq > (16384 - 1) ) p1 = (f / 15.9E3) * im;
    else if (sample_freq > (4096 - 1) )  p1 = (f / 3.7E3)  * im;
    else if (sample_freq > (128 - 1) )   p1 = (f / 159.)   * im;



    trf = (1. / (1. + p1));

    //!< @todo RADIO FILTER missing
    // Radio Filter not respected yet
    //    if      ((radio_filter == ADU::adu07e_rf_1) && (input_div == ADU::div_8)) p4 = (f / 30.0E3) * im;
    //    else if ((radio_filter == ADU::adu07e_rf_2) && (input_div == ADU::div_8)) p4 = (f / 10.5E3) * im;
    //    else if ((radio_filter == ADU::adu07e_rf_1) && (input_div == ADU::div_1)) p4 = (f / ( 0.159 / ((resistance + 200.) * 470.0E-12) ) ) * im;
    //    else if ((radio_filter == ADU::adu07e_rf_2) && (input_div == ADU::div_1)) p4 = (f / ( 0.159 / ((resistance + 200.) * 7.27E-9) ) ) * im;

    if (lp_filter == ADU::adu07e_lp4hz_on) {
        p3 = (f / 4.) * im;
        trf *= (1. / (1. + 1.41421356237 * p3 + (p3 * p3) ));

    }
    if (hp_filter == ADU::adu07e_hp500hz_on) {
        p4 = (f / 482.0) * im;
        trf *= (p4 / ( 1. + p4)) ;
    }


}

*/


// should reach E = 50mV / nT at 10 kHz
std::complex<double> gen_trf_shft02e_template(const double &f) {
    std::complex<double> im (0.0, 1.0);
    std::complex<double> p1 = (f / 3.0E5) * im;
    return 1. * 0.05 * (1./ (1. + p1));
}

std::complex<double> gen_trf_adb_07e_1Hz(const double &f, std::complex<double> &p1) {
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

    std::complex<double> im (0.0, 1.0);
    p1 = (f * im);
    return (p1 / ( 1. + p1));
}




#endif // SYNTHETIC_H
