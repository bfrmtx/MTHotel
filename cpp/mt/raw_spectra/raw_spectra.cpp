#include "raw_spectra.h"

void raw_spectra::move_raw_spectra(std::shared_ptr<channel> chan) {

  this->add_spectra(chan);
  this->channels.push_back(chan);
}

void raw_spectra::advanced_stack_all(const double &fraction_to_use) {
  if (this->size() == 0) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no spectra for stacking available";
    throw std::runtime_error(err_str.str());
  }
  std::cerr << "pushing tasks to thread pool" << std::endl;

  for (auto &ac : this->sa) {
    if (fraction_to_use < 0.0 || fraction_to_use > 1.0) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::fraction_to_use must be between 0.01 and 1.0";
      throw std::runtime_error(err_str.str());
    }
    if (sa.is_auto_spc(ac.first)) {
      // this->pool->submit_task([this, ac, fraction_to_use]() {
      //   this->do_advanced_stack_auto(ac.first, fraction_to_use);
      // });
      this->pool->detach_task([this, &ac, fraction_to_use]() {
        this->do_advanced_stack_auto(ac.first, fraction_to_use);
      });
    } else {
      this->pool->detach_task([this, &ac, fraction_to_use]() {
        this->do_advanced_stack_cross(ac.first, fraction_to_use);
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

  std::cerr << "pushing tasks to thread pool" << std::endl;

  for (auto &ac : this->sa_prz) {
    // get the key from sa_prz, which is the same as in sa
    auto result = this->sa_prz.get_spectra(ac.first); // put the result into sa_prz
    // task to push is parzen_t<double, double> with the parameters ac ...
    // from prz_vecor.h:
    // we call void parzen_t(const std::shared_ptr<std::vector<T>> &data, const std::vector<S> &selected_freqs, const std::vector<std::vector<S>> &parzendists, std::shared_ptr<std::vector<T>> &result) {
    // parzen(*data, selected_freqs, parzendists, *result);
    // this->pool->push_task(parzen_t<double, double>, std::ref(this->sa[ac.first]), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(result));
    parzen_t<double, double>(this->sa[ac.first], this->fft_freqs->selected_freqs, this->fft_freqs->parzendists, result);
  }

  // old:     this->pool->push_task(parzen<double, double>, std::ref(this->sa_ex), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_ex));
  // for (auto &m : this->sa) {
  //   this->sa_prz.add_spectra(m.first); // add an empty vector, but generate the key in sa_prz
  //   // this->pool->push_task(parzen_t<double, double>, std::ref(m.second), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz[m.first]));
  //   // this->pool->push_task(parzen<double, double>, *m.second, std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), *this->sa_prz[m.first]);
  // }
}

std::vector<double> raw_spectra::get_abs_sa_spectra(const std::pair<std::string, std::string> &name, const bool is_remote, const bool is_emap) const {
  if (this->sa.size() == 0) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no stack all spectra found!";
    throw std::runtime_error(err_str.str());
  }
  return this->sa.get_spectra_vec(name);
}

std::pair<double, double> raw_spectra::get_abs_sa_spectra_min_max(const std::pair<std::string, std::string> &name, const bool is_remote, const bool is_emap) const {
  std::pair<double, double> result(DBL_MIN, DBL_MAX);
  auto v = this->get_abs_sa_spectra(name, is_remote, is_emap);
  auto xmm = std::minmax_element(v.cbegin(), v.cend());
  result.first = *xmm.first;
  result.second = *xmm.second;
  return result;
}

std::vector<double> raw_spectra::get_abs_sa_prz_spectra(const std::pair<std::string, std::string> &name, const bool is_remote, const bool is_emap) const {
  if (this->sa_prz.size() == 0) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no stack all spectra found!";
    throw std::runtime_error(err_str.str());
  }
  return this->sa_prz.get_spectra_vec(name);
}

