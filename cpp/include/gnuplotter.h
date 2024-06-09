#ifndef GNUPLOTTER_H
#define GNUPLOTTER_H

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <list>
#include <ostream>
#include <queue>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// a vector of point types "pt 1", "pt 2", "pt 3" and so on

static std::vector<std::string> gplt_blues = {"web-blue", "royalblue", "steelblue", "light-blue", "blue", "dark-blue", "midnight-blue", "medium-blue", "skyblue", "slateblue"};
static std::vector<std::string> gplt_reds = {"red", "dark-red", "light-red", "orange-red", "brown", "brown4", "sandybrown"};
static std::vector<std::string> gplt_greens = {"web-green", "dark-spring-green", "light-green", "green", "dark-green", "spring-green", "forest-green", "sea-green", "dark-olivegreen", "seagreen", "greenyellow"};
static std::vector<std::string> gplt_yellows = {"yellow", "dark-yellow", "yellow4", "light-goldenrod", "dark-goldenrod", "gold", "goldenrod"};
static std::vector<std::string> gplt_oranges = {"orange", "dark-orange", "orange-red", "coral", "orangered4"};
static std::vector<std::string> gplt_blacks = {"black", "grey30", "grey40", "grey50", "grey60", "grey70", "grey80", "grey90", "grey100"};

class gnuplot_next_meas_point_type {
public:
  gnuplot_next_meas_point_type() = default;
  ~gnuplot_next_meas_point_type() = default;
  std::vector<std::string> pt_types_gnuplot = {"pt 6", "pt 4", "pt 8", "pt 10", "pt 1", "pt 2"};
  size_t pt_types_gnuplot_index = 0;
  std::list<double> f_processed_gnuplot;
  std::string meas_point_type(const std::string &channel_type_trigger, const std::string &channel_type, const double &f) {

    if (channel_type_trigger == channel_type) {
      // if list does not contain f, add it
      if (std::find(f_processed_gnuplot.begin(), f_processed_gnuplot.end(), f) == f_processed_gnuplot.end()) {
        f_processed_gnuplot.push_back(f);
        // return the current index as long as it is in the list
        return pt_types_gnuplot[pt_types_gnuplot_index];
      } else {
        // clear list
        f_processed_gnuplot.clear();
        // increment index as long as it is in pt_types_gnuplot
        if (pt_types_gnuplot_index < pt_types_gnuplot.size() - 1) {
          return pt_types_gnuplot.at(pt_types_gnuplot_index++);
        } else {
          // clear reset index
          pt_types_gnuplot_index = 0;
          // std::cout << "reset index" << std::endl;
          return pt_types_gnuplot.at(pt_types_gnuplot_index++);
        }
      }
    }
    return "pt 6";
  }
};

class next_color {
public:
  next_color(const std::string &base_color) : base_color(base_color) {
    this->set_base_color(base_color);
  }

  void set_base_color(const std::string &base_color) {
    this->base_color = base_color;
    this->colors.clear();
    this->colors_index = 0;
    if (this->base_color == "blue") {
      this->colors = gplt_blues;
    } else if (this->base_color == "red") {
      this->colors = gplt_reds;
    } else if (this->base_color == "green") {
      this->colors = gplt_greens;
    } else if (this->base_color == "yellow") {
      this->colors = gplt_yellows;
    } else if (this->base_color == "orange") {
      this->colors = gplt_oranges;
    } else if (this->base_color == "black") {
      this->colors = gplt_blacks;
    } else {
      this->colors = gplt_blues;
    }
  }

  ~next_color() = default;

    std::string color() {
    std::string color;
    if (colors_index < colors.size()) {
      color = colors.at(colors_index++);
    } else {
      // clear reset index
      colors_index = 0;
      return colors.at(colors_index++);
    }
    return color;
  }
  std::string color_line() {
    std::string color = "lc rgbcolor \"";
    if (colors_index < colors.size()) {
      color += colors.at(colors_index++);
    } else {
      // clear reset index
      colors_index = 0;
      color += colors.at(colors_index++);
    }
    return color + "\" ";
  }

