#ifndef CHANNEL_COLLECTOR_H
#define CHANNEL_COLLECTOR_H

#include <list>
#include <string>
#include <vector>

#include "atmm.h"
#include "atss.h"
#include "freqs.h"
#include "gnuplotter.h"
//
#include "raw_spectra.h"
//

class channel_collector_gplt {
public:
  /*!
   * @brief constructor for a channel plotter for x y plots
   * @param gplt
   * @param channel_type
   * @param use_selected
   */
  channel_collector_gplt(std::shared_ptr<gnuplotter<double, double>> gplt, std::string channel_type, bool use_selected = true)
      : gplt(gplt), channel_type(channel_type), use_selected(use_selected) {
    if (channel_type == "Ex") {
      this->color = "lc rgbcolor \"yellow\"";
    } else if (channel_type == "Ey") {
      color = "lc rgbcolor \"orange\"";
    } else if (channel_type == "Ez") {
      color = "lc rgbcolor \"black\"";
    } else if (channel_type == "Hx") {
      color = "lc rgbcolor \"blue\"";
    } else if (channel_type == "Hy") {
      color = "lc rgbcolor \"red\"";
    } else if (channel_type == "Hz") {
      color = "lc rgbcolor \"green\"";
    } else {
      color = "lc rgbcolor \"grey\"";
    }
  }
  /*!
   * @brief constructor for a channel plotter for x y SPECTRA plots
   * @param gplt
   * @param spectra_type
   * @param use_selected
   */
  channel_collector_gplt(std::shared_ptr<gnuplotter<double, double>> gplt, std::pair<std::string, std::string> spectra_type, bool use_selected = true)
      : gplt(gplt), spectra_type(spectra_type), use_selected(use_selected) {
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
  }

  void collect(std::shared_ptr<channel> in_chan, std::shared_ptr<fftw_freqs> in_freqs, std::shared_ptr<raw_spectra> in_raw_spectra) {
    if (in_chan->channel_type != channel_type) {
      return;
    }
    // if chan sample rate is not in f_collected
    if (std::find(f_collected.begin(), f_collected.end(), in_chan->get_sample_rate()) == f_collected.end()) {

      // copy the shared pointers
      this->chan = in_chan;
      this->freqs = in_freqs;
      this->raw_spectra = in_raw_spectra;
      // create the label (don't care about calling it again, cost nothing)
      this->label = chan->cal->sensor + " " + chan->cal->serial2string();

      f_collected.push_back(chan->get_sample_rate());
      // add freqs to freqs; append the freqs to the end of the vector
      // use freqs->get_selected_frequencies() if use_selected is true
      this->get();
    } else {
      // make the plot - new data is already there in case
      this->plot();
      // copy the shared pointers
      this->chan = in_chan;
      this->freqs = in_freqs;
      this->raw_spectra = in_raw_spectra;
      f_collected.push_back(chan->get_sample_rate());
      this->label = chan->cal->sensor + " " + chan->cal->serial2string();
      this->get();
    }
  }

  // void collect_runs(std::shared_ptr<run_d> run) {
  //   // get the channels numbers
  //   auto ch_nums = run->get_channel_with_spectra(use_selected);

  //   // loop over the channels
  //   for (auto i = 0; i < ch_nums.size(); ++i) {
  //     // get the channel
  //     auto chan = run->channels[ch_nums[i]];
  //     // get the freqs
  //     auto freqs = chan->fft_freqs;
  //     // get the raw_spectra
  //     auto raw_spectra = run->raw_spc;
  //     // collect the data
  //     this->collect(chan, freqs, raw_spectra);
  //   }
  // }

  void plot() {
    if (this->f_l.size() == 0) {
      return;
    }

    // get the data from the lists
    // order by ascending sample rate according to f_collected
    auto sorted = this->f_collected;
    // sort sorted
    std::sort(sorted.begin(), sorted.end());

    // append to f and data in sorted order; use sorted to get the index
    for (auto i = 0; i < sorted.size(); ++i) {
      auto index = std::find(this->f_collected.begin(), this->f_collected.end(), sorted[i]) - this->f_collected.begin();
      // append the data
      this->f.insert(this->f.end(), this->f_l[index].begin(), this->f_l[index].end());
      this->data.insert(this->data.end(), this->data_l[index].begin(), this->data_l[index].end());
    }

    this->gplt->set_xy_linespoints(this->f, this->data, this->label, 2, 2, this->gplt->default_color(chan->channel_type, this->pt_types_gnuplot[this->pt_types_gnuplot_index]));

    this->f_l.clear();
    this->data_l.clear();

    this->data.clear();
    this->f.clear();
    f_collected.clear();
    // reset the shared pointers
    this->chan.reset();
    this->freqs.reset();
    this->raw_spectra.reset();
    if (pt_types_gnuplot_index < pt_types_gnuplot.size() - 1) {
      ++pt_types_gnuplot_index;
    } else {
      pt_types_gnuplot_index = 0;
    }
  }

private:
  std::string channel_type;
  std::pair<std::string, std::string> spectra_type;
  std::string color;
  std::vector<std::vector<double>> f_l;
  std::vector<std::vector<double>> data_l;
  std::vector<double> f;
  std::vector<double> data;
  // we need to store the shared pointers in order to call plot when the main loop is done
  std::shared_ptr<channel> chan;
  std::shared_ptr<fftw_freqs> freqs;
  std::shared_ptr<raw_spectra> raw_spectra;
  std::string label;
  std::vector<std::string> pt_types_gnuplot = {"pt 6", "pt 4", "pt 8", "pt 10", "pt 12", "pt 14", "pt 1", "pt 2", "pt 3"};
  size_t pt_types_gnuplot_index = 0;
  std::vector<double> f_collected;
  bool use_selected = true;
  // add gnuplotter double double template
  std::shared_ptr<gnuplotter<double, double>> gplt;

  void get() {
    if (use_selected) {
      this->f_l.push_back(this->freqs->get_selected_frequencies());
    } else {
      this->f_l.push_back(this->freqs->get_frequencies());
    }
    // use raw_spectra->get_abs_sa_prz_spectra() if use_selected is true
    if (use_selected) {
      this->data_l.push_back(this->raw_spectra->get_abs_sa_prz_spectra(this->spectra_type));
    } else {
      this->data_l.push_back(this->raw_spectra->get_abs_sa_spectra(this->spectra_type));
    }
  }
};
#endif // CHANNEL_COLLECTOR_H
