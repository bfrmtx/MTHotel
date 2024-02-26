#include "raw_spectra.h"

void raw_spectra::set_raw_spectra(std::shared_ptr<channel> chan) {

  this->bw = chan->bw;
  bool submitted = false;
  // enclose the if and else statements in brackets to avoid "jump to case label" error
  if (chan->channel_type == "Ex") {
    if (!chan->is_emap && !chan->is_remote) {
      submitted = true;
      std::swap(chan->spc, this->ex);
    } else if (chan->is_emap && !chan->is_remote) {
      submitted = true;
      std::swap(chan->spc, this->eex);
    } else if (!chan->is_emap && chan->is_remote) {
      submitted = true;
      std::swap(chan->spc, this->rex);
    }
  }

  if (chan->channel_type == "Ey") {
    if (!chan->is_emap && !chan->is_remote) {
      submitted = true;
      std::swap(chan->spc, this->ey);
    } else if (chan->is_emap && !chan->is_remote) {
      submitted = true;
      std::swap(chan->spc, this->eey);
    } else if (!chan->is_emap && chan->is_remote) {
      submitted = true;
      std::swap(chan->spc, this->rey);
    }
  }

  if (chan->channel_type == "Hx") {
    if (!chan->is_emap && !chan->is_remote) {
      submitted = true;
      std::swap(chan->spc, this->hx);
    } else if (!chan->is_emap && chan->is_remote) {
      submitted = true;
      std::swap(chan->spc, this->rhx);
    }
  }

  if (chan->channel_type == "Hy") {
    if (!chan->is_emap && !chan->is_remote) {
      submitted = true;
      std::swap(chan->spc, this->hy);
    } else if (!chan->is_emap && chan->is_remote) {
      submitted = true;
      std::swap(chan->spc, this->rhy);
    }
  }

  if (chan->channel_type == "Hz") {
    if (!chan->is_emap && !chan->is_remote) {
      submitted = true;
      std::swap(chan->spc, this->hz);
    } else if (!chan->is_emap && chan->is_remote) {
      submitted = true;
      std::swap(chan->spc, this->rhz);
    }
  }

  if (!submitted) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no raw spectra found for SWAPPING! " << chan->channel_type << " remote: " << chan->is_remote << " emap: " << chan->is_emap;
    throw std::runtime_error(err_str.str());
  } else {
    this->channels.push_back(chan);
  }
}

void raw_spectra::simple_stack_all() {
  std::cerr << "pushing tasks to thread pool" << std::endl;
  bool submitted = false;
  if (this->ex.size()) {
    submitted = true;
    this->pool->push_task(simple_ampl_stack, std::ref(this->ex), std::ref(this->sa_ex));
  }
  if (this->ey.size()) {
    submitted = true;
    this->pool->push_task(simple_ampl_stack, std::ref(this->ey), std::ref(this->sa_ey));
  }
  if (this->hx.size()) {
    submitted = true;
    this->pool->push_task(simple_ampl_stack, std::ref(this->hx), std::ref(this->sa_hx));
  }
  if (this->hy.size()) {
    submitted = true;
    this->pool->push_task(simple_ampl_stack, std::ref(this->hy), std::ref(this->sa_hy));
  }
  if (this->hz.size()) {
    submitted = true;
    this->pool->push_task(simple_ampl_stack, std::ref(this->hz), std::ref(this->sa_hz));
  }
  if (this->rex.size()) {
    submitted = true;
    this->pool->push_task(simple_ampl_stack, std::ref(this->rex), std::ref(this->sa_rex));
  }
  if (this->rey.size()) {
    submitted = true;
    this->pool->push_task(simple_ampl_stack, std::ref(this->rey), std::ref(this->sa_rey));
  }
  if (this->rhx.size()) {
    submitted = true;
    this->pool->push_task(simple_ampl_stack, std::ref(this->rhx), std::ref(this->sa_rhx));
  }
  if (this->rhy.size()) {
    submitted = true;
    this->pool->push_task(simple_ampl_stack, std::ref(this->rhy), std::ref(this->sa_rhy));
  }
  if (this->rhz.size()) {
    submitted = true;
    this->pool->push_task(simple_ampl_stack, std::ref(this->rhz), std::ref(this->sa_rhz));
  }
  if (this->eex.size()) {
    submitted = true;
    this->pool->push_task(simple_ampl_stack, std::ref(this->eex), std::ref(this->sa_eex));
  }
  if (this->eey.size()) {
    submitted = true;
    this->pool->push_task(simple_ampl_stack, std::ref(this->eey), std::ref(this->sa_eey));
  }

  // throw error if none has size
  if (!submitted) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no spectra for stacking available";
    throw std::runtime_error(err_str.str());
  }
}