std::string raw_spectra::get_sensor_name(const std::pair<std::string, std::string> &name) const {
  auto name1 = name.first;
  auto name2 = name.second;
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

std::string raw_spectra::get_sensor_serial(const std::pair<std::string, std::string> &name) const {
  auto name1 = name.first;
  auto name2 = name.second;
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

std::string raw_spectra::get_sensor_name_serial(const std::pair<std::string, std::string> &name, const bool cat_underscore) const {
  auto name1 = name.first;
  auto name2 = name.second;
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

std::string raw_spectra::get_sampling_rate(const std::pair<std::string, std::string> &name) const {
  for (const auto &c : this->channels) {
    if (c->channel_type == name.first) {
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

void raw_spectra::dump_sa_spectra() const {
  // if not exist; create a directory at users home, called dump_spectra
  fs::path home_dir_dump(getenv("HOME"));
  home_dir_dump /= "dump_spectra";
  if (!std::filesystem::exists(home_dir_dump)) {
    std::filesystem::create_directory(home_dir_dump);
  }
  // dump all spectra in sa to files
  for (const auto &ac : this->sa) {
    std::string spc_name = ac.first.first + "_" + ac.first.second;
    std::string coils = this->get_sensor_name_serial(ac.first, true);
    std::string sampling_rate = this->get_sampling_rate(ac.first);
    fs::path filename = home_dir_dump / (spc_name + "_" + coils + "_" + sampling_rate + ".dat");
    std::ofstream out(filename);
    if (!out) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::cannot open file " << filename;
      throw std::runtime_error(err_str.str());
    }
    auto f = this->fft_freqs->get_frequencies();
    size_t i = 0;
    for (const auto &d : *ac.second) {
      out << f[i++] << " " << d << std::endl;
    }
    out.close();
  }
}

void raw_spectra::dump_sa_prz_spectra() const {
  // if not exist; create a directory at users home, called dump_spectra
  fs::path home_dir_dump(getenv("HOME"));
  home_dir_dump /= "dump_spectra";
  if (!std::filesystem::exists(home_dir_dump)) {
    std::filesystem::create_directory(home_dir_dump);
  }
  // dump all spectra in sa_prz to files
  for (const auto &ac : this->sa_prz) {
    std::string spc_name = ac.first.first + "_" + ac.first.second;
    std::string coils = this->get_sensor_name_serial(ac.first, true);
    std::string sampling_rate = this->get_sampling_rate(ac.first);
    fs::path filename = home_dir_dump / (spc_name + "_" + coils + "_" + sampling_rate + "_prz.dat");
    std::ofstream out(filename);
    if (!out) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::cannot open file " << filename;
      throw std::runtime_error(err_str.str());
    }
    auto f = this->fft_freqs->get_selected_frequencies();
    size_t i = 0;
    for (const auto &d : *ac.second) {
      out << f[i++] << " " << d << std::endl;
    }
    out.close();
  }
}

void raw_spectra::do_advanced_stack_auto(const std::pair<std::string, std::string> &name, const double &fraction_to_use) {
  auto in = this->get_spectra(name.first); // complex spectra, vector of vectors from raw_spectra
  size_t n = in->at(0).size();             // stack size
  auto out = this->sa.get_spectra(name);   // double spectra, vector, we reserved the space in advance
  bool adv = (fraction_to_use < 1.0);
  out->resize(n, 0.0);
  for (size_t i = 0; i < n; ++i) {                  //   for frequencies
    auto ff = bvec::absv(bvec::get_fslice(*in, i)); // get all stacks,
    if (!adv) {
      out->at(i) = bvec::mean(ff);
    } else {
      out->at(i) = bvec::median_range_mean(ff, fraction_to_use); // two_pass_variance var, var.variance(ff.cbegin(), ff.cend());
    }
  }
  // out is a shared pointer to a vector of doubles, we don't need to return it or copy it
}

void raw_spectra::do_advanced_stack_cross(const std::pair<std::string, std::string> &name, const double &fraction_to_use) {
  auto in1 = this->get_spectra(name.first);  // complex spectra, vector of vectors
  auto in2 = this->get_spectra(name.second); // complex spectra, vector of vectors
  auto out = this->sa.get_spectra(name);     // double spectra vector, we reserved the space in advance
  size_t n = in1->at(0).size();              // stack size
  bool adv = (fraction_to_use < 1.0);
  out->resize(n, 0.0); // stack size

  for (size_t i = 0; i < n; ++i) {        // for all stacks
    auto ff1 = bvec::get_fslice(*in1, i); // get all stacks
    auto ff2 = bvec::get_fslice(*in2, i); // get all stacks
    std::vector<double> ff = bvec::make_cross_sqrt_conj_abs(ff1, ff2);
    if (!adv) {
      out->at(i) = bvec::mean(ff);
    } else {
      out->at(i) = bvec::median_range_mean(ff, fraction_to_use);
    }
  }
}