  void reset() { colors_index = 0; }

private:
  std::string base_color;
  std::vector<std::string> colors;
  size_t colors_index = 0;
};

class gnuplot_next_meas_point_type_simple {
public:
  gnuplot_next_meas_point_type_simple(const std::string color) {
    // lt rgb \"red\"
    this->nc = std::make_unique<next_color>(color);
    this->set_color(this->nc->color()); // get the first color
  }
  ~gnuplot_next_meas_point_type_simple() = default;
  void reset() {
    pt_types_gnuplot_index = 0;
    this->nc->reset();
    this->set_color(this->nc->color());
  }

  void set_color(const std::string &color) {
    this->nc->set_base_color(color);
    this->color = "lt rgb \"" + this->nc->color() + "\" ";
  }

  std::string meas_point_type() {
    if (pt_types_gnuplot_index < pt_types_gnuplot.size()) {
      // std::cout << this->color + pt_types_gnuplot.at(pt_types_gnuplot_index) << std::endl;
      return this->color + pt_types_gnuplot.at(pt_types_gnuplot_index++);
    } else {
      // clear reset index
      pt_types_gnuplot_index = 0;
      this->inc_color();
      // std::cout << "reset index & inc color" << std::endl;
      // std::cout << this->color + pt_types_gnuplot.at(pt_types_gnuplot_index) << std::endl;
      return this->color + pt_types_gnuplot.at(pt_types_gnuplot_index++);
    }
    return "pt 6";
  }

private:
  std::vector<std::string> pt_types_gnuplot = {"pt 6", "pt 4", "pt 8", "pt 10", "pt 1", "pt 2"};
  size_t pt_types_gnuplot_index = 0;
  std::string color;
  std::unique_ptr<next_color> nc;
  void inc_color() {
    this->color = "lt rgb \"" + this->nc->color() + "\" ";
  }
};

class concat_vector_xy {
public:
  concat_vector_xy() = default;
  ~concat_vector_xy() = default;

  void add(const std::vector<double> &f, const std::vector<double> &v) {
    this->data.insert(this->data.end(), v.begin(), v.end());
    this->freqs.insert(this->freqs.end(), f.begin(), f.end());
  }

  void clear() {
    this->data.clear();
    this->freqs.clear();
  }

  std::vector<double> data;
  std::vector<double> freqs;
};

// ### plot timedata equidistant as xtic label
// reset session

//$Data <<EOD
// 1609257628 2
// 1609256682 4
// 1609255914 1
// EOD

// myTimeFmt = "%d.%m.%Y %H:%M"

// plot $Data u 0:2:xtic(strftime(myTimeFmt,column(1))) w lp pt 7
// ### end of code

