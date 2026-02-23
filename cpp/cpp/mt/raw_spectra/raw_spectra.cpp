#include "raw_spectra.hpp"

void raw_spectra::move_raw_spectra(std::shared_ptr<channel> chan) {

  this->move_spectra(chan);
  this->channels.emplace_back(std::make_shared<channel>(chan)); // don't copy if you want free memory later
  // we assume that the fft_freqs are the same for all channels added to the raw_spectra
  if (this->fft_freqs == nullptr) {
    this->fft_freqs = std::make_shared<fftw_freqs>(chan->fft_freqs);
  }
}

void raw_spectra::advanced_stack_all(const double &fraction_to_use) {
  if (this->size() == 0) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no spectra for stacking available";
    throw std::runtime_error(err_str.str());
  }
  std::cerr << "pushing tasks to thread pool" << std::endl;

  for (auto &ac : this->sa) {
    if (fraction_to_use < 0.0 || fraction_to_use > 1.00001) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::fraction_to_use must be between 0.01 and 1.0";
      throw std::runtime_error(err_str.str());
    }
    if (is_auto_spectra(ac.first)) {
      this->pool->detach_task([this, chan_pair = ac.first, fraction_to_use]() {
        std::cerr << "stacking auto spectra for " << chan_pair.first->channel_type << " " << chan_pair.second->channel_type << std::endl;
        this->do_advanced_stack_auto(chan_pair, fraction_to_use);
      });
    } else {
      this->pool->detach_task([this, chan_pair = ac.first, fraction_to_use]() {
        this->do_advanced_stack_cross(chan_pair, fraction_to_use);
      });
    }
  }
}

void raw_spectra::parzen_stack_all() {
  // no previously stacked spectra
  if (this->sa.size() == 0) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no stack all spectra for parzening available";
    throw std::runtime_error(err_str.str());
  }

  std::cerr << "pushing tasks to thread pool (to be implemented)" << std::endl;

  for (auto &ac : this->sa_prz) {
    // get the key from sa_prz, which is the same as in sa
    auto result = this->sa_prz.get_spectra(ac.first); // put the result into sa_prz, empty at this point
    for (const auto &res : *result)
      std::cout << res << " ";
    std::cout << std::endl;
    // task to push is parzen_t<double, double> with the parameters ac ...
    // from prz_vecor.h:
    // we call void parzen_t(const std::shared_ptr<std::vector<T>> &data, const std::vector<S> &selected_freqs, const std::vector<std::vector<S>> &parzendists, std::shared_ptr<std::vector<T>> &result)
    // parzen(*data, selected_freqs, parzendists, *result);
    // this->pool->push_task(parzen_t<double, double>, std::ref(this->sa[ac.first]), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(result));
    parzen_t<double, double>(this->sa[ac.first], this->fft_freqs->selected_freqs, this->fft_freqs->parzendists, result);
    for (const auto &res : *result)
      std::cout << res << " ";
    std::cout << std::endl;
  }
}

void raw_spectra::smooth_stack_all() {
  // no previously stacked spectra
  if (this->sa.size() == 0) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no stack all spectra for smoothing available";
    throw std::runtime_error(err_str.str());
  }

  std::cerr << "pushing tasks to thread pool to be implemented" << std::endl;

  for (auto &ac : this->sa_avg) {
    // get the key from sa_avg, which is the same as in sa
    auto result = this->sa_avg.get_spectra(ac.first); // put the result into sa_avg
    // task to push is smooth_t<double, double> with the parameters ac ...
    // we call void smooth_t(const std::shared_ptr<std::vector<T>> &data, const std::vector<S> &selected_freqs, const size_t &nlines_avg, std::shared_ptr<std::vector<T>> &result)
    // smooth(*data, selected_freqs, nlines_avg, *result);
    // this->pool->push_task(smooth_t<double, double>, std::ref(this->sa[ac.first]), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->nlines_avg), std::ref(result));
    nsmooth_t<double>(this->sa[ac.first], this->fft_freqs->smooth_index, this->fft_freqs->nlines_avg, result);
  }
}

