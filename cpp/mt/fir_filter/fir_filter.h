#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include <memory>
#include <string>
#include <vector>

#include "atss.h"
#include "sqlite_handler.h"

#define db_sql_file "filter.sql3"

class fir_filter {
public:
  fir_filter();

  ~fir_filter();

  std::shared_ptr<channel> set_filter(std::shared_ptr<channel> &chan, const std::string &filter_type,
                                      const bool &shift_to_full_seconds = true, const int64_t grid_raster = 0);

  /*!
   * \brief filter does the actual filtering inside a thread
   */
  void filter();

  std::string get_info() const;

private:
  std::string filter_type;  //!< mtx4, mtx8, mtx32, mtx10, mtx15, mtx25
  size_t filter_factor = 0; //!< 4, 8, 32, 10, 15, 25
  std::shared_ptr<channel> in_chan, out_chan;
  int64_t grid = 0; //!< 0 filter to any time, 1 filter to next full second, 64 filter to 64 grid time
  size_t skip_samples_read = 0;
  std::vector<double> coeff; //!< filter coefficients
  int64_t full_secs = 0.0;   //!< full seconds of the start time of the input channel
  double frac_secs = 0.0;    //!< fraction of the start time of the input channel

  /*!
   * @brief @128 Hz and 32 x filter and 471 filter length; take (471-1)/2 samples = 235; need 21 extra skip = 256 = 2 full seconds
   for the next full second start time
   */
  size_t samples_skip = 0;  //!< sample to skip at the beginning of the input channel
  size_t samples_delay = 0; //!< samples to delay - aka filter length / 2
  void
  shift_to_new_start_time(const bool &shift_to_full_seconds = true);

  void shift_to_grid(const int64_t grid_raster = 0);

  std::unique_ptr<sqlite_handler> sql_filter;
  std::filesystem::path db_file; //!< path to the database file with filter coefficients
};

#endif // FIR_FILTER_H
