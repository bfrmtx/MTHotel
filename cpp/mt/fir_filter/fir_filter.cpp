#include "fir_filter.h"


fir_filter::fir_filter()
{

}

fir_filter::~fir_filter()
{
    if (this->in_chan != nullptr) this->in_chan.reset();
    if (this->out_chan != nullptr) this->out_chan.reset();
}

std::shared_ptr<channel> fir_filter::set_filter(std::shared_ptr<channel> &chan, const std::string &filter_type, const bool &shift_to_full_seconds, const int64_t grid_raster)
{
    auto dbfile = working_dir("data", db_sql_file);

    if (!std::filesystem::exists(dbfile)) {
        dbfile = getenv("HOME");
        dbfile /= "devel/github_mthotel/MTHotel/cpp/data/" db_sql_file;

        if (!std::filesystem::exists(dbfile)) {
            std::string err_str = __func__;
            err_str += ":: database not found " db_sql_file;
            err_str += dbfile.string();
            throw err_str;
        }
    }

    sql_info = std::make_unique<sqlite_handler>();

    this->out_chan = std::make_shared<channel>(chan);  // that is a new channel, not a copy
    this->in_chan = chan;
    this->filter_type = filter_type;

    if (this->filter_type == "mtx4") {
        std::string sql_query("SELECT * FROM mtx4;");
        try {
            this->coeff = sql_info->sqlite_vector_double(dbfile, sql_query);
        }
        catch (const std::string &error) {
            std::cerr << error << std::endl;
            this->sql_info.reset();
        }
        this->out_chan->set_sample_rate(this->out_chan->get_sample_rate() / 4.0);
    }

    this->shift_to_new_start_time(shift_to_full_seconds);     // do last - before we calculate time offset and shift samples of input




    // skip samples output to fit into the raster
    if (grid_raster) {
        this->grid = grid_raster;
    }

    return this->out_chan;
}

void fir_filter::filter()
{

}


void fir_filter::shift_to_new_start_time(const bool &shift_to_full_seconds)
{
    // example for 128 Hz and 32 x filter
    // fracs = 1.8398s delay = 471 * 0.5 = 235 / 128.
    // 1.83984375 * 128 = 235.5 samples -> round up
    // return 2

    if (!this->coeff.size()) {
        std::string err_str = std::string("fir_filter::") + __func__;
        err_str += "::filter has no coefficients ";
        throw err_str;
        return;
    }
    if ( (this->coeff.size() % 2) == 0) {
        std::string err_str = std::string("fir_filter::") + __func__;
        err_str += "::filter is even - must be odd ";
        throw err_str;
        return;
    }

    size_t hlen = (this->coeff.size() -1) / 2;
    double delay_time = double(hlen) / this->in_chan->get_sample_rate(); // time needed for 1/2 filter length


    if (!shift_to_full_seconds) {
        double fullsecs;
        auto fracs = modf (delay_time, &fullsecs);
        this->out_chan->pt.add_secs(fullsecs, fracs);
        return;

    }

    // if it is 1.1 -> we need 2 seconds
    double shift_time =  (ceil(delay_time));

    double fill_in_time = shift_time - delay_time;
    size_t samples_shift = size_t(fill_in_time * this->in_chan->get_sample_rate());
    size_t samples_delay = size_t( delay_time * this->in_chan->get_sample_rate());
    size_t samples_control = samples_shift + samples_delay;

    if((samples_shift + samples_delay) != size_t(shift_time * this->in_chan->get_sample_rate())) {
        std::string err_str = std::string("fir_filter::") + __func__;
        err_str += "::filter can not calculate full seconds start time, integer parts ";
        throw err_str;
        return;
    }

    size_t samples_at_new_start_time = size_t(shift_time * this->in_chan->get_sample_rate());


    double fullsecs;
    auto fracs = modf ((double(samples_at_new_start_time)/this->in_chan->get_sample_rate()), &fullsecs);



    if (std::abs(fracs) > treat_as_null ) {
        std::string err_str = std::string("fir_filter::") + __func__;
        err_str += "::filter can not calculate full seconds start time sec calc";
        throw err_str;
        return;
    }

    this->out_chan->pt.tt += int64_t(fullsecs);


}

void fir_filter::shift_to_grid(const int64_t grid_raster)
{
    if (grid_raster <= 0) {
        std::string err_str = std::string("fir_filter::") + __func__;
        err_str += "::grid raster is 0 or negative " + std::to_string(grid_raster);
        throw err_str;
        return;
    }
    if (this->out_chan->pt.fracs > treat_as_null) {
        this->out_chan->pt.fracs = 0.0;
        this->out_chan->pt.tt += 1;
    }
    auto start_time = this->in_chan->pt;
    // will be equal or less; integer division rounds always down
    start_time.tt = (start_time.tt / grid_raster) * grid_raster;
    if (start_time < this->in_chan->pt) start_time.tt += grid_raster;

}