std::pair<double, double> raw_spectra::get_abs_sa_spectra_min_max(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
  std::pair<double, double> result(DBL_MIN, DBL_MAX);
  auto v = this->get_abs_sa_spectra(chan_pair);
  auto xmm = std::minmax_element(v.cbegin(), v.cend());
  result.first = *xmm.first;
  result.second = *xmm.second;
  return result;
}

std::string
raw_spectra::get_sensor_name(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
  auto name1 = chan_pair.first->channel_type;
  auto name2 = (chan_pair.second != nullptr) ? chan_pair.second->channel_type : std::string();
  auto result1 = name1;
  auto result2 = name2;
  // add is_remote and is_emap
  for (const auto &c : this->channels) {
    if (c->channel_type == name1) {
      result1 = name1 + " " + c->cal->sensor;
    }
  }
  if (name1 == name2) {
    return result1;
  }
  for (const auto &c : this->channels) {
    if (c->channel_type == name2) {
      result2 = name2 + " " + c->cal->sensor;
    }
  }
  return name1 + ", " + name2;
}

std::string raw_spectra::get_sensor_serial(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
  auto name1 = chan_pair.first->channel_type;
  auto name2 = (chan_pair.second != nullptr) ? chan_pair.second->channel_type : std::string();
  auto result1 = name1;
  auto result2 = name2;
  // add is_remote and is_emap
  for (const auto &c : this->channels) {
    if (c->channel_type == name1) {
      result1 = c->cal->serial2string();
    }
  }
  if (name1 == name2) {
    return name1;
  }
  for (const auto &c : this->channels) {
    if (c->channel_type == name2) {
      result2 = c->cal->serial2string();
    }
  }
  return result1 + ", " + result2;
}

std::string raw_spectra::get_sensor_name_serial(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair, const bool cat_underscore) const {
  auto name1 = chan_pair.first->channel_type;
  auto name2 = (chan_pair.second != nullptr) ? chan_pair.second->channel_type : std::string();
  auto result1 = name1;
  auto result2 = name2;
  std::string serial1;
  std::string serial2;
  // add is_remote and is_emap
  for (const auto &c : this->channels) {
    if (c->channel_type == name1) {
      result1 = c->cal->sensor;
      serial1 = c->cal->serial2string();
    }
  }
  if (name1 == name2) {
    if (cat_underscore)
      return result1 + "_" + serial1;
    return result1 + " " + serial1;
  }
  for (const auto &c : this->channels) {
    if (c->channel_type == name2) {
      result2 = c->cal->sensor;
      serial2 = c->cal->serial2string();
    }
  }
  if (cat_underscore)
    return result1 + "_" + serial1 + "__" + result2 + "_" + serial2;

  return result1 + " " + serial1 + ", " + result2 + " " + serial2;
}

std::string raw_spectra::get_sampling_rate(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
  for (const auto &c : this->channels) {
    if (c->channel_type == chan_pair.first->channel_type) {
      return mstr::sample_rate_to_str_simple(c->get_sample_rate());
    }
  }
  return std::string();
}

void raw_spectra::multiply_sa_spectra(const double &factor) {
  // multiply all spectra in sa by factor directly
  for (auto &ac : this->sa) {
    std::transform(ac.second->begin(), ac.second->end(), ac.second->begin(), [factor](double &d) { return d * factor; });
  }
}

std::vector<double> raw_spectra::get_spectra_generic(const spc_base<double> &data_container, const std::function<std::vector<double>()> &get_freqs, const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
  if (data_container.size() == 0) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no spectra found!";
    throw std::runtime_error(err_str.str());
  }
  return data_container.get_spectra_vec(chan_pair);
}

