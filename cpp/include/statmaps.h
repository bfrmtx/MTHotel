#ifndef STATMAPS_H
#define STATMAPS_H

#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
/**
  * @file statmaps.h
  * @brief defines map objects for statistical data
  *
  */

/*!
 * \brief statmap is a typedef for statistical data
 * you insert as follows variance_data[g_stat::min_x] = d_min; and find it later with: if min < variance_data[g_stat::min_x]
 */
typedef std::unordered_map <size_t, double> statmap;

/*
  *
  *
  *
  *  DO NOT INSERT DATA, APPEND! ELSE RE-NUMBER and check last_x and last_y which will used for copy data from x->y for 2D dists
  *
  *
  */

namespace g_stat {
static std::vector<std::string> gauss_names { "d_n_x", "sum_x", "mean_x", "median_x", "variance_population_x", "variance_x", "stddev_population_x", "stddev_x", "skewness_x", "kurtosis_x", "min_x", "max_x", "range_x",
    "d_n_y", "sum_y", "mean_y", "median_y", "variance_population_y", "variance_y", "stddev_population_y", "stddev_y", "skewness_y", "kurtosis_y", "min_y", "max_y", "range_y",
    "covariance_xy", "correlation_xy", "slope_xy", "delta_slope_xy", "abscissa_xy", "sum_alldist_from_slope_xy", "student_t_upper_quantile", "student_t_inv",
    "student_t_upper_confidence_xy", "student_t_lower_confidence_xy", "student_t_test_against_other_slope"
};


// a map or unordered map creates in case of non existance - to avoid that you must check in advance
//  copy range with not existing data
//  if (x_variance_data.find(i) != x_variance_data.cend()) datamap[i] = x_variance_data.at(i);
//


enum g_stat : std::size_t {

    // variance data compact keep the same for y, e need the symetry here
    d_n_x = 0,
    sum_x = 1,
    mean_x = 2,
    median_x = 3,
    variance_population_x = 4,
    variance_x = 5,
    stddev_population_x = 6,
    stddev_x = 7,
    skewness_x = 8,
    kurtosis_x = 9,
    min_x = 10,
    max_x = 11,
    range_x = 12,

    xstat_size = 13,   // not inside iterator and string list!

    // //////////////////////////////////////////////
    d_n_y = xstat_size,
    sum_y = xstat_size + 1,
    mean_y = xstat_size + 2,
    median_y = xstat_size + 3,
    variance_population_y = xstat_size + 4,
    variance_y = xstat_size + 5,
    stddev_population_y = xstat_size + 6,
    stddev_y = xstat_size + 7,
    skewness_y = xstat_size + 8,
    kurtosis_y = xstat_size + 9,
    min_y = xstat_size + 10,
    max_y = xstat_size + 11,
    range_y = xstat_size + 12,

    ystat_size = 26, // not inside iterator and string list!

    // that was the same as above

    covariance_xy = ystat_size,
    correlation_xy = ystat_size + 1,
    slope_xy = ystat_size + 2,
    delta_slope_xy = ystat_size + 3,
    abscissa_xy = ystat_size + 4,
    sum_alldist_from_slope_xy = ystat_size + 5,
    student_t_upper_quantile = ystat_size + 6,
    student_t_inv = ystat_size + 7,
    student_t_upper_confidence_xy = ystat_size + 8,
    student_t_lower_confidence_xy = ystat_size + 9,
    student_t_test_against_other_slope = ystat_size + 10,

    statmap_size = 37 // not inside iterator and string list!


};

template<class T> size_t statmap_name_to_idx (const std::string_view &statname){

    auto it = std::find(gauss_names.begin(), gauss_names.end(), statname);

    if (it != gauss_names.end())  {
        return size_t(distance(gauss_names.begin(), it));
    }
    return g_stat::statmap_size;

}

} // end namespace


#endif // STATMAPS_H
