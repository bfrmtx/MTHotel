#ifndef PTSPC_LIB_HPP
#define PTSPC_LIB_HPP

#include "channel.hpp"
#include "freqs.hpp"
#include "mt_base.hpp"
#include <algorithm>
#include <chrono>
#include <complex>
#include <fftw3.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <survey_tree.hpp>
#include <utility>
#include <vector>

#include "about_system.hpp"
#include "cal_get_sql.hpp"
#include "channel_collector.hpp"
#include "gnuplotter.hpp"
#include "merge_abs_spectra.hpp"
#include "mini_math.hpp"
#include "raw_spectra.hpp"
#include "sqlite_handler.hpp"
#include "strings_etc.hpp"
#include "survey_tree.hpp"
#include "survey_tree_cmdline.hpp"
#include "vector_math.hpp"

class ptspc_lib {
public:
  // ptspc_lib(std::shared_ptr<survey_tree> survey,
  //           std::vector<std::string> stations_names,
  //           const std::vector<size_t> run_numbers, const std::filesystem::path &exePath);
  ptspc_lib(std::shared_ptr<BS::thread_pool<BS::tp::none>> pool_in);
  ~ptspc_lib() = default;
  // ptspc_lib(const ptspc_lib &) = delete;

private:
  std::shared_ptr<survey_tree_d> survey;                //!< shared pointer to the survey
  std::shared_ptr<survey_tree_d> survey_tmp;            //!< shared pointer to the survey
  std::filesystem::path survey_path;                    //!< name of the survey
  std::vector<std::shared_ptr<survey_tree_d>> stations; //!< vector of shared pointers to stations (children of survey)
  std::vector<std::shared_ptr<run_d>> runs;             //!< vector of shared pointers to ALL runs
  std::vector<std::shared_ptr<channel>> channels;       //!< vector of shared pointers to ALL channels
  // std::vector<size_t> run_numbers;                                           //!< vector of run numbers
  // std::vector<std::string> channel_types;                                    //!< vector of channel types
  std::vector<std::pair<std::string, std::string>> auto_cross_spectra_names; //!< vector of auto and cross spectra names
  std::vector<std::shared_ptr<fftw_freqs>> tmp_fft_freqs;                    //!< vector of fftw_freqs for labels
  std::vector<station_config> station_configs;                               //!< vector of station configurations

  // variables for options
  bool same_base = false; // // always compare against first RMS, default no (outer), yes likely for inner f range
  bool inner_range = false;
  bool dump = false;
  std::shared_ptr<BS::thread_pool<BS::tp::none>> pool; //!< thread pool for parallel processing
  // try to make a stable reference by dividing by E in general
  std::string ref_channel;

  double median_limit = 0.5;                  //!< for median limit
  bool lowres = false;                        //!< low resolution plot only
  bool highres = false;                       //!< high resolution plot only
  bool normalize = false;                     //!< normalize the calibration amplitude by f (old style)
  bool smooth = false;                        //!< smooth the spectra with a running average
  bool bcross_spectra = false;                //!< calculate cross spectra
  bool all_auto_spectra = false;              //!< calculate all auto spectra (HxHx, HyHy, ExEx, EyEy, ...) for same channel
  std::pair<double, double> f_range = {0, 0}; //!< frequency range
  std::pair<double, double> a_range = {0, 0}; //!< amplitude range
  std::pair<double, double> p_range = {0, 0}; //!< phase range
  double parzen_radius = 0.1;                 //!< parzen radius
  bool magnify_06e = false;                   //!< take a smaller subset of the frequency data for MFS-06e
  bool magnify_07e = false;                   //!< take a smaller subset of the frequency data for MFS-07e
  bool no_cal_plot = true;                    //!< skip the calibration plots at the end, and do NOT calibrate the data
  bool syscal = false;                        //!< use the system calibration
  bool railway = false;                       //!< railway data 16 2/3 Hz
  double pwr_base = 0.0;                      //!< power line base frequency, if != 0, power lines are suppressed like 50, 100, 150 Hz and so on
  bool use_master_cal = false;                //!< use the master calibration for all channels in case we don't have a calibration inside the json file
  std::unique_ptr<get_from_master_cal> master_cal;
  size_t min_wl = 512; //!< minimum window length for fftw; typical frequency for PST
  size_t min_rl = 256; //!< minimum read length for fftw, if wl < min_wl, rl = min_rl -> padding
  std::vector<std::pair<double, double>> power_lines_ranges = {{12, 20}, {46, 54}, {146, 154}};
  bool br = false;      //!< prepare for runs, default no
  bool has_gui = false; //!< has a GUI, default no
  size_t wl;            //!< window length for fftw
  size_t rl;            //!< read length for fftw
  size_t thread_index;  //!< thread index for parallel processing

