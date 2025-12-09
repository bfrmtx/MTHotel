
size_t l = 0;
// Iterate through the arguments and process them
for (l = 0; l < margs.size(); ++l) {
  std::string marg = margs[l];
  if (marg == "-u") {
    if (++l >= margs.size())
      throw std::runtime_error("-u requires an argument");
    fs::path survey_name = margs[l];
    if (!fs::is_directory(survey_name)) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " -u survey_dir needs an existing directory with stations inside" << std::endl;
      err_str << " given: " << survey_name.string() << std::endl;
      throw std::runtime_error(err_str.str());
    }
    survey_name = fs::canonical(survey_name);
    survey = std::make_shared<survey_tree>(survey_name);
    if (survey == nullptr) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " first use -u survey_name in order to init the survey";
      throw std::runtime_error(err_str.str());
    }
    survey->scan();
  } else if (marg == "-s") {
    if (survey == nullptr) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " first use -u survey_name in order to init the survey";
      throw std::runtime_error(err_str.str());
    }
    // repeat until we find a - or the end of the arguments
    while ((l + 1) < margs.size() && !margs[l + 1].empty() && margs[l + 1][0] != '-') {
      std::string station_name = margs[++l];
      stations.emplace_back(survey->get_child(station_name));
      if (stations.back() == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " secondly use -s station_name [station names] in order to init station[s]";
        err_str << " station_name " << station_name << " not found in survey " << survey->get_path().string() << std::endl;
        stations.clear();
        throw std::runtime_error(err_str.str());
      }
    }
  } else if (marg == "-c") {
    while ((l + 1) < margs.size() && !margs[l + 1].empty() && margs[l + 1][0] != '-') {
      std::string channel_type = margs[++l];
      if (std::find(available_channel_types.begin(), available_channel_types.end(), channel_type) == available_channel_types.end()) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " channel type " << channel_type << " not available" << std::endl;
        throw std::runtime_error(err_str.str());
      }
      channel_types.emplace_back(channel_type);
      auto_cross_spectra_names.emplace_back(channel_type, channel_type);
    }
  } else if (marg == "-cx") {
    while ((l + 2) < margs.size() && !margs[l + 1].empty() && margs[l + 1][0] != '-') {
      std::string first_channel = margs[++l];
      if ((l + 1) >= margs.size())
        throw std::runtime_error("-cx requires two arguments");
      std::string second_channel = margs[++l];
      if (first_channel == second_channel) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " first and second channel must be different, you want cross spectra with -cx option" << std::endl;
        throw std::runtime_error(err_str.str());
      }
      std::string ac = first_channel + second_channel;
      if (std::find(available_ac_spectra_types.begin(), available_ac_spectra_types.end(), ac) == available_ac_spectra_types.end()) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " ac spectra type " << ac << " not available" << std::endl;
        throw std::runtime_error(err_str.str());
      }
      if ((std::find(channel_types.begin(), channel_types.end(), first_channel) == channel_types.end()) ||
          (std::find(channel_types.begin(), channel_types.end(), second_channel) == channel_types.end())) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " first and second channel must be in channel_types (the -c option)" << std::endl;
        throw std::runtime_error(err_str.str());
      }
      auto_cross_spectra_names.emplace_back(first_channel, second_channel);
    }
    bcross_spectra = true;
  } else if (marg == "-ref") {
    if (++l >= margs.size())
      throw std::runtime_error("-ref requires an argument");
    ref_channel = margs[l];

  } else if (marg == "-mc") {
    use_master_cal = true;
    master_cal = std::make_unique<get_from_master_cal>(master_cal_db);
  } else if (marg == "-ml") {
    if (++l >= margs.size())
      throw std::runtime_error("-ml requires an argument");
    median_limit = mstr::mystod(margs[l]);
    if (median_limit < 0.0 || median_limit > 1.0) {
      std::cout << " median limit must be between 0.0 and 1.0" << std::endl;
      throw std::runtime_error("median limit out of range");
    }
  } else if (marg == "-f_range") {
    if ((l + 2) >= margs.size())
      throw std::runtime_error("-f_range requires two arguments");
    f_range.first = mstr::mystod(margs[++l]);
    f_range.second = mstr::mystod(margs[++l]);
    if (f_range.first > f_range.second) {
      std::cout << " must be min max: f_range.first < f_range.second" << std::endl;
      throw std::runtime_error("frequency range invalid");
    }
    if (f_range.first <= 0) {
      std::cout << " f_range.first > 0 for log plot" << std::endl;
      throw std::runtime_error("frequency range invalid");
    }
  } else if (marg == "-a_range") {
    if ((l + 2) >= margs.size())
      throw std::runtime_error("-a_range requires two arguments");
    a_range.first = mstr::mystod(margs[++l]);
    a_range.second = mstr::mystod(margs[++l]);
    if (a_range.first > a_range.second) {
      std::cout << " must be min max: a_range.first < a_range.second" << std::endl;
      throw std::runtime_error("amplitude range invalid");
    }
    if (a_range.first <= 0) {
      std::cout << " a_range.first > 0 for log plot" << std::endl;
      throw std::runtime_error("amplitude range invalid");
    }
  } else if (marg == "-p_range") {
    if ((l + 2) >= margs.size())
      throw std::runtime_error("-p_range requires two arguments");
    p_range.first = mstr::mystod(margs[++l]);
    p_range.second = mstr::mystod(margs[++l]);
    if (p_range.first > p_range.second) {
      std::cout << " must be min max: p_range.first < p_range.second" << std::endl;
      throw std::runtime_error("phase range invalid");
    }
  } else if (marg == "-pl") {
    if (++l >= margs.size())
      throw std::runtime_error("-pl requires an argument");
    pwr_base = mstr::mystod(margs[l]);
    std::cout << "power lines suppressed >= " << pwr_base << " Hz" << std::endl;
  } else if (marg == "-") {
    std::cerr << "\nunrecognized option " << marg << std::endl;
    throw std::runtime_error("unrecognized option: " + marg);
  } else if (!marg.empty() && marg[0] == '-') {
    std::cerr << "\nunrecognized option " << marg << std::endl;
    throw std::runtime_error("unrecognized option: " + marg);
  }
  // else: skip, already incremented l
}