void raw_spectra::dump_spectra_generic(const spc_base<double> &data_container,
                                       const std::string &spectra_type,
                                       const std::function<std::vector<double>()> &get_freqs) const {
  // Check if the data_container is empty, if so return
  if (data_container.size() == 0) {
    return;
  }
  // Create dump directory if needed
  fs::path home_dir_dump(getenv("HOME"));
  home_dir_dump /= "dump_spectra";
  try {
    if (!std::filesystem::exists(home_dir_dump)) {
      std::filesystem::create_directory(home_dir_dump);
    }
  } catch (const std::filesystem::filesystem_error &e) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::failed to create dump directory " << home_dir_dump << ": " << e.what();
    throw std::runtime_error(err_str.str());
  }

  // Dump all spectra in the container to files
  for (const auto &ac : data_container) {
    auto chan1_type = ac.first.first != nullptr ? ac.first.first->channel_type : std::string();
    auto chan2_type = ac.first.second != nullptr ? ac.first.second->channel_type : std::string();
    std::string spc_name = chan1_type + "_" + chan2_type;
    std::string sensor1_and_2 = this->get_sensor_name_serial(ac.first, true);

    std::string sampling_rate = this->get_sampling_rate(ac.first);
    fs::path filename = home_dir_dump / (spc_name + "_" + sensor1_and_2 + "_" + sampling_rate + spectra_type + ".dat");

    std::ofstream out(filename);
    if (!out) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::cannot open file " << filename;
      throw std::runtime_error(err_str.str());
    }

    auto f = get_freqs();
    size_t i = 0;
    for (const auto &d : *ac.second) {
      out << f[i++] << " " << d << std::endl;
    }
    out.close();
  }
  // now we save also save the channel(s) as json
  for (const auto &chan : this->channels) {
    fs::path filename = home_dir_dump / (chan->channel_type + "_" + chan->cal->serial2string() + "_channel.json");
    chan->save(filename);
  }
  // now we save also the fft_freqs as json, we can only ONE fft_freqs in raw_spectra
  if (this->fft_freqs != nullptr) {
    fs::path filename = home_dir_dump / ("fft_freqs.json");
    this->fft_freqs->save(filename);
  }
}
void raw_spectra::load_from_rundir_result(const std::string &which_spectra_type, const std::filesystem::path &top_dir) {
  std::string expected_extension = ".datfa";
  std::shared_ptr<fftw_freqs> freqs_read;
  if (which_spectra_type == "sa") {
    freqs_read = this->sa.load_from_rundir(top_dir, expected_extension);
  } else if (which_spectra_type == "sa_prz") {
    freqs_read = this->sa_prz.load_from_rundir(top_dir, expected_extension);
  } else if (which_spectra_type == "sa_avg") {
    freqs_read = this->sa_avg.load_from_rundir(top_dir, expected_extension);
  } else if (which_spectra_type == "noise") {
    freqs_read = this->noise.load_from_rundir(top_dir, expected_extension);
  } else if (which_spectra_type == "noise_prz") {
    freqs_read = this->noise_prz.load_from_rundir(top_dir, expected_extension);
  } else {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::unknown spectra type: " << which_spectra_type;
    throw std::runtime_error(err_str.str());
  }
  if (this->fft_freqs == nullptr) {
    this->fft_freqs = std::make_shared<fftw_freqs>(freqs_read);
  } else {
    if (this->fft_freqs->get_frequencies() != freqs_read->get_frequencies()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::fft frequencies read from " << top_dir << " do not match existing fft frequencies in raw_spectra";
      throw std::runtime_error(err_str.str());
    }
  }
}