void raw_spectra::simple_stack_all_div(const std::shared_ptr<raw_spectra> raw, const std::string &channel_type) {
}
void raw_spectra::advanced_stack_all(const double &fraction_to_use) {
  std::cerr << "pushing tasks to thread pool" << std::endl;

  bool submitted = false;

  if (this->ex.size()) {
    submitted = true;
    this->pool->push_task(advanced_ampl_stack, std::ref(this->ex), std::ref(this->sa_ex), std::ref(fraction_to_use));
  }
  if (this->ey.size()) {
    submitted = true;
    this->pool->push_task(advanced_ampl_stack, std::ref(this->ey), std::ref(this->sa_ey), std::ref(fraction_to_use));
  }
  if (this->hx.size()) {
    submitted = true;
    this->pool->push_task(advanced_ampl_stack, std::ref(this->hx), std::ref(this->sa_hx), std::ref(fraction_to_use));
  }
  if (this->hy.size()) {
    submitted = true;
    this->pool->push_task(advanced_ampl_stack, std::ref(this->hy), std::ref(this->sa_hy), std::ref(fraction_to_use));
  }
  if (this->hz.size()) {
    submitted = true;
    this->pool->push_task(advanced_ampl_stack, std::ref(this->hz), std::ref(this->sa_hz), std::ref(fraction_to_use));
  }

  if (this->rex.size()) {
    submitted = true;
    this->pool->push_task(advanced_ampl_stack, std::ref(this->rex), std::ref(this->sa_rex), std::ref(fraction_to_use));
  }
  if (this->rey.size()) {
    submitted = true;
    this->pool->push_task(advanced_ampl_stack, std::ref(this->rey), std::ref(this->sa_rey), std::ref(fraction_to_use));
  }
  if (this->rhx.size()) {
    submitted = true;
    this->pool->push_task(advanced_ampl_stack, std::ref(this->rhx), std::ref(this->sa_rhx), std::ref(fraction_to_use));
  }
  if (this->rhy.size()) {
    submitted = true;
    this->pool->push_task(advanced_ampl_stack, std::ref(this->rhy), std::ref(this->sa_rhy), std::ref(fraction_to_use));
  }
  if (this->rhz.size()) {
    submitted = true;
    this->pool->push_task(advanced_ampl_stack, std::ref(this->rhz), std::ref(this->sa_rhz), std::ref(fraction_to_use));
  }

  if (this->eex.size()) {
    this->pool->push_task(advanced_ampl_stack, std::ref(this->eex), std::ref(this->sa_eex), std::ref(fraction_to_use));
  }
  if (this->eey.size()) {
    submitted = true;
    this->pool->push_task(advanced_ampl_stack, std::ref(this->eey), std::ref(this->sa_eey), std::ref(fraction_to_use));
  }

  // throw error if none has size
  if (!submitted) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no spectra for stacking available";
    throw std::runtime_error(err_str.str());
  }
}