template <class T, class S>
class gnuplotter {
public:
  /*!
   * \brief gnuplotter
   * \param err_str returns the error in initialization; you should return EXIT_FAILURE in main
   * \param outfile_ writes a gnuplot script (with binary data) to script file; open with gnuplot -persist sprectra.bgp for example
   *
   * make a script file
   *   auto gplt_mm = std::make_unique<gnuplotter<double, double>>(init_err_mm, "/tmp/x.bgp");
   *   gplt_mm->cmd << "set terminal qt size 2048,1600 enhanced" << std::endl;
   *
   *   later you can use it as gnuplot -persist /tmp/x.bgp
   *
   * make a plot on screen - leave the filename empty
   *    auto gplt_mm = std::make_unique<gnuplotter<double, double>>(init_err_mm);
   *    gplt_mm->cmd << "set terminal qt size 2048,1600 enhanced" << std::endl;
   *
   * make a plot into file - leave the filename empty but use the output option
   *    auto gplt_mm = std::make_unique<gnuplotter<double, double>>(init_err_mm);
   *    gplt_mm->cmd << "set terminal svg size 2048,1600 enhanced" << std::endl;
   *    gplt_mm->cmd << "set output '/tmp/spectra.svg'" << std::endl;
   */
  gnuplotter(std::string &err_str, const std::string outfile_ = "") {

    try {
      if (outfile_.size()) {
        this->outfile = std::filesystem::path(outfile_);
        this->plotHandle = fopen(outfile_.c_str(), "w");

      } else {
        // windows maybe _popen
        this->plotHandle = popen("gnuplot -persist\n", "w");
      }
    } catch (const std::exception &ex) {
      err_str = __func__;
      err_str += " canonical path for gnuplot: " + outfile_ + err_str + " ::" + ex.what();
      return;
    }

    if (this->plotHandle == nullptr) {
      err_str = __func__;
      if (!this->outfile.empty())
        err_str += "::can not open gnuplot file!";
      else
        err_str += "::can not open gnuplot pipe!";
      return;
    }

    this->data_format << "format='";
    if (typeid(T) == typeid(double))
      this->data_format << "%float64";
    else if (typeid(T) == typeid(float))
      this->data_format << "%float32";
    else if (typeid(T) == typeid(int32_t))
      this->data_format << "%int32";
    else if (typeid(T) == typeid(int64_t))
      this->data_format << "%int64";
    else {
      err_str = __func__;
      err_str += "::xaxis can be double, float, int32_t or int64_t ONLY";
    }
    if (typeid(S) == typeid(double))
      this->data_format << "%float64";
    else if (typeid(S) == typeid(float))
      this->data_format << "%int32";
    else if (typeid(S) == typeid(int32_t))
      this->data_format << "%int32";
    else if (typeid(S) == typeid(int64_t))
      this->data_format << "%int64";
    else {
      std::string err_str = __func__;
      err_str += "::yaxis can be double, float, int32_t or int64_t ONLY";
    }
    this->data_format << "'";
  }

  ~gnuplotter() {
    if (this->plotHandle != nullptr) {
      if (this->outfile.empty())
        pclose(this->plotHandle);
      else
        fclose(this->plotHandle);
    }
    // do I explictly delete this
    // I get free(): double free detected in tcache 2
    // delete this->plotHandle;
  }

  void plot() {
    std::ostringstream stmp;

    fprintf(this->plotHandle, "%s", this->cmd.str().c_str());
    stmp << "plot ";
    size_t i = 0;
    for (const auto &str : this->plt) {
      if (i && (i < this->plt.size()))
        stmp << ", ";
      stmp << str;
      ++i;
    }
    if (plt.size()) {
      fprintf(this->plotHandle, "%s", stmp.str().c_str());
      fprintf(this->plotHandle, "\n");
    }
    plt.clear();
    if (this->xaxis.size() && (this->xaxis.size() == this->yaxis.size())) {
      while (!this->xaxis.empty()) {
        auto &x = this->xaxis.front();
        auto &y = this->yaxis.front();
        for (size_t i = 0; i < x.size(); ++i) {
          fwrite(&x[i], sizeof(char), sizeof(T), this->plotHandle);
          fwrite(&y[i], sizeof(char), sizeof(S), this->plotHandle);
        }
        this->xaxis.pop();
        this->yaxis.pop();
      }
    }
    fprintf(this->plotHandle, "\n");
    fflush(this->plotHandle);
    // clear commands
    this->cmd = std::ostringstream();
  }