void raw_spectra::do_advanced_stack_auto(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair, const double &fraction_to_use) {
  // chan_pair should be (channel, channel) where both point to the same channel for auto spectra
  // But wait - we need to get the<X, null> spectra from the raw_spectra first, not the auto spectra
  // The raw data is stored with nullptr as second channel

  std::shared_ptr<std::vector<std::vector<std::complex<double>>>> in;
  std::shared_ptr<std::vector<double>> out;
  size_t n;

  try {
    auto single_chan = std::make_pair(chan_pair.first, nullptr);
    in = this->get_spectra(single_chan);   // Get the raw spectra <X, null>, complex spectra, vector of vectors from raw_spectra
    n = in->at(0).size();                  // f size
    out = this->sa.get_spectra(chan_pair); // Get or create output auto spectra <X, X>
  } catch (const std::exception &e) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::error retrieving spectra for auto stacking: " << e.what();
    throw std::runtime_error(err_str.str());
  }

  bool adv = (fraction_to_use < 0.99999); // adv is false if fraction_to_use is 1.0
  out->resize(n, 0.0);
  for (size_t i = 0; i < n; ++i) {                  //   for frequencies
    auto ff = bvec::absv(bvec::get_fslice(*in, i)); // get all stacks for f
    if (!adv) {
      out->at(i) = bvec::mean(ff);
    } else {
      out->at(i) = bvec::median_range_mean(ff, fraction_to_use); // two_pass_variance var, var.variance(ff.cbegin(), ff.cend());
    }
  }
  // out is a shared pointer to a vector of doubles, we don't need to return it or copy it
}

void raw_spectra::do_advanced_stack_cross(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair, const double &fraction_to_use) {
  // chan_pair is like <X, Y> (two different channels)
  // Get the raw spectra <X, null> and <Y, null>
  std::shared_ptr<std::vector<std::vector<std::complex<double>>>> in1;
  std::shared_ptr<std::vector<std::vector<std::complex<double>>>> in2;
  std::shared_ptr<std::vector<double>> out;
  size_t n;
  bool adv;

  try {
    auto single_chan1 = std::make_pair(chan_pair.first, nullptr);
    auto single_chan2 = std::make_pair(chan_pair.second, nullptr);
    in1 = this->get_spectra(single_chan1); // complex spectra, vector of vectors like <Hx, null> - single spectra
    in2 = this->get_spectra(single_chan2); // complex spectra, vector of vectors like <Hy, null> - single spectra
    out = this->sa.get_spectra(chan_pair); // double spectra vector, we reserved the space in advance like <Hx, Hy> - cross spectra
    n = in1->at(0).size();                 // f size
    adv = (fraction_to_use < 1.0);
    out->resize(n, 0.0); // stack size
  } catch (const std::exception &e) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::error retrieving spectra for cross stacking: " << e.what();
    throw std::runtime_error(err_str.str());
  }

  for (size_t i = 0; i < n; ++i) {        // for all f
    auto ff1 = bvec::get_fslice(*in1, i); // get all stacks for f
    auto ff2 = bvec::get_fslice(*in2, i); // get all stacks for f
    // if the above would be a matrix we would transpose it, so take the rows instead of columns
    std::vector<double> ff = bvec::make_cross_sqrt_conj_abs(ff1, ff2);
    if (!adv) {
      out->at(i) = bvec::mean(ff);
    } else {
      out->at(i) = bvec::median_range_mean(ff, fraction_to_use);
    }
  }
}