void raw_spectra::parzen_stack_all() {
  std::cerr << "pushing tasks to thread pool" << std::endl;
  bool submitted = false;
  if (this->sa_ex.size()) {
    submitted = true;
    this->pool->push_task(parzen<double, double>, std::ref(this->sa_ex), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_ex));
  }
  if (this->sa_ey.size()) {
    submitted = true;
    this->pool->push_task(parzen<double, double>, std::ref(this->sa_ey), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_ey));
  }
  if (this->sa_hx.size()) {
    submitted = true;
    this->pool->push_task(parzen<double, double>, std::ref(this->sa_hx), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_hx));
  }
  if (this->sa_hy.size()) {
    submitted = true;
    this->pool->push_task(parzen<double, double>, std::ref(this->sa_hy), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_hy));
  }
  if (this->sa_hz.size()) {
    submitted = true;
    this->pool->push_task(parzen<double, double>, std::ref(this->sa_hz), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_hz));
  }
  if (this->sa_rex.size()) {
    submitted = true;
    this->pool->push_task(parzen<double, double>, std::ref(this->sa_rex), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_rex));
  }
  if (this->sa_rey.size()) {
    submitted = true;
    this->pool->push_task(parzen<double, double>, std::ref(this->sa_rey), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_rey));
  }
  if (this->sa_rhx.size()) {
    submitted = true;
    this->pool->push_task(parzen<double, double>, std::ref(this->sa_rhx), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_rhx));
  }
  if (this->sa_rhy.size()) {
    this->pool->push_task(parzen<double, double>, std::ref(this->sa_rhy), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_rhy));
  }
  if (this->sa_rhz.size()) {
    this->pool->push_task(parzen<double, double>, std::ref(this->sa_rhz), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_rhz));
  }
  if (this->sa_eex.size()) {
    submitted = true;
    this->pool->push_task(parzen<double, double>, std::ref(this->sa_eex), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_eex));
  }
  if (this->sa_eey.size()) {
    submitted = true;
    this->pool->push_task(parzen<double, double>, std::ref(this->sa_eey), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_eey));
  }

  // throw error if none has size
  if (!submitted) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no spectra for parzening available";
    throw std::runtime_error(err_str.str());
  }
}

std::vector<double> raw_spectra::get_abs_sa_spectra(const std::string &channel_type, const bool is_remote, const bool is_emap) const {
  bool submitted = false;
  if (channel_type == "Ex") {
    if (!is_emap && !is_remote && this->sa_ex.size()) {
      submitted = true;
      return this->sa_ex;
    } else if (is_emap && !is_remote && this->sa_eex.size()) {
      submitted = true;
      return this->sa_eex;
    } else if (!is_emap && is_remote && this->sa_rex.size()) {
      submitted = true;
      return this->sa_rex;
    }
  }

  if (channel_type == "Ey") {
    if (!is_emap && !is_remote && this->sa_ey.size()) {
      submitted = true;
      return this->sa_ey;
    } else if (is_emap && !is_remote && this->sa_eey.size()) {
      submitted = true;
      return this->sa_eey;
    } else if (!is_emap && is_remote && this->sa_rey.size()) {
      submitted = true;
      return this->sa_rey;
    }
  }

  if (channel_type == "Hx") {
    if (!is_emap && !is_remote && this->sa_hx.size()) {
      submitted = true;
      return this->sa_hx;
    } else if (!is_emap && is_remote && this->sa_rhx.size()) {
      submitted = true;
      return this->sa_rhx;
    }
  }

  if (channel_type == "Hy") {
    if (!is_emap && !is_remote && this->sa_hy.size()) {
      submitted = true;
      return this->sa_hy;
    } else if (!is_emap && is_remote && this->sa_rhy.size()) {
      submitted = true;
      return this->sa_rhy;
    }
  }

  if (channel_type == "Hz") {
    if (!is_emap && !is_remote && this->sa_hz.size()) {
      submitted = true;
      return this->sa_hz;
    } else if (!is_emap && is_remote && this->sa_rhz.size()) {
      submitted = true;
      return this->sa_rhz;
    }
  }

  if (!submitted) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no spectra found! " << channel_type << " remote: " << is_remote << " emap: " << is_emap;
    throw std::runtime_error(err_str.str());
  }

  return std::vector<double>();
}

