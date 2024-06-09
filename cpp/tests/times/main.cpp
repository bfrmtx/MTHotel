#include "atss.h"
#include "strings_etc.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

#include "bthread.h"

template <class Rep, std::intmax_t num, std::intmax_t denom>
auto chronoBurst(std::chrono::duration<Rep, std::ratio<num, denom>> d) {
  const auto hrs = duration_cast<std::chrono::hours>(d);
  const auto mins = duration_cast<std::chrono::minutes>(d - hrs);
  const auto secs = duration_cast<std::chrono::seconds>(d - hrs - mins);
  const auto ms = duration_cast<std::chrono::milliseconds>(d - hrs - mins - secs);

  return std::make_tuple(hrs, mins, secs, ms);
}

/*
    auto [hrs, mins, secs, ms] = chronoBurst(dt);
    std::cout << dt.count() << "s, casts: " << hrs.count() <<"h " << mins.count() << "m " << secs.count() << "s" << std::endl;

    t1->pt = t1->pt + diff_t1_t2;
    std::cout << "adding time diff to 2009-08-20T13:22:01 -> result 2009-08-20T15:52:13  ...: " << t1->pt.start_datetime() << std::endl;

    auto bs = chronoBurst(hz1024);
    std::cout << hz1024.count() << "s, casts: " << get<0>(bs).count() <<"h " << get<1>(bs).count() << "m " << get<2>(bs).count() << "s " << get<3>(bs).count() << "ms" << std::endl;
    std::cout << get<0>(bs).count() << std::endl;
*/

int main() {

  double out_fracs = 0.0;

  std::vector<std::string> ts;
  ts.emplace_back("2009-08-20T13:22:01.04"); // with frac
  ts.emplace_back("2009-08-20T13:22:01");    // w/o frac
  uint64_t n_2 = 1250774521;                 // that is the date above for [1]
  ts.emplace_back("1969-12-31T23:59:01");    // what time_t implementation we have?
  ts.emplace_back("2020-09-14T12:32:01Z");   //  zulu

  size_t i = 0;
  std::cout << std::endl;
  for (const auto &str : ts) {
    auto date = mstr::get_date_from_iso8601(str);
    auto time = mstr::get_time_from_iso8601(str);
    auto time_fracs = mstr::get_time_fracs_from_iso8601(str);
    double fracs = mstr::get_fracs_from_iso8601(str);
    auto r_1 = mstr::time_t_iso_8601_str(str);
    std::cout << "input: " << str << std::endl;
    std::cout << date << " " << time << " " << fracs << ", time with fracs " << time_fracs << std::endl;
    auto back = mstr::iso8601_time_t(r_1, 0, out_fracs);
    std::cout << "back conversion: " << back << std::endl;
    std::cout << "timestamp: " << r_1 << std::endl;
    if (i == 1) {
      std::cout << "original timestamp for example 1: " << n_2 << std::endl;
    }
    auto compare_date = std::gmtime(&r_1);
    std::string readable_time(asctime(compare_date));
    std::cout << "gmt time as string " << readable_time << std::endl;
    std::cout << std::endl;

    if (i == 1) {
      std::cout << "adding 120 s " << std::endl;
      r_1 += 120;
      std::cout << mstr::iso8601_time_t(r_1, 0, out_fracs) << std::endl;
    }

    ++i;
  }

  std::cout << std::endl;
  std::cout << "channel time end check" << std::endl
            << std::endl;

  std::vector<std::shared_ptr<channel>> chans;
  std::vector<std::shared_ptr<channel>> chans_srt;

  std::vector<size_t> use_secs{60, 3900, 62, 120, 60};
  std::vector<size_t> add_samps{0, 0, 512, 0, 0};
  i = 0;
  for (const auto &us : use_secs) {
    chans.emplace_back(std::make_shared<channel>("Hx", 1024, "2009-08-20T13:22:01"));
    chans_srt.emplace_back(std::make_shared<channel>("Hx", 1024, "2009-08-20T13:22:01"));
    chans.back()->pt.set_max_samples(1024 * us + add_samps.at(i++));
    std::cout << "start time: " << chans.back()->get_datetime() << std::endl;
    std::cout << "stop time: " << chans.back()->stop_datetime(0) << std::endl
              << std::endl;
  }
  std::cout << std::endl;

  std::cout << "channel time start check" << std::endl
            << std::endl;
  auto tt = mstr::time_t_iso_8601_str("2009-08-20T13:22:01");
  i = 0;
  for (const auto &us : use_secs) {
    auto str = mstr::iso8601_time_t(tt + us);
    //  chans_srt[i]->set_date_time(str);  // that is the pointer access
    chans_srt.at(i++)->set_datetime(str); // pointer access via at, address is unchanged, works
  }

  std::sort(chans_srt.begin(), chans_srt.end(), compare_channel_start_lt);

  for (i = 0; i < chans.size(); ++i) {
    std::cout << chans.at(i)->get_datetime() << " sorted " << chans_srt.at(i)->get_datetime() << std::endl;
  }

  auto t1 = std::make_shared<channel>("Hx", 1024, "2009-08-20T13:22:01");
  auto t2 = std::make_shared<channel>("Hx", 1024, "2009-08-20T15:52:13");

  p_timer diff_t1_t2 = t2->pt - t1->pt;

  std::cout << " strange but possible" << mstr::iso8601_time_t(diff_t1_t2.tt, 0, diff_t1_t2.fracs) << std::endl;
  std::chrono::duration<double, std::ratio<1, 1024>> hz1024(0);
  std::chrono::duration<double, std::ratio<1, 1024>> hz2(30.5);

  hz1024 += std::chrono::seconds(1);
  hz1024++;
  hz1024 += std::chrono::duration<int64_t, std::ratio<1, 1024>>(10);

  std::cout << "duri " << hz1024.count() << " " << hz2.count() << std::endl;

  std::chrono::seconds dt(diff_t1_t2.tt);

  if (compare_channel_start_lt(t1, t2)) {
    std::cout << "t1 start time early" << std::endl;
  }

  // std::cout << hz30 << std::end;

  std::cout << "main finished " << std::endl;
  return 0;
}