void raw_spectra::do_cross_coherence_raw_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair, const size_t &n_f_smooth) {
  // chan_pair is like <Hx, Hy> - a cross spectrum pair
  // Get the raw spectra <Hx, null> and <Hy, null>
  std::shared_ptr<std::vector<std::vector<std::complex<double>>>> in1;
  std::shared_ptr<std::vector<std::vector<std::complex<double>>>> in2;
  std::shared_ptr<std::vector<double>> out;
  size_t n;

  try {
    auto single_chan1 = std::make_pair(chan_pair.first, nullptr);
    auto single_chan2 = std::make_pair(chan_pair.second, nullptr);
    in1 = this->get_spectra(single_chan1);  // complex spectra, vector of vectors like <Hx, null> - single spectra
    in2 = this->get_spectra(single_chan2);  // complex spectra, vector of vectors like <Hy, null> - single spectra
    out = this->coh.get_spectra(chan_pair); // double spectra vector, we reserved the space in advance
    n = in1->at(0).size();                  // f size
  } catch (const std::exception &e) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::error retrieving spectra for coherence calculation: " << e.what();
    throw std::runtime_error(err_str.str());
  }
  //
  // *out = bvec::coherence_raw_spectra_smooth<double, std::complex<double>>(*in1, *in2, 4);
  // return;
  // new style
  out->resize(n, 0.0);                    // f size
  for (size_t i = 0; i < n; ++i) {        // for all f
    auto ff1 = bvec::get_fslice(*in1, i); // get all stacks for f
    auto ff2 = bvec::get_fslice(*in2, i); // get all stacks for f
    out->at(i) = bvec::coherence<std::complex<double>>(ff1, ff2);
  }
  // out is a shared pointer to a vector of doubles, we don't need to return it or copy it
  if (n_f_smooth > 0) {
    // smooth the coherence spectra
    auto smoothed = bvec::simple_smoothing(*out, n_f_smooth);
    out->clear();
    out->insert(out->end(), smoothed.cbegin(), smoothed.cend());
  }
  // std::cout << name.first << " " << name.second << std::endl;
  // if (this->sa.empty()) {
  //   std::ostringstream err_str(__func__, std::ios_base::ate);
  //   err_str << "::no stacked spectra found for " << name.first << ", " << name.second;
  //   throw std::runtime_error(err_str.str());
  // }
  // this->sa.info();

  // now we can calculate the noise spectra noise = SpectraAmpl * sqrt (1 - coherency)
  // noise has been prepared before, so we can use it; it double
  auto out2 = this->noise.get_spectra(chan_pair);
  // we have the stacked spectra, get
  auto sa_spectra = this->sa.get_spectra(chan_pair);

  out2->resize(n, 0.0);                                                    // f size
  for (size_t i = 0; i < n; ++i) {                                         // for all f
    out2->at(i) = std::abs((*sa_spectra)[i]) * std::sqrt(1.0 - (*out)[i]); // noise = SpectraAmpl * sqrt (1 - coherency)
  }
}

void raw_spectra::coherence_raw_spectra(const size_t &n_f_smooth) {
  // so here we are in the raw_spectra class, contain all <Hx, > <Hy, > <Hz, > <Ex, > <Ey, >  and so on spectra
  // we want to calculate the coherency between all of them
  if (this->size() == 0) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no spectra for coherency calculation available";
    throw std::runtime_error(err_str.str());
  }
  if (this->size() == 1) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::only one spectra available, cannot calculate coherency";
    throw std::runtime_error(err_str.str());
  }
  // get the channel map - get all cross spectra channel pairs from sa
  std::vector<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> chan_pairs = this->sa.generate_cross_spectra_channels();
  if (!chan_pairs.size()) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no channel pairs found for coherency calculation";
    throw std::runtime_error(err_str.str());
  }
  // auto chan_cross = this->sa.get_channel_pairs_cross();
  this->sa.info();
  // for (const auto &ac : chan_cross) {
  //   std::cout << "cross channel pair: " << ac.first->channel_type << ", " << ac.second->channel_type << std::endl;
  //  }
  for (const auto &chan_pair : chan_pairs) {
    std::cout << "cross channel pair: " << chan_pair.first->channel_type << ", " << chan_pair.second->channel_type << std::endl;
  }
  for (const auto &chan_pair : chan_pairs) {
    // we have a pair of channels, like <Hx, Hy>
    // we need to prepare the spectra for the coherency calculation
    this->coh.prepare_ac_cross_spectra(chan_pair);
    this->coh_prz.prepare_ac_cross_spectra(chan_pair);
    this->noise.prepare_ac_cross_spectra(chan_pair);
    this->noise_prz.prepare_ac_cross_spectra(chan_pair);
  }
  std::cerr << "pushing tasks to thread pool" << std::endl;
  for (const auto &chan_pair : chan_pairs) {
    // we have a pair of channels, like <Hx, Hy>
    // we need to calculate the coherency for this pair
    this->pool->detach_task([this, chan_pair, n_f_smooth]() {
      this->do_cross_coherence_raw_spectra(chan_pair, n_f_smooth);
    });
  }
  // now we have all coherency spectra in this->coh
}