std::pair<double, double> raw_spectra::get_abs_sa_spectra_min_max(const std::string &channel_type, const bool is_remote, const bool is_emap) const {
  std::pair<double, double> result(DBL_MIN, DBL_MAX);
  auto v = this->get_abs_sa_spectra(channel_type, is_remote, is_emap);
  auto xmm = std::minmax_element(v.cbegin(), v.cend());
  result.first = *xmm.first;
  result.second = *xmm.second;

  return result;
}

std::vector<double> raw_spectra::get_abs_sa_prz_spectra(const std::string &channel_type, const bool is_remote, const bool is_emap) const {
  if (channel_type == "Ex") {
    if (!is_emap && !is_remote && this->sa_prz_ex.size()) {
      return this->sa_prz_ex;
    } else if (is_emap && !is_remote && this->sa_prz_eex.size()) {
      return this->sa_prz_eex;
    } else if (!is_emap && is_remote && this->sa_prz_rex.size()) {
      return this->sa_prz_rex;
    }
  }

  if (channel_type == "Ey") {
    if (!is_emap && !is_remote && this->sa_prz_ey.size()) {
      return this->sa_prz_ey;
    } else if (is_emap && !is_remote && this->sa_prz_eey.size()) {
      return this->sa_prz_eey;
    } else if (!is_emap && is_remote && this->sa_prz_rey.size()) {
      return this->sa_prz_rey;
    }
  }

  if (channel_type == "Hx") {
    if (!is_emap && !is_remote && this->sa_prz_hx.size()) {
      return this->sa_prz_hx;
    } else if (!is_emap && is_remote && this->sa_prz_rhx.size()) {
      return this->sa_prz_rhx;
    }
  }

  if (channel_type == "Hy") {
    if (!is_emap && !is_remote && this->sa_prz_hy.size()) {
      return this->sa_prz_hy;
    } else if (!is_emap && is_remote && this->sa_prz_rhy.size()) {
      return this->sa_prz_rhy;
    }
  }

  if (channel_type == "Hz") {
    if (!is_emap && !is_remote && this->sa_prz_hz.size()) {
      return this->sa_prz_hz;
    } else if (!is_emap && is_remote && this->sa_prz_rhz.size()) {
      return this->sa_prz_rhz;
    }
  }

  return std::vector<double>();
}

