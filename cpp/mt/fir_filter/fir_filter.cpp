#include "fir_filter.h"

#include "about_system.h"

fir_filter::fir_filter() {
}

fir_filter::~fir_filter() {
  if (this->in_chan != nullptr)
    this->in_chan.reset();
  if (this->out_chan != nullptr)
    this->out_chan.reset();
}

std::shared_ptr<channel> fir_filter::set_filter(std::shared_ptr<channel> &chan, const std::string &filter_type, const bool &shift_to_full_seconds, const int64_t grid_raster) {

  if (chan == nullptr) {
    std::ostringstream err_str((std::string("fir_filter::") + __func__), std::ios_base::ate);
    err_str << " channel is null ";
    throw std::runtime_error(err_str.str());
  }
  // check db file
  this->db_file = working_dir_data(db_sql_file);
  if (!std::filesystem::exists(this->db_file)) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << ":: database not found " << this->db_file;
    throw std::runtime_error(err_str.str());
  }
  this->in_chan = chan;
  if (this->out_chan != nullptr)
    this->out_chan.reset();
  this->out_chan = std::make_shared<channel>(chan); // that is a new channel, not a copy

  sql_filter = std::make_unique<sqlite_handler>(this->db_file);

  this->filter_type = filter_type;
  this->filter_factor = std::stoi(filter_type.substr(3));

  std::string sql_query = "SELECT * FROM " + this->filter_type;
  try {
    this->coeff = sql_filter->sqlite_vector_double(sql_query);
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    this->sql_filter.reset();
  }
  this->out_chan->set_sample_rate(this->out_chan->get_sample_rate() / double(this->filter_factor));

  this->shift_to_new_start_time(shift_to_full_seconds); // do last - before we calculate time offset and shift samples of input

  // skip samples output to fit into the raster
  if (grid_raster) {
    this->grid = grid_raster;
  }

  return this->out_chan;
}

void fir_filter::filter() {

  std::cout << this->in_chan->get_filepath_wo_ext() << " -> " << this->out_chan->get_filepath_wo_ext() << std::endl;
  // read samples from input channel
  // shift samples to fit into the raster
  // filter samples
  // write samples to output channel
  std::ofstream file;
  std::vector<double> in(this->coeff.size()), out;
  out.reserve(256); // write in chunks of 256 samples
  this->in_chan->ts_slice.resize(this->coeff.size());
  this->in_chan->open_atss_read();
  this->in_chan->skip_samples(this->samples_skip);

  // first read must be full data size, not a chunk
  if (this->in_chan->read_data(false)) {
    out.push_back(bvec::fold(this->in_chan->ts_slice, this->coeff));
  }
  // now we continue with chunks by setting the chunk size
  this->in_chan->ts_chunk.resize(this->filter_factor);
  while (this->in_chan->read_data() > 0) {
    out.push_back(bvec::fold(this->in_chan->ts_slice, this->coeff));
    if (out.size() == 256) {
      this->out_chan->write_data(out);
      out.clear();
      out.reserve(256);
    }
  }
  if (out.size()) {
    this->out_chan->write_data(out);
  }
  this->in_chan->close_atss_read();
  this->out_chan->close_outfile();
}

void fir_filter::shift_to_new_start_time(const bool &shift_to_full_seconds) {
  // example for 128 Hz and 32 x filter
  // fracs = 1.8398s delay = 471 * 0.5 = 235 / 128.
  // 1.83984375 * 128 = 235.5 samples -> round up
  // return 2

  if (!this->coeff.size()) {
    std::ostringstream err_str((std::string("fir_filter::") + __func__), std::ios_base::ate);
    err_str << " filter has no coefficients ";
    throw std::runtime_error(err_str.str());
  }
  if ((this->coeff.size() % 2) == 0) {
    std::ostringstream err_str((std::string("fir_filter::") + __func__), std::ios_base::ate);
    err_str << " filter is even - must be odd ";
    throw std::runtime_error(err_str.str());
  }

  size_t hlen = (this->coeff.size() - 1) / 2;                          // half length of the filter -1; we can't use half samples
  double delay_time = double(hlen) / this->in_chan->get_sample_rate(); // time needed for 1/2 filter length

  if (!shift_to_full_seconds) {
    double fullsecs;
    auto fracs = modf(delay_time, &fullsecs);
    this->full_secs = int64_t(fullsecs);
    this->frac_secs = fracs;
    this->out_chan->pt.add_secs(fullsecs, fracs);
    return;
  }

  // if it is 1.1 -> we need 2 seconds
  double shift_time = (ceil(delay_time));

  double fill_in_time = shift_time - delay_time;
  this->samples_skip = size_t(fill_in_time * this->in_chan->get_sample_rate());
  this->samples_delay = size_t(delay_time * this->in_chan->get_sample_rate());
  size_t samples_control = samples_skip + samples_delay;

  if ((samples_skip + samples_delay) != size_t(shift_time * this->in_chan->get_sample_rate())) {
    std::ostringstream err_str((std::string("fir_filter::") + __func__), std::ios_base::ate);
    err_str << " filter can not calculate full seconds start time, integer parts ";
    throw std::runtime_error(err_str.str());
  }
  size_t samples_at_new_start_time = size_t(shift_time * this->in_chan->get_sample_rate());

  double fullsecs;
  auto fracs = modf((double(samples_at_new_start_time) / this->in_chan->get_sample_rate()), &fullsecs);

  if (std::abs(fracs) > treat_as_null) {
    std::ostringstream err_str((std::string("fir_filter::") + __func__), std::ios_base::ate);
    err_str << " filter can not calculate full seconds start time sec calc";
    throw std::runtime_error(err_str.str());
  }
  // copy finally to class members
  this->full_secs = int64_t(shift_time);
  // set the start time of the output channel in the json file
  this->out_chan->pt.tt += int64_t(this->full_secs);
}

std::string fir_filter::get_info() const {
  std::ostringstream info_str;
  info_str << "fir_filter: " << this->filter_type << " " << this->filter_factor << "x";
  info_str << "  full secs: " << this->full_secs << "  frac secs: " << this->frac_secs;
  info_str << "  samples skip: " << this->samples_skip << "  samples delay: " << this->samples_delay << "  sum: " << this->samples_skip + this->samples_delay;
  return info_str.str();
}

void fir_filter::shift_to_grid(const int64_t grid_raster) {
  if (grid_raster <= 0) {
    std::ostringstream err_str((std::string("fir_filter::") + __func__), std::ios_base::ate);
    err_str << " grid raster is 0 or negative " + std::to_string(grid_raster);
    throw std::runtime_error(err_str.str());
  }
  if (this->out_chan->pt.fracs > treat_as_null) {
    this->out_chan->pt.fracs = 0.0;
    this->out_chan->pt.tt += 1;
  }
  auto start_time = this->in_chan->pt;
  // will be equal or less; integer division rounds always down
  start_time.tt = (start_time.tt / grid_raster) * grid_raster;
  if (start_time < this->in_chan->pt)
    start_time.tt += grid_raster;
}
