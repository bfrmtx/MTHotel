#ifndef CAL_ENTRY
#define CAL_ENTRY

#include <complex>
#include <memory>
#include <string>
#include <vector>

class cal_entry {

public:
  cal_entry();

  ~cal_entry();

  void clear() {
    this->f_unit = "Hz";
    this->a_unit = "mV/nT";
    this->p_unit = "deg";
    this->f.clear();
    this->cap.clear();
  }

/*!
 * @brief 
 * 
 * @param f double f_unit = "Hz"
 * @param a double a_unit = "V/(nT*Hz)"
 * @param p double p_unit = "deg"
 * @return actual data size
 */
  size_t add_from_mtx(const double &f, const double &a, const double &p) {

    if (f <= 0.0 || a < 0.0) {
        this->clear();
    }
    this->f.push_back(f);
    this->cap.push_back(std::polar(a * 1000.0 * f, (p * M_PI) / 180.0));

    return true;
  }

/*!
 * @brief 
 * 
 * @param f vector f_unit = "Hz";
 * @param a vector a_unit = "V/(nT*Hz)";
 * @param p vector p_unit = "deg"
 * @return true if size availabe
 */
  size_t to_mtx(std::vector<double>& fm, std::vector<double>& am, std::vector<double>& pm) {
    if (!this->check_fcap()) {
      return 0;
    }
    fm.resize(this->f.size());
    am.resize(this->f.size());
    pm.resize(this->f.size());
    size_t i = 0;
    for (auto const &fd : this->f) {
            fm[i] = fd;
        am[i] = (std::fabs(this->cap.at(i)) / 1000.0) / fd;
            pm[i] = std::arg(this->cap.at(i)) * (180.0 / M_PI);
            ++i;
    }

    return fm.size();
  }

  void mtx_units(std::string& f_unit_m, std::string& a_unit_m, std::string& p_unit_m) const {
    f_unit_m = "Hz";
    a_unit_m = "V/(nT*Hz)";
    p_unit_m = "deg";
  }

  // some sensors may have gain dependencies;
  // for most metronix sensors we just have chopper dependency
  int chopper = 0;                                                 //!< 0 = off (e.g. HF mode), 1 = on (e.g. LF mode)
  double gain_1 = 0.0;                                             //!< calibrated at this sensor gain
  double gain_2 = 0.0;                                             //!< calibrated at this sensor gain
  double gain_3 = 0.0;                                             //!< calibrated at this sensor gain
  std::complex<double> impedance = std::complex<double>(0.0, 0.0); //!< calibrated at this impedance

  std::string base_line_unit = "V"; //!< unused
  std::string f_unit = "Hz";        //!< frequency vector unit
  std::string a_unit = "mV/nT";     //!< amplitude vector unit
  std::string p_unit = "deg";       //!< phase vector unit (deg = 0 ... 360, rad 0 .. 2 * PI)

  std::vector<double> f;                 //!< frequency vector Hz
  std::vector<double> a;                 //!< amplitude vector mV/nT
  std::vector<double> p;                 //!< phase vector deg
  
  std::vector<std::complex<double>> cap; //!< [mV/nT] in the complex plane

  std::string sensor_name;
  int serial = 0;

private:

  bool check_fcap() const {
    if (!this->f.size())
      return false;
    if (this->f.size() != this->cap.size())
      return false;
    return true;
  }
};

#endif // CAL_ENTRY