std::vector<double> raw_spectra::get_abs_spectra(const std::string &channel_type, const size_t nstack, const bool is_remote, const bool is_emap) const {
  std::vector<double> abs_spc;
  std::vector<std::complex<double>>::const_iterator citer_b;
  std::vector<std::complex<double>>::const_iterator citer_e(citer_b);

  if (channel_type == "Ex") {
    if (!is_emap && !is_remote && (this->ex.size() > nstack)) {
      citer_b = this->ex.at(nstack).cbegin();
      citer_e = this->ex.at(nstack).cend();
    } else if (is_emap && !is_remote && (this->eex.size() > nstack)) {
      citer_b = this->eex.at(nstack).cbegin();
      citer_e = this->eex.at(nstack).cend();
    } else if (!is_emap && is_remote && (this->rex.size()) > nstack) {
      citer_b = this->rex.at(nstack).cbegin();
      citer_e = this->rex.at(nstack).cend();
    }
  }

  if (channel_type == "Ey") {
    if (!is_emap && !is_remote && (this->ey.size() > nstack)) {
      citer_b = this->ey.at(nstack).cbegin();
      citer_e = this->ey.at(nstack).cend();
    } else if (is_emap && !is_remote && (this->eey.size() > nstack)) {
      citer_b = this->eey.at(nstack).cbegin();
      citer_e = this->eey.at(nstack).cend();
    } else if (!is_emap && is_remote && (this->rey.size() > nstack)) {
      citer_b = this->rey.at(nstack).cbegin();
      citer_e = this->rey.at(nstack).cend();
    }
  }

  if (channel_type == "Hx") {
    if (!is_emap && !is_remote && (this->hx.size() > nstack)) {
      citer_b = this->hx.at(nstack).cbegin();
      citer_e = this->hx.at(nstack).cend();
    } else if (!is_emap && is_remote && (this->rhx.size() > nstack)) {
      citer_b = this->rhx.at(nstack).cbegin();
      citer_e = this->rhx.at(nstack).cend();
    }
  }

  if (channel_type == "Hy") {
    if (!is_emap && !is_remote && (this->hy.size() > nstack)) {
      citer_b = this->hy.at(nstack).cbegin();
      citer_e = this->hy.at(nstack).cend();
    } else if (!is_emap && is_remote && (this->rhy.size() > nstack)) {
      citer_b = this->rhy.at(nstack).cbegin();
      citer_e = this->rhy.at(nstack).cend();
    }
  }

  if (channel_type == "Hz") {
    if (!is_emap && !is_remote && (this->hz.size() > nstack)) {
      citer_b = this->hz.at(nstack).cbegin();
      citer_e = this->hz.at(nstack).cend();
    } else if (!is_emap && is_remote && (this->rhz.size() > nstack)) {
      citer_b = this->rhz.at(nstack).cbegin();
      citer_e = this->rhz.at(nstack).cend();
    }
  }

  abs_spc.reserve(std::distance(citer_b, citer_e));

  while (citer_b != citer_e) {
    abs_spc.push_back(std::abs(*citer_b++));
  }

  return abs_spc;
}

void raw_spectra::scale_by_stacked_spectra_local(std::vector<double> &nominator, const bool is_remote, const bool is_emap) {
  // get an iterator to the first element of the spectra of this->sa _ex sa_ey sa_hx sa_hy sa_hz

  std::vector<double>::iterator iter_nominator = nominator.begin();
  std::vector<double>::iterator iter_denominator;
  std::vector<double>::iterator iter_begin;

  if (this->ex.size()) {
    iter_denominator = this->sa_ex.begin();
    // divide the nominator by the denominator
    while (iter_denominator != this->sa_ex.end()) {
      *iter_denominator = *iter_nominator / *iter_denominator;
      ++iter_nominator;
      ++iter_denominator;
    }
    iter_begin = iter_nominator;
  }

  if (this->ey.size()) {
    iter_denominator = this->sa_ey.begin();
    // divide the nominator by the denominator
    while (iter_denominator != this->sa_ey.end()) {
      *iter_denominator = *iter_nominator / *iter_denominator;
      ++iter_nominator;
      ++iter_denominator;
    }
    iter_begin = iter_nominator;
  }

  if (this->sa_hx.size()) {
    iter_denominator = this->sa_hx.begin();
    // divide the nominator by the denominator
    while (iter_denominator != this->sa_hx.end()) {
      *iter_denominator = *iter_nominator / *iter_denominator;
      ++iter_nominator;
      ++iter_denominator;
    }
    iter_begin = iter_nominator;
  }

  if (this->sa_hy.size()) {
    iter_denominator = this->sa_hy.begin();
    // divide the nominator by the denominator
    while (iter_denominator != this->sa_hy.end()) {
      *iter_denominator = *iter_nominator / *iter_denominator;
      ++iter_nominator;
      ++iter_denominator;
    }
    iter_begin = iter_nominator;
  }

  if (this->sa_hz.size()) {
    iter_denominator = this->sa_hz.begin();
    // divide the nominator by the denominator
    while (iter_denominator != this->sa_hz.end()) {
      *iter_denominator = *iter_nominator / *iter_denominator;
      ++iter_nominator;
      ++iter_denominator;
    }
    iter_begin = iter_nominator;
  }
}
