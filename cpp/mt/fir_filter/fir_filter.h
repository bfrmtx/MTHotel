#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include <memory>
#include <string>
#include <vector>

#include "atss.h"
#include "sqlite_handler.h"

#define db_sql_file "filter.sql3"

class fir_filter
{
public:
    fir_filter();

    ~fir_filter();

    std::shared_ptr<channel>  set_filter(std::shared_ptr<channel> &chan, const std::string &filter_type,
                                        const bool &shift_to_full_seconds = true, const int64_t grid_raster = 0);

    /*!
     * \brief filter does the actual filtering inside a thread
     */
    void filter();




private:

    std::string filter_type;            //!< mtx4
    std::shared_ptr<channel> in_chan, out_chan;
    int64_t grid = 0;                   //!< 0 filter to any time, 1 filter to next full second, 64 filter to 64 grid time
    size_t skip_samples_read = 0;
    std::vector<double> in, out, coeff;
    double filter_result;


    void shift_to_new_start_time(const bool &shift_to_full_seconds = true);

    void shift_to_grid(const int64_t grid_raster = 0);

    std::unique_ptr<sqlite_handler> sql_info;

};



#endif // FIR_FILTER_H