void raw_spectra::coherence_raw_spectra_prz() {
  // so here we are in the raw_spectra class, contain all <Hx, > <Hy, > <Hz, > <Ex, > <Ey, >  and so on spectra
  // we want to calculate the coherency between all of them
  if (this->size() == 0) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no spectra for coherency calculation available";
    throw std::runtime_error(err_str.str());
  }
  if (this->size() == 1) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::only one spectra available, cannot calculate coherency";
    throw std::runtime_error(err_str.str());
  }
  // check if we have the sizes of this->coh_prz and this->noise_prz
  // the coherence_raw_spectra must have been called before
  if (!this->coh_prz.size() || !this->noise_prz.size()) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::coh_prz and noise_prz are not initialized, call coherence_raw_spectra() first";
    throw std::runtime_error(err_str.str());
  }
  // check if frequencies selected in this->fft_freqs
  if (this->fft_freqs == nullptr || this->fft_freqs->selected_freqs.empty()) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::fft_freqs is not initialized, call set_fft_freqs() first";
    throw std::runtime_error(err_str.str());
  }
  // finally we assume to parzen stack all or smooth stack all before
  // and the size of the first spectra in sa_prz or sa_avg must be the same size as selected_freqs
  if (this->sa_prz.size() == 0 && this->sa_avg.size() == 0) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no sa_prz or sa_avg spectra found, call parzen_stack_all() or smooth_stack_all() first";
    throw std::runtime_error(err_str.str());
  }
  // get the first spectrum from the map and check its size
  if (this->sa_prz.size() > 0) {
    auto first_spectrum = this->sa_prz.begin()->second;
    if (first_spectrum != nullptr && first_spectrum->size() != this->fft_freqs->selected_freqs.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::sa_prz size " << first_spectrum->size() << " does not match fft_freqs selected size " << this->fft_freqs->selected_freqs.size();
      throw std::runtime_error(err_str.str());
    }
  }
  std::cerr << "pushing tasks to thread pool" << std::endl;
  for (const auto &ac : this->coh_prz) {
    // we have a pair of channels, like <Hx, Hy>
    // we need to parzen the coherency for this pair
    this->pool->detach_task([this, chan_pair = ac.first]() {
      this->do_coh_noise_prz(chan_pair);
    });
  }
}
void raw_spectra::do_coh_noise_prz(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) {
  // we have the stacked spectra, get
  std::shared_ptr<std::vector<double>> in1;
  std::shared_ptr<std::vector<double>> out1;
  std::shared_ptr<std::vector<double>> in2;
  std::shared_ptr<std::vector<double>> out2;

  try {
    in1 = this->coh.get_spectra(chan_pair); // coherency spectra, vector of doubles
    out1 = this->coh_prz.get_spectra(chan_pair);
    in2 = this->noise.get_spectra(chan_pair);
    out2 = this->noise_prz.get_spectra(chan_pair);
  } catch (const std::exception &e) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::error retrieving spectra for coherence parzen: " << e.what();
    throw std::runtime_error(err_str.str());
  }
  // with the akima_vector_double I map in1 to out1 and in2 to out2
  bvec::akima_vector_double(this->fft_freqs->get_frequencies(), *in1, this->fft_freqs->get_selected_frequencies(), *out1);
  bvec::akima_vector_double(this->fft_freqs->get_frequencies(), *in2, this->fft_freqs->get_selected_frequencies(), *out2);
}