  // path variables
  fs::path ptspc_path;            //!< path to the ptspc CALLING executable
  fs::path sqlfile;               //!< path to the SQL file for calibration data
  fs::path master_cal_db;         //!< path to the master calibration database
  inner_outer<double> innerouter; //!< inner and outer range for the resulting spectra, we do not want all frequencies

  // functions
public:
  void get_options(const std::list<std::string> &args, const bool &has_gui); //!< get the options from main command line
  void process_station_configs(const bool verbose = false);                  //!< process parsed station configurations
  void prepare_fft(const bool verbose = false);                              //!< prepare the fft for all channels
  void process_raw_spectra();                                                //!< process the spectra; this executes the fft
  void set_inner_outer_frequencies_prepare_spectra();                        //!< set the inner and outer frequencies for all channels / spectra  ; we don't want mostly NOT the complete, especially not the upper part. Then prepare the spectra (from queue of vectors to vector of vectors)

  void move_raw_spectra(); //!< move the raw spectra from channels to runs and raw_spectra objects

  void prepare_auto_spectra(const bool verbose = false); //!< prepare auto and cross spectra names

  void stack_spectra(); //!< stack the auto spectra for each channel type with fft freqs

  void save(const fs::path &top_dir_ = fs::path(), const std::string &sub_dir_ = ""); //!< save the ptspc_lib data to files in top_dir/sub_dir by creating survey !! below subdir with same structure as survey tree

  void
  collect_and_calibrate();            //!< the queue in the channel is (if) calibrated and transformed to e vector of vector complex; here the spectra will be finally moved into the runs and into a raw_spectra object
  void run_info_console();            //!< output the run information to the console
  void stack_ac_spectra();            //!< stack the auto cross spectra for each channel type with fft freqs
  void parzen_ac_coh_noise_spectra(); //!< create the parzen auto cross spectra for each channel type withtarget frequencies and coherence and noise spectra

  /*void dump_ac_spectra_coh_noise();   //!< dump the auto cross spectra, coherence and noise spectra to files

  void collect_channels();          //!< collect channels from the survey
  void create_auto_cross_spectra(); //!< create auto and cross spectra
  void plot_spectra();              //!< plot the spectra using gnuplotter

  void dump_spectra();             //!< dump the spectra to files
  void calculate_rms();            //!< calculate the RMS of the spectra
  void create_parzen_spectra();    //!< create parzen spectra
  void plot_parzen_spectra();      //!< plot the parzen spectra
  void create_noise_spectra();     //!< create noise spectra
  void plot_noise_spectra();       //!< plot the noise spectra
  void create_coherence_spectra(); //!< create coherence spectra
  void plot_coherence_spectra();   //!< plot the coherence spectra
  void create_calibration_plots(); //!< create calibration plots
  void plot_calibration_plots();   //!< plot the calibration plots
  void create_fft_freqs();         //!< create fft frequencies
  void plot_fft_freqs();           //!< plot the fft frequencies
  void create_survey_tree();       //!< create the survey tree
  void scan_survey();              //!< scan the survey for stations and runs
  void create_survey_dirs();       //!< create the survey directories
  */
};

#endif // PTSPC_LIB_HPP
