#ifndef ATSS_TIME_H
#define ATSS_TIME_H

#include <string>
#include <time.h> // time_t, struct tm, time, gmtime

#include "strings_etc.h" // convinence functions for strings

namespace mtime {

static const double zero_frac = 1E-6;

std::string iso_8601_str_d_str_d(const std::string &date, const std::string &time, const double &fracs = 0.0, const std::string separator = "T", const std::string TZ = "Z") {
  std::string dt = date + separator + time;
  std::string sfracs;
  if (fabs(fracs) > zero_frac) {
    sfracs = std::to_string(fracs);
    sfracs.erase(0, 1);
    mstr::removeTrailingCharacters(sfracs, '0');
    dt += sfracs;
  }
  return dt + TZ;
}
/*!
 * \brief time_t_iso_8601_str parses an ISO dat string like 2022-01-18T17:30:00.4Z or 2022-01-18T17:30:00Z or 2022-01-18T17:30:00.4
 * \param date ISO date string w or /wo ending Z os seconds like 00 or 00.4
 * \param fracs in case it has fraction of secnod, will be set here
 * \return time_t struct; time_t can NOT hold fractions of seconds
 */
time_t time_t_iso_8601_str(const std::string &date, double fracs = 0.0) {
  struct tm tt = {0};
  double dseconds;
  if (sscanf(date.c_str(), "%04d-%02d-%02dT%02d:%02d:%lfZ",
             &tt.tm_year, &tt.tm_mon, &tt.tm_mday,
             &tt.tm_hour, &tt.tm_min, &dseconds) != 6)
    return -1;
  double seconds;
  fracs = fabs(std::modf(dseconds, &seconds));
  if (fracs < zero_frac)
    fracs = 0.0;
  tt.tm_sec = (int)seconds;
  tt.tm_mon -= 1;
  tt.tm_year -= 1900;
  // tt.tm_isdst = 0;
  return std::mktime(&tt) - timezone;
}

std::string iso8601_time_t(const time_t &ti, std::string &date, std::string &time, const double &fracs = 0.0, const std::string separator = "T", const std::string TZ = "Z") {
  struct tm tt = {0};
  std::string sfracs;
  tt = *std::gmtime(&ti);
  date.clear();
  time.clear();
  date += std::to_string(tt.tm_year + 1900) + "-";
  date += mstr::zero_fill_field(tt.tm_mon + 1, 2) + "-";
  date += mstr::zero_fill_field(tt.tm_mday, 2);
  time += mstr::zero_fill_field(tt.tm_hour, 2) + ":";
  time += mstr::zero_fill_field(tt.tm_min, 2) + ":";
  time += mstr::zero_fill_field(tt.tm_sec, 2);
  if (fabs(fracs) > zero_frac) {
    sfracs = std::to_string(fracs);
    sfracs.erase(0, 1);
    mstr::removeTrailingCharacters(sfracs, '0');
  }
  return date + separator + time + sfracs + TZ;
}

std::string measdir_time(const time_t &ti) {
    struct tm tt = {0};
    std::string sfracs;
    tt = *std::gmtime(&ti);
    std::string date ("meas_");
    std::string time ("_");
    date += std::to_string(tt.tm_year + 1900) + "-";
    date += mstr::zero_fill_field(tt.tm_mon + 1, 2) + "-";
    date += mstr::zero_fill_field(tt.tm_mday, 2);
    time += mstr::zero_fill_field(tt.tm_hour, 2) + "-";
    time += mstr::zero_fill_field(tt.tm_min, 2) + "-";
    time += mstr::zero_fill_field(tt.tm_sec, 2);

    return date + time;
}

template <class T>
time_t add_d_m_y(const time_t &ti, const T d, const T m, const T y) {
  struct tm tt = {0};
  tt = *std::gmtime(&ti);
  tt.tm_mday += (int)d;
  tt.tm_mon += (int)m;
  tt.tm_year += (int)y;
  return std::mktime(&tt) - timezone;
}

/*
time_t parseiso8601utc(const char *date) {
  struct tm tt = {0};
    double seconds;
    if (sscanf(date, "%04d-%02d-%02dT%02d:%02d:%lfZ",
               &tt.tm_year, &tt.tm_mon, &tt.tm_mday,
               &tt.tm_hour, &tt.tm_min, &seconds) != 6)
        return -1;
    tt.tm_sec   = (int) seconds;
    tt.tm_mon  -= 1;
    tt.tm_year -= 1900;
    tt.tm_isdst =-1;
    return mktime(&tt) - timezone;
}
*/

} // end namespace mtime

#endif // ATSS_TIME_H