  /*!
   * @brief set xrange - simply return if both values are 0 - that is the default
   * @param xmin
   * @param xmax
   * @return false if nothing done, else throw exception
   */
  bool set_x_range(const double &xmin, const double &xmax) {
    if ((xmin == xmax) && (xmin == 0))
      return false;

    if ((xmin > xmax) || (xmin == xmax)) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::xmin > xmax! || xmin == xmax " << xmin << ", " << xmax;
      throw std::runtime_error(err_str.str());
    }
    this->cmd << "set xrange [" << xmin << ":" << xmax << "]" << std::endl;
    return true;
  }

  /*!
   * @brief set xrange - simply return if both values are 0 - that is the default
   * @param min_max
   * @return false if nothing done, else throw exception
   */
  bool set_x_range(const std::pair<double, double> &min_max) {
    if ((min_max.first == min_max.second) && (min_max.first == 0))
      return false;

    if ((min_max.first > min_max.second) || (min_max.first == min_max.second)) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::min_max.first > min_max.second!  || min_max.first == min_max.second" << min_max.first << ", " << min_max.second;
      throw std::runtime_error(err_str.str());
    }
    this->cmd << "set xrange [" << min_max.first << ":" << min_max.second << "]" << std::endl;
    return true;
  }

  /*!
   * @brief set yrange - simply return if both values are 0 - that is the default
   * @param ymin
   * @param ymax
   * @return false if nothing done, else throw exception
   */

  bool set_y_range(const std::pair<double, double> &min_max) {
    if ((min_max.first == min_max.second) && (min_max.first == 0))
      return false;

    if ((min_max.first > min_max.second) || (min_max.first == min_max.second)) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::min_max.first > min_max.second! || min_max.first == min_max.second! " << min_max.first << ", " << min_max.second;
      throw std::runtime_error(err_str.str());
    }

    this->cmd << "set yrange [" << min_max.first << ":" << min_max.second << "]" << std::endl;
    return true;
  }

  /*!
   * @brief set yrange - simply return if both values are 0 - that is the default
   * @param ymin
   * @param ymax
   * @return false if nothing done, else throw exception
   */
  bool set_y_range(const double &ymin, const double &ymax) {
    if ((ymin == ymax) && (ymin == 0))
      return false;

    if ((ymin > ymax) || (ymin == ymax)) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::ymin > ymax! || ymin == ymax" << ymin << ", " << ymax;
      throw std::runtime_error(err_str.str());
    }

    this->cmd << "set yrange [" << ymin << ":" << ymax << "]" << std::endl;
    return true;
  }

  std::string default_color(const std::string &channel_type, const std::string formats = "") const {
    std::string color;
    if (channel_type == "Ex") {
      color = "lc rgbcolor \"yellow\"";
    }
    if (channel_type == "Ey") {
      color = "lc rgbcolor \"orange\"";
    }
    if (channel_type == "Ez") {
      color = "lc rgbcolor \"black\"";
    }
    if (channel_type == "Hx") {
      color = "lc rgbcolor \"blue\"";
    }
    if (channel_type == "Hy") {
      color = "lc rgbcolor \"red\"";
    }
    if (channel_type == "Hz") {
      color = "lc rgbcolor \"green\"";
    }

    return color + " " + formats;
  }

  std::string default_color(const std::pair<std::string, std::string> spectra_type, const std::string formats = "") const {
    std::string color;
    if (spectra_type == std::make_pair<std::string, std::string>("Ex", "Ex")) {
      color = "lc rgbcolor \"yellow\"";
    } else if (spectra_type == std::make_pair<std::string, std::string>("Ey", "Ey")) {
      color = "lc rgbcolor \"orange\"";
    } else if (spectra_type == std::make_pair<std::string, std::string>("Ez", "Ez")) {
      color = "lc rgbcolor \"black\"";
    } else if (spectra_type == std::make_pair<std::string, std::string>("Hx", "Hx")) {
      color = "lc rgbcolor \"blue\"";
    } else if (spectra_type == std::make_pair<std::string, std::string>("Hy", "Hy")) {
      color = "lc rgbcolor \"red\"";
    } else if (spectra_type == std::make_pair<std::string, std::string>("Hz", "Hz")) {
      color = "lc rgbcolor \"green\"";
    }

    if (color.size())
      return color + " " + formats;

    std::vector<std::string> ac = {"Hx", "Hy", "Hz", "Ex", "Ey", "Ez"};
    // colour vectors must be longer than ac!
    std::vector<std::string> colors_Hx = {"web-blue", "royalblue", "steelblue", "lightblue", "darkblue", "midnightblue", "mediumblue", "skyblue", "slateblue1"};
    std::vector<std::string> colors_Hy = {"web-red", "firebrick", "darkred", "indianred", "maroon", "darkmagenta", "darkviolet", "purple"};
    std::vector<std::string> colors_Hz = {"web-green", "darkgreen", "forestgreen", "limegreen", "mediumseagreen", "seagreen", "springgreen"};
    std::vector<std::string> colors_Ex = {"web-yellow", "khaki", "lightgoldenrod", "lightgoldenrod1", "lightgoldenrod2", "lightgoldenrod3", "lightgoldenrod4", "lightyellow", "lightyellow1", "lightyellow2", "lightyellow3", "lightyellow4"};
    std::vector<std::string> colors_Ey = {"web-orange", "darkorange", "orangered", "tomato", "coral", "darkgoldenrod", "goldenrod", "goldenrod1", "goldenrod2", "goldenrod3", "goldenrod4"};
    std::vector<std::string> colors_Ez = {"web-black", "dimgray", "gray", "darkgray", "lightgray", "gray0", "gray1", "gray2", "gray3", "gray4", "gray5", "gray6", "gray7", "gray8", "gray9", "gray10", "gray11", "gray12", "gray13", "gray14", "gray15", "gray16", "gray17", "gray18", "gray19", "gray20", "gray21", "gray22", "gray23", "gray24", "gray25", "gray26", "gray27", "gray28", "gray29", "gray30", "gray31", "gray32", "gray33", "gray34", "gray35", "gray36", "gray37", "gray38", "gray39", "gray40", "gray41", "gray42", "gray43", "gray44", "gray45", "gray46", "gray47", "gray48", "gray49", "gray50", "gray51", "gray52", "gray53", "gray54", "gray55", "gray56", "gray57", "gray58", "gray59", "gray60", "gray61", "gray62", "gray63", "gray64", "gray65", "gray66", "gray67", "gray68", "gray69", "gray70", "gray71", "gray72", "gray73", "gray74", "gray75", "gray76", "gray77", "gray78", "gray79", "gray80", "gray81", "gray82", "gray83", "gray84", "gray85", "gray86", "gray87", "gray88", "gray89", "gray90", "gray91", "gray92", "gray93", "gray94", "gray95", "gray96", "gray97", "gray98", "gray99"};

    size_t index = (std::find(ac.begin(), ac.end(), spectra_type.second) - ac.begin());
    if (spectra_type.first == "Hx") {
      if (index < colors_Hx.size()) {
        color = "lc rgbcolor \"" + colors_Hx[index] + "\"";
      } else {
        color = "lc rgbcolor \"grey\"";
      }
    } else if (spectra_type.first == "Hy") {
      if (index < colors_Hy.size()) {
        color = "lc rgbcolor \"" + colors_Hy[index] + "\"";
      } else {
        color = "lc rgbcolor \"grey\"";
      }
    } else if (spectra_type.first == "Hz") {
      if (index < colors_Hz.size()) {
        color = "lc rgbcolor \"" + colors_Hz[index] + "\"";
      } else {
        color = "lc rgbcolor \"grey\"";
      }
    } else if (spectra_type.first == "Ex") {
      if (index < colors_Ex.size()) {
        color = "lc rgbcolor \"" + colors_Ex[index] + "\"";
      } else {
        color = "lc rgbcolor \"grey\"";
      }
    } else if (spectra_type.first == "Ey") {
      if (index < colors_Ey.size()) {
        color = "lc rgbcolor \"" + colors_Ey[index] + "\"";
      } else {
        color = "lc rgbcolor \"grey\"";
      }
    } else if (spectra_type.first == "Ez") {
      if (index < colors_Ez.size()) {
        color = "lc rgbcolor \"" + colors_Ez[index] + "\"";
      } else {
        color = "lc rgbcolor \"grey\"";
      }
    }

    return color + " " + formats;
  }

  void set_qt_terminal(const std::string &title = "", const size_t &file_count = 0) {
    this->cmd << "set terminal qt size 1024,768 enhanced font 'Noto Sans, 10'";
    if (title.size()) {
      this->cmd << " title '" << title;
      if (file_count)
        this->cmd << "_" << file_count;
      else
        this->cmd << "'";
    }
    this->cmd << std::endl;
  }

  void set_svg_terminal(const std::filesystem::path &path, const std::filesystem::path &file_name, const std::string &title = "", const size_t &max_sizes = 1, const size_t &file_count = 0) {
    std::ostringstream nam;
    this->cmd << "set terminal svg size 1024,768 enhanced dynamic font 'Noto Sans, 14'" << std::endl;
    if (title.size()) {
      this->cmd << "set title '" << title;
      if (file_count)
        this->cmd << "_" << file_count;
      else
        this->cmd << "'" << std::endl;
    }
    if (!file_count)
      nam << "set output '" << (path / file_name).string() << ".svg'";
    else
      nam << "set output '" << (path / file_name).string() << "_" << file_count << ".svg'";
    this->cmd << nam.str() << std::endl;

    this->max_sizes = max_sizes; // take 1 for svg - lines and symbol thickness are bad
  }

  void set_pdf_terminal(const std::filesystem::path &path, const std::filesystem::path &file_name, const std::string &title = "", const size_t &max_sizes = 1, const size_t &file_count = 0) {
    std::ostringstream nam;

    this->cmd << "set terminal pdf size 29.00cm,21.00cm enhanced font 'Noto Sans, 14'" << std::endl;
    if (title.size()) {
      this->cmd << "set title '" << title;
      if (file_count)
        this->cmd << "_" << file_count;
      else
        this->cmd << "'" << std::endl;
    }
    this->cmd << std::endl;

    if (!file_count)
      nam << "set output '" << (path / file_name).string() << ".pdf'";
    else
      nam << "set output '" << (path / file_name).string() << "_" << file_count << ".pdf'";
    this->cmd << nam.str() << std::endl;

    this->max_sizes = max_sizes; // take 1 for svg - lines and symbol thickness are bad
  }

  void set_xy_points(const std::vector<T> &x, const std::vector<S> &y, const std::string title, const uint64_t ps, const std::string formats = "") {

    this->set_data(x, y);
    std::ostringstream tmp;
    tmp << "'-' binary record=(" << x.size() << ") " << this->data_format.str() << " with points ps " << ps;
    if (formats.size())
      tmp << " " << formats;
    if (title.size())
      tmp << " title '" << title << "'";
    this->plt.push_back(this->trim(tmp.str()));
  }

  void set_xy_lines(const std::vector<T> &x, const std::vector<S> &y, const std::string title, const uint64_t lw, const std::string formats = "") {

    this->set_data(x, y);
    std::ostringstream tmp;
    tmp << "'-' binary record=(" << x.size() << ") " << this->data_format.str() << " with lines" << this->set_lw(lw);
    if (formats.size())
      tmp << " " << formats;
    if (title.size())
      tmp << " title '" << title << "'";
    this->plt.push_back(this->trim(tmp.str()));
  }

  void set_xy_linespoints(const std::vector<T> &x, const std::vector<S> &y, const std::string &title, const uint64_t &lw, const uint64_t &ps, const std::string &formats = "") {

    this->set_data(x, y);
    std::ostringstream tmp;
    tmp << "'-' binary record=(" << x.size() << ") " << this->data_format.str() << " with linespoints" << this->set_lw(lw) << this->set_ps(ps);
    if (formats.size())
      tmp << " " << formats;
    if (title.size())
      tmp << " title '" << title << "'";
    this->plt.push_back(this->trim(tmp.str()));
  }

  std::ostringstream cmd;
  std::vector<std::string> plt;

  std::ostringstream data_format;

  std::queue<std::vector<T>> xaxis;
  std::queue<std::vector<S>> yaxis;

