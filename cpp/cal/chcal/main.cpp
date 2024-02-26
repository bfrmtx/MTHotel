#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>

#include "cal_synthetic.h"
#include "freqs.h"
#include "gnuplotter.h"
#include "read_cal.h"
#include "sqlite_handler.h" // get reference data from sqlite

/// test
#include "vector_math.h"
#define INTERPOL_TEST

int main(int argc, char **argv) {
  using namespace std;
  namespace fs = std::filesystem;

  auto exec_path = std::filesystem::path(argv[0]);

  bool tojson = false;
  bool toxml = false;
  bool tomtx = false;
  bool ampl_div_f = false;
  bool ampl_mul_f = false;
  bool ampl_mul_by_1000 = false;
  bool old_to_new = false;
  bool new_to_old = false;
  bool force_measdoc = false;
  bool force_single = false;
  bool keep_name = false;

  bool gen_cal = false;
  std::string gen_cal_sensor;

  // plot control
  bool plot = false;
  bool pdf = false;
  bool svg = false;
  bool on = false;
  bool off = false;

  std::pair<double, double> f_range = {0, 0};
  std::pair<double, double> a_range = {0, 0};
  std::pair<double, double> p_range = {0, 0};

  fs::path outdir;

  unsigned l = 1;
  while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
    std::string marg(argv[l]);

    if (marg.compare("-tojson") == 0) {
      tojson = true;
    }
    if (marg.compare("-toxml") == 0) {
      toxml = true;
    }
    if (marg.compare("-tomtx") == 0) {
      tomtx = true;
    }
    if (marg.compare("-ampl_div_f") == 0) {
      ampl_div_f = true;
    }
    if (marg.compare("-ampl_mul_f") == 0) {
      ampl_mul_f = true;
    }
    if (marg.compare("-ampl_mul_by_1000") == 0) {
      ampl_mul_by_1000 = true;
    }
    if (marg.compare("-old_to_new") == 0) {
      old_to_new = true;
    }
    if (marg.compare("-new_to_old") == 0) {
      new_to_old = true;
    }
    if (marg.compare("-force_measdoc") == 0) {
      force_measdoc = true;
    }
    if (marg.compare("-force_single") == 0) {
      force_single = true;
    }

    if (marg.compare("-keep_name") == 0) {
      keep_name = true;
    }

    if (marg.compare("-plot") == 0) {
      plot = true;
    }

    if (marg.compare("-pdf") == 0) {
      pdf = true;
    }

    if (marg.compare("-svg") == 0) {
      svg = true;
    }

    if (marg.compare("-on") == 0) {
      on = true;
    }

    if (marg.compare("-off") == 0) {
      off = true;
    }
    bool exit_flag = false;
    if (marg.compare("-f_range") == 0) {
      f_range.first = std::stod(argv[++l]);
      f_range.second = std::stod(argv[++l]);
      if (f_range.first > f_range.second) {
        std::cout << " must be min max: f_range.first < f_range.second" << std::endl;
        exit_flag = true;
      }
      if (f_range.first <= 0) {
        std::cout << " f_range.first > 0 for log plot" << std::endl;
        exit_flag = true;
      }
    }

    if (marg.compare("-a_range") == 0) {
      a_range.first = std::stod(argv[++l]);
      a_range.second = std::stod(argv[++l]);
      if (a_range.first > a_range.second) {
        std::cout << " must be min max: a_range.first < a_range.second" << std::endl;
        exit_flag = true;
      }
      if (a_range.first <= 0) {
        std::cout << " a_range.first > 0 for log plot" << std::endl;
        exit_flag = true;
      }
    }

    if (marg.compare("-p_range") == 0) {
      p_range.first = std::stod(argv[++l]);
      p_range.second = std::stod(argv[++l]);
      if (p_range.first > p_range.second) {
        std::cout << " must be min max: p_range.first < p_range.second" << std::endl;
        exit_flag = true;
      }
    }

    if (marg.compare("-gen_cal") == 0) {
      gen_cal = true;
      gen_cal_sensor = std::string(argv[++l]);
    }

    if (exit_flag) {
      std::cout << " exit failure setting axis " << std::endl;
      return EXIT_FAILURE;
    }

    if (marg.compare("-outdir") == 0) {
      outdir = std::string(argv[++l]);
      try {
        if (!fs::exists(outdir))
          fs::create_directories(outdir);
      } catch (...) {
        std::cerr << "could not create outdir " << argv[l - 1] << std::endl;
        return EXIT_FAILURE;
      }
      outdir = fs::canonical(outdir);
      if (fs::exists(outdir)) {
        std::cout << "using output dir " << outdir << std::endl;
      } else {
        std::cerr << "outdir " << outdir << " does not exist" << std::endl;
        return EXIT_FAILURE;
      }
    }
    if ((marg.compare("-help") == 0) || (marg.compare("--help") == 0)) {
      std::cout << "Sensor and Serial are derived from filename ONLY!" << std::endl;
      std::cout << "-toxml file.txt" << std::endl;
      std::cout << "-toxml *.txt " << std::endl;
      std::cout << "-old_to_new -tojson file.txt" << std::endl;
      std::cout << "-old_to_new -tojson *.txt " << std::endl;
      std::cout << "you may want to call " << std::endl;
      std::cout << "-outdir /home/newcal -old_to_new -tojson *.txt " << std::endl;
      std::cout << " use -outdir [options] *txt in order to place the results at a different place" << std::endl;
      std::cout << "-keep_name should be active for txt -> xml when using script files ancient style" << std::endl;
      std::cout << "otherwise file name would be " << std::endl;

      return EXIT_FAILURE;
    } else if (marg.compare("-") == 0) {
      std::cerr << "\nunrecognized option " << argv[l] << std::endl;
      return EXIT_FAILURE;
    }
    ++l;
  }

  if (!outdir.empty() && !plot) {
    if (!std::filesystem::exists(outdir)) {
      std::filesystem::create_directory(outdir);
      if (!std::filesystem::exists(outdir)) {
        std::cout << "can not create outdir" << std::endl;
        return EXIT_FAILURE;
      }
      std::cout << outdir << " created" << std::endl;
    }
  }

  if (tojson && !old_to_new) {
    std::cout << "  ***************************************************************   " << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "  YOU MAY WANTED TO CALL  " << std::endl;
    std::cout << "-outdir /home/newcal -old_to_new -tojson *.txt " << std::endl;
    std::cout << "-old_to_new -tojson file.txt" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "  ***************************************************************   " << std::endl;
  }

  std::vector<std::shared_ptr<calibration>> cals;
  std::multimap<fs::path, size_t> mtxfiles_and_cals;

  l = 1;
  while (argc > 1 && (l < unsigned(argc))) {

    fs::path check_ext(argv[l]);
    if (check_ext.has_extension()) {
      if ((check_ext.extension() == ".txt") || (check_ext.extension() == ".TXT")) {
        try {
          std::shared_ptr<read_cal> mtx_cal_file_on = std::make_shared<read_cal>();
          std::shared_ptr<read_cal> mtx_cal_file_off = std::make_shared<read_cal>();
          cals.emplace_back(mtx_cal_file_off->read_std_mtx_txt(check_ext, ChopperStatus::off));
          if (cals.back()->is_empty())
            cals.pop_back();
          else if (cals.size())
            mtxfiles_and_cals.emplace(fs::path(check_ext), cals.size() - 1);
          cals.emplace_back(mtx_cal_file_on->read_std_mtx_txt(check_ext, ChopperStatus::on));
          if (cals.back()->is_empty())
            cals.pop_back();
          else if (cals.size())
            mtxfiles_and_cals.emplace(fs::path(check_ext), cals.size() - 1);

        } catch (const std::runtime_error &error) {
          std::cerr << error.what() << std::endl;
          cals.clear();
        }
      }

      if ((check_ext.extension() == ".xml") || (check_ext.extension() == ".XML")) {
        try {

          std::string messages;

          std::shared_ptr<read_cal> mtx_cal_file = std::make_shared<read_cal>();
          std::vector<std::shared_ptr<calibration>> xcals;
          std::string fdig(check_ext.stem().string());
          if ((isdigit(fdig.at(0)) || force_measdoc) && !force_single)
            xcals = mtx_cal_file->read_std_xml(check_ext, messages);
          else
            xcals = mtx_cal_file->read_std_xml_single(check_ext);

          for (auto &xcal : xcals) {
            if (!xcal->is_empty()) {
              cals.emplace_back(xcal);
            }
          }

        } catch (const std::runtime_error &error) {
          std::cerr << error.what() << std::endl;
          cals.clear();
        }
      }
      if ((check_ext.extension() == ".json") || (check_ext.extension() == ".JSON")) {

        try {
          auto cal = std::make_shared<calibration>();
          cal->read_file(check_ext, true);
          if (!cal->is_empty())
            cals.emplace_back(cal);
        } catch (const std::runtime_error &error) {
          std::cerr << error.what() << std::endl;
          cals.clear();
        }
      }
    }
    ++l;
  }

  if (!cals.size() && !gen_cal) {
    std::cout << "no calibrations found / loaded" << std::endl;
    return EXIT_FAILURE;
  }

  if (gen_cal && !cals.size()) { // if you make a stand alone calibration you need a frequency vector
    if (f_range.second - f_range.first < 1) {
      std::cout << "f_range must be set at least to one Hz" << std::endl;
      std::cout << "f_range.second - f_range.first = " << f_range.second - f_range.first << std::endl;
      return EXIT_FAILURE;
    }
  }

  // *******************************************   G E N E R A T E  O N L Y - no data    **************************************************************************
  if (gen_cal && !cals.size()) {
    auto synthetic_f = gen_equidistant_logvector(f_range.first, f_range.second, 11);
    auto synthetic_ap_cplx_on = gen_trf_mfs06e(synthetic_f, ChopperStatus::on);
    auto synthetic_ap_cplx_off = gen_trf_mfs06e(synthetic_f, ChopperStatus::off);
    std::vector<double> synthetic_a_on;
    std::vector<double> synthetic_p_on;
    std::vector<double> synthetic_a_off;
    std::vector<double> synthetic_p_off;

    std::string init_err;

    bvec::cplx2ap(synthetic_ap_cplx_off, synthetic_a_off, synthetic_p_off, true);
    bvec::cplx2ap(synthetic_ap_cplx_on, synthetic_a_on, synthetic_p_on, true);

    auto gplt_synthetic_a = std::make_shared<gnuplotter<double, double>>(init_err);
    if (!init_err.empty()) {
      std::cout << init_err << std::endl;
      return EXIT_FAILURE;
    }
    gplt_synthetic_a->cmd << "set terminal qt size 1024,768 enhanced" << std::endl;
    gplt_synthetic_a->cmd << "set title 'Theoretical Amplitude Chopper on /off'" << std::endl;
    gplt_synthetic_a->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
    gplt_synthetic_a->cmd << "set ylabel 'amplitude'" << std::endl;
    gplt_synthetic_a->cmd << "set grid" << std::endl;
    gplt_synthetic_a->cmd << "set format y '1E^%+03T'" << std::endl;
    gplt_synthetic_a->cmd << "set logscale xy" << std::endl;
    gplt_synthetic_a->set_x_range(f_range);
    gplt_synthetic_a->set_y_range(a_range);

    // divide _a_on by f
    if (ampl_div_f) {
      for (auto &a : synthetic_a_on) {
        a /= synthetic_f.at(&a - &synthetic_a_on.at(0));
      }
      for (auto &a : synthetic_a_off) {
        a /= synthetic_f.at(&a - &synthetic_a_off.at(0));
      }
    }

    gplt_synthetic_a->set_xy_linespoints(synthetic_f, synthetic_a_on, gen_cal_sensor + " on", 1, 2);
    gplt_synthetic_a->set_xy_linespoints(synthetic_f, synthetic_a_off, gen_cal_sensor + " off", 1, 2);
    gplt_synthetic_a->plot();

    return EXIT_SUCCESS;
  }
  // end generate only ****************************************************************************************************************************

  // in case operations are forced - do it here
  for (auto &cal : cals) {
    cal->tasks_todo(ampl_div_f, ampl_mul_f, ampl_mul_by_1000, old_to_new, new_to_old);
  }

  // make the plots and then exit --- do not mixt graphical output with conversion tasks!

  // *******************************************   P L O T S     **************************************************************************

  if (plot) {

    try {

      std::vector<std::shared_ptr<calibration>> cals_on;
      std::vector<std::shared_ptr<calibration>> cals_off;

      for (const auto &cal : cals) {
        if (cal->chopper == ChopperStatus::on)
          cals_on.emplace_back(cal);
        else
          cals_off.emplace_back(cal);
      }

      // // find the ranges comment out
      // std::pair<double, double> f_range16 = {14, 18};
      // std::pair<double, double> p_range16 = {16.0, 20};
      // for (const auto &cal : cals_on) {
      //   cal->find_fRange_valueRange(f_range16, p_range16, true);
      // }
      // p_range16.first = 10.;
      // p_range16.second = 14.1;
      // for (const auto &cal : cals_on) {
      //   cal->find_fRange_valueRange(f_range16, p_range16, true);
      // }

      // end comment out

      // start with amplitude plot
      std::string init_err;
      auto colline = gnuplot_next_meas_point_type_simple("blue");

      if (cals_on.size() && on) {

        auto gplt_cal_a_on = std::make_shared<gnuplotter<double, double>>(init_err);
        if (!init_err.empty()) {
          std::cout << init_err << std::endl;
          return EXIT_FAILURE;
        }
        // gplt_cal_a_on->cmd << "set terminal qt size 1024,768 enhanced" << std::endl;
        if (pdf)
          gplt_cal_a_on->set_pdf_terminal(outdir, cals_on.at(0)->basename(), "Calibration Amplitude Chopper on", 1);
        else if (svg)
          gplt_cal_a_on->set_svg_terminal(outdir, cals_on.at(0)->basename(), "Calibration Amplitude Chopper on", 1);
        else
          gplt_cal_a_on->set_qt_terminal("Calibration Amplitude Chopper on");
        // gplt_cal_a_on->cmd << "set title 'Calibration Amplitude Chopper on'" << std::endl;
        gplt_cal_a_on->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
        gplt_cal_a_on->cmd << "set ylabel 'amplitude'" << std::endl;
        gplt_cal_a_on->cmd << "set grid" << std::endl;
        gplt_cal_a_on->cmd << "set format y '1E^%+03T'" << std::endl;
        gplt_cal_a_on->cmd << "set logscale xy" << std::endl;
        gplt_cal_a_on->set_x_range(f_range);
        gplt_cal_a_on->set_y_range(a_range);

        for (auto &cal : cals_on) {
          gplt_cal_a_on->set_xy_linespoints(cal->f, cal->a, cal->sensor + " " + cal->serial2string(4), 1, 2, colline.meas_point_type());
        }
        gplt_cal_a_on->plot();
      }

      if (cals_off.size() && off) {
        colline.reset();
        auto gplt_cal_a_off = std::make_shared<gnuplotter<double, double>>(init_err);
        if (!init_err.empty()) {
          std::cout << init_err << std::endl;
          return EXIT_FAILURE;
        }
        // gplt_cal_a_off->cmd << "set terminal qt size 1024,768 enhanced" << std::endl;
        if (pdf)
          gplt_cal_a_off->set_pdf_terminal(outdir, cals_off.at(0)->basename(), "Calibration Amplitude Chopper off", 1);
        else if (svg)
          gplt_cal_a_off->set_svg_terminal(outdir, cals_off.at(0)->basename(), "Calibration Amplitude Chopper off", 1);
        else
          gplt_cal_a_off->set_qt_terminal("Calibration Amplitude Chopper off");
        // gplt_cal_a_off->cmd << "set title 'Calibration Amplitude Chopper on'" << std::endl;
        gplt_cal_a_off->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
        gplt_cal_a_off->cmd << "set ylabel 'amplitude'" << std::endl;
        gplt_cal_a_off->cmd << "set grid" << std::endl;
        gplt_cal_a_off->cmd << "set format y '1E^%+03T'" << std::endl;
        gplt_cal_a_off->cmd << "set logscale xy" << std::endl;
        gplt_cal_a_off->set_x_range(f_range);
        gplt_cal_a_off->set_y_range(a_range);
        for (auto &cal : cals_off) {
          gplt_cal_a_off->set_xy_linespoints(cal->f, cal->a, cal->sensor + " " + cal->serial2string(4), 1, 2, colline.meas_point_type());
        }
        gplt_cal_a_off->plot();
      }

      if (cals_on.size() && on) {
        colline.reset();
        colline.set_color("red");
        auto gplt_cal_p_on = std::make_shared<gnuplotter<double, double>>(init_err);
        if (!init_err.empty()) {
          std::cout << init_err << std::endl;
          return EXIT_FAILURE;
        }
        // gplt_cal_p_on->cmd << "set terminal qt size 1024,768 enhanced" << std::endl;
        if (pdf)
          gplt_cal_p_on->set_pdf_terminal(outdir, cals_on.at(0)->basename(), "Calibration Phase Chopper on", 1);
        else if (svg)
          gplt_cal_p_on->set_svg_terminal(outdir, cals_on.at(0)->basename(), "Calibration Phase Chopper on", 1);
        else
          gplt_cal_p_on->set_qt_terminal("Calibration Phase Chopper on");
        // gplt_cal_p_on->cmd << "set title 'Calibration Amplitude Chopper on'" << std::endl;
        gplt_cal_p_on->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
        gplt_cal_p_on->cmd << "set ylabel 'phase [deg]'" << std::endl;
        gplt_cal_p_on->cmd << "set grid" << std::endl;
        // gplt_cal_p_on->cmd << "set format y '1E^%+03T'" << std::endl;
        gplt_cal_p_on->cmd << "set logscale x" << std::endl;
        gplt_cal_p_on->set_x_range(f_range);
        gplt_cal_p_on->set_y_range(p_range);
        for (auto &cal : cals_on) {
          gplt_cal_p_on->set_xy_linespoints(cal->f, cal->p, cal->sensor + " " + cal->serial2string(4), 1, 2, colline.meas_point_type());
        }
        gplt_cal_p_on->plot();
      }

      if (cals_off.size() && off) {
        colline.reset();
        auto gplt_cal_p_off = std::make_shared<gnuplotter<double, double>>(init_err);
        if (!init_err.empty()) {
          std::cout << init_err << std::endl;
          return EXIT_FAILURE;
        }
        // gplt_cal_p_off->cmd << "set terminal qt size 1024,768 enhanced" << std::endl;
        if (pdf)
          gplt_cal_p_off->set_pdf_terminal(outdir, cals_off.at(0)->basename(), "Calibration Phase Chopper off", 1);
        else if (svg)
          gplt_cal_p_off->set_svg_terminal(outdir, cals_off.at(0)->basename(), "Calibration Phase Chopper off", 1);
        else
          gplt_cal_p_off->set_qt_terminal("Calibration Phase Chopper off");
        // gplt_cal_p_off->cmd << "set title 'Calibration Amplitude Chopper on'" << std::endl;
        gplt_cal_p_off->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
        gplt_cal_p_off->cmd << "set ylabel 'phase [deg]'" << std::endl;
        gplt_cal_p_off->cmd << "set grid" << std::endl;
        // gplt_cal_p_off->cmd << "set format y '1E^%+03T'" << std::endl;
        gplt_cal_p_off->cmd << "set logscale x" << std::endl;
        gplt_cal_p_off->set_x_range(f_range);
        gplt_cal_p_off->set_y_range(p_range);
        for (auto &cal : cals_off) {
          gplt_cal_p_off->set_xy_linespoints(cal->f, cal->p, cal->sensor + " " + cal->serial2string(4), 1, 2, colline.meas_point_type());
        }
        gplt_cal_p_off->plot();
      }

      // ******************************** interpolation test *******************************************************************************
#ifdef INTERPOL_TEST

      init_err.clear();
      if (cals_off.size() && off) {
        colline.reset();
        auto gplt_cal_a_off_x = std::make_shared<gnuplotter<double, double>>(init_err);
        if (!init_err.empty()) {
          std::cout << init_err << std::endl;
          return EXIT_FAILURE;
        }
        // gplt_cal_a_off_x->cmd << "set terminal qt size 1024,768 enhanced" << std::endl;
        if (pdf)
          gplt_cal_a_off_x->set_pdf_terminal(outdir, cals_off.at(0)->basename(), "Calibration Phase Chopper off INTER", 1);
        else if (svg)
          gplt_cal_a_off_x->set_svg_terminal(outdir, cals_off.at(0)->basename(), "Calibration Phase Chopper off INTER", 1);
        else
          gplt_cal_a_off_x->set_qt_terminal("Calibration Phase Chopper off");
        // gplt_cal_a_off_x->cmd << "set title 'Calibration Amplitude Chopper on'" << std::endl;
        gplt_cal_a_off_x->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
        gplt_cal_a_off_x->cmd << "set ylabel 'phase [deg]'" << std::endl;
        gplt_cal_a_off_x->cmd << "set grid" << std::endl;
        // gplt_cal_a_off_x->cmd << "set format y '1E^%+03T'" << std::endl;
        gplt_cal_a_off_x->cmd << "set logscale x" << std::endl;
        gplt_cal_a_off_x->set_x_range(f_range);
        gplt_cal_a_off_x->set_y_range(p_range);
        for (auto &cal : cals_off) {

          std::vector<double> f;
          f.reserve(10 * cal->f.size() + 1);
          // interpolate cal->f with 10 values in between
          for (size_t i = 0; i < cal->f.size() - 1; ++i) {
            double dx = (cal->f.at(i + 1) - cal->f.at(i)) / 10.;
            for (size_t j = 0; j < 10; ++j) {
              f.emplace_back(cal->f.at(i) + j * dx);
            }
          }
          std::vector<double> p;
          bvec::akima_vector_double(cal->f, cal->a, f, p);

          gplt_cal_a_off_x->set_xy_linespoints(f, p, cal->sensor + " " + cal->serial2string(4), 1, 2, colline.meas_point_type());
        }
        gplt_cal_a_off_x->plot();
      }
#endif

    } catch (const std::runtime_error &error) {
      std::cerr << error.what() << std::endl;
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  }

  // *******************************************   C O N V E R S I O N     **************************************************************************

  l = 0;
  // keep name should be active for txt -> xml when using script files ancient style
  if (!keep_name || (cals.size() != mtxfiles_and_cals.size())) {

    // if outdir does not exists, exit
    if (!std::filesystem::exists(outdir)) {
      std::cout << "outdir for placing files not exists!!" << std::endl;
      return EXIT_FAILURE;
    }

    mtxfiles_and_cals.clear();
    for (auto &cal : cals) {
      mtxfiles_and_cals.emplace(cal->mtx_cal_head(outdir, true), l++);
    }
  }

  if (toxml) {

    fs::path y;
    for (const auto &x : mtxfiles_and_cals) {

      if (y != x.first) {
        std::pair<std::multimap<fs::path, size_t>::iterator, std::multimap<fs::path, size_t>::iterator> ret;
        fs::path outxmlfile;
        if (!outdir.empty()) {
          outxmlfile = outdir;
          outxmlfile /= x.first.filename();
        } else
          outxmlfile = x.first;
        outxmlfile.replace_extension("xml");
        fs::path outxmlfile_plot(outxmlfile);
        outxmlfile_plot.replace_filename(outxmlfile.stem() += "_plot.xml");

        auto tix = std::make_shared<tinyxmlwriter>(true, outxmlfile);
        auto tixplot = std::make_shared<tinyxmlwriter>(true, outxmlfile_plot);

        ret = mtxfiles_and_cals.equal_range(x.first);
        std::cout << x.first << " -> " << outxmlfile << " =>";
        int segments = 0;
        for (std::multimap<fs::path, size_t>::iterator it = ret.first; it != ret.second; ++it) {
          if (!segments) {
            if (it->second < cals.size()) {
              cals.at(it->second)->add_to_xml_1_of_3(tix);
              cals.at(it->second)->add_to_xml_2_of_3(tix);
              tixplot->push("calibration_sensors");
              tixplot->push("channel", "id", it->second);
              cals.at(it->second)->add_to_xml_1_of_3(tixplot);
              cals.at(it->second)->add_to_xml_2_of_3(tixplot);
            }
          } else {
            if (it->second < cals.size()) {
              cals.at(it->second)->add_to_xml_2_of_3(tix);
              cals.at(it->second)->add_to_xml_2_of_3(tixplot);
            }
          }
          ++segments;
          std::cout << ' ' << it->second;
        }
        // below add_to_xml_3_of_3(tix);
        tix->pop("calibration");
        tix->write_file();
        tixplot->pop("calibration");
        tixplot->pop("channel");
        tixplot->pop("calibration_sensors");
        tixplot->write_file();

        std::cout << '\n';
      }
      y = x.first;
    }
  }

  if (tojson) {
    for (auto &cal : cals) {
      cal->write_file(outdir);
    }
  }

  if (tomtx) {

    fs::path y;
    for (const auto &x : mtxfiles_and_cals) {

      if (y != x.first) {
        std::pair<std::multimap<fs::path, size_t>::iterator, std::multimap<fs::path, size_t>::iterator> ret;

        ret = mtxfiles_and_cals.equal_range(x.first);
        std::cout << x.first << " =>";
        int segments = 0;
        std::filesystem::path full_name;
        for (std::multimap<fs::path, size_t>::iterator it = ret.first; it != ret.second; ++it) {
          if (!segments) {
            if (it->second < cals.size()) {
              full_name = cals.at(it->second)->mtx_cal_head(outdir, false);
              cals.at(it->second)->mtx_cal_body(full_name);
            }
          } else {
            if (it->second < cals.size()) {
              cals.at(it->second)->mtx_cal_body(full_name);
            }
          }
          ++segments;
          std::cout << ' ' << it->second;
        }
        std::cout << '\n';
      }
      y = x.first;
    }
  }

  if (tojson && !old_to_new) {
    std::cout << "  ***************************************************************   " << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "  YOU MAY WANTED TO CALL  " << std::endl;
    std::cout << "-outdir /home/newcal -old_to_new -tojson *.txt " << std::endl;
    std::cout << "-old_to_new -tojson file.txt" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "  ***************************************************************   " << std::endl;
  }

  std::cout << endl
            << "finish write " << endl;

  return EXIT_SUCCESS;
}
