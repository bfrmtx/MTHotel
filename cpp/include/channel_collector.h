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
#include "../mt/raw_spectra/raw_spectra.h"
//

class channel_collector_gplt {
public:
  channel_collector_gplt(std::shared_ptr<gnuplotter<double, double>> gplt, std::string channel_type, bool use_selected = true) :
      gplt(gplt),
      channel_type(channel_type), use_selected(use_selected) {
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

  void collect_runs(std::shared_ptr<run_d> run) {
    // get the channels numbers
    auto ch_nums = run->get_channel_with_spectra(use_selected);

    // loop over the channels
    for (auto i = 0; i < ch_nums.size(); ++i) {
      // get the channel
      auto chan = run->channels[ch_nums[i]];
      // get the freqs
      auto freqs = chan->fft_freqs;
      // get the raw_spectra
      auto raw_spectra = run->raw_spc;
      // collect the data
      this->collect(chan, freqs, raw_spectra);
    }
  }

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
      this->data_l.push_back(this->raw_spectra->get_abs_sa_prz_spectra(this->channel_type));
    } else {
      this->data_l.push_back(this->raw_spectra->get_abs_sa_spectra(this->channel_type));
    }
  }
};
#endif // CHANNEL_COLLECTOR_H