private:
  FILE *plotHandle = nullptr;
  std::filesystem::path outfile;

  uint64_t max_sizes = 100;

  std::string set_lw(const uint64_t &lw) {
    std::ostringstream tmp;
    if (lw > this->max_sizes)
      tmp << " lw " << max_sizes;
    else
      tmp << " lw " << lw;
    return tmp.str();
  }

  std::string set_ps(const uint64_t &ps) {
    std::ostringstream tmp;
    if (ps > this->max_sizes)
      tmp << " ps " << max_sizes;
    else
      tmp << " ps " << ps;
    return tmp.str();
  }

  void set_data(const std::vector<T> &x, const std::vector<S> &y) {

    if (x.size() != y.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::vectors have different sizes! " << x.size() << ", " << y.size();
      throw std::runtime_error(err_str.str());
    }
    this->xaxis.push(std::vector<T>(x));
    this->yaxis.push(std::vector<S>(y));
  }

  void ascci_blocks() {
    if (this->xaxis.size() && (this->xaxis.size() == this->yaxis.size())) {
      while (!this->xaxis.empty()) {
        this->data.clear();
        this->data.str().clear();
        this->data << std::scientific;
        auto &x = this->xaxis.front();
        auto &y = this->yaxis.front();

        for (size_t i = 0; i < x.size(); ++i) {
          this->data << x[i] << " " << y[i] << std::endl;
        }
        this->data << "e" << std::endl;
        if (this->data.tellp() > 1) {
          fprintf(this->plotHandle, "%s", this->data.str().c_str());
        }
        this->xaxis.pop();
        this->yaxis.pop();
      }
    }
    this->data.clear();
    this->data.str().clear();
  }

  // copy from my library - this plotter may wants to be copied without dependencies

  /*!
   * \brief rtrim remove trailing empty spaces from a string
   * \param s
   * \return
   */
  std::string rtrim(const std::string &s) {
    std::string ws(" \t\f\v\n\r");
    auto found = s.find_last_not_of(ws);
    if (found != std::string::npos) {
      return s.substr(0, found + 1);
    }
    return s;
  }

  /*!
   * \brief ltrim remove leading empty spaces from a string
   * \param s
   * \return
   */
  std::string ltrim(const std::string &s) {
    std::string ws(" \t\f\v\n\r");
    auto found = s.find_first_not_of(ws);
    if (found != std::string::npos) {
      return s.substr(found);
    }
    return std::string();
  }

  /*!
   * \brief trim trims a string on both ends
   * \param s
   * \return
   */
  std::string trim(const std::string &s) {
    std::string str(rtrim(s));
    return ltrim(str);
  }
};

/*
    std::string init_err_mm;
    auto gplt_mm = std::make_unique<gnuplotter<double, double>>(init_err_mm);

    if (init_err_mm.size()) {
        std::cout << init_err_mm << std::endl;
        return EXIT_FAILURE;
    }


    gplt_mm->cmd << "set terminal qt size 2048,1600" << std::endl;

//    gplt_mm->cmd << "set terminal svg size 2048,1600" << std::endl;
//    gplt_mm->cmd << "set output '/tmp/spectra.svg'" << std::endl;
    gplt_mm->cmd << "set multiplot layout 2, 1" << std::endl;
    gplt_mm->cmd << "set title 'TS Data'" << std::endl;
    //gplt->cmd << "set key off" << std::endl;
    gplt_mm->cmd << "set xlabel 'ts'" << std::endl;
    gplt_mm->cmd << "set ylabel 'amplitude [mV]'" << std::endl;
    gplt_mm->cmd << "set grid" << std::endl;
    gplt_mm->cmd << "set key font \"Hack, 10\"" << std::endl;
    //gplt_mm->cmd << "plot sin(x)/x" << std::endl;
    gplt_mm->set_x_range(ts_beg, ts_end);
    gplt_mm->set_xy_lines(xax, yax, "input", 1);

    gplt_mm->plot();

    //gplt_mm->cmd << std::endl;
    gplt_mm->cmd << "set title 'TS 2x Data'" << std::endl;
    //gplt->cmd << "set key off" << std::endl;
    gplt_mm->cmd << "set xlabel 'ts'" << std::endl;
    gplt_mm->cmd << "set ylabel 'amplitude [mV]'" << std::endl;
    gplt_mm->cmd << "set grid" << std::endl;
    gplt_mm->cmd << "set key font \"Hack, 10\"" << std::endl;

    gplt_mm->set_x_range(ts_beg, ts_end);
    for(auto &y : yax) y *= 10.0;
    gplt_mm->set_xy_lines(xax, yax, "input", 1);
    //gplt_mm->cmd << "plot sin(x)" << std::endl;

    gplt_mm->plot();


*/

#endif
