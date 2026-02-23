
void load_from_rundir(const std::filesystem::path &top_dir) {
  std::unique_lock lock(spc_lock);
  if (!std::filesystem::exists(top_dir) || !std::filesystem::is_directory(top_dir)) {
    throw std::runtime_error("spc_base::load_from_rundir: directory " + top_dir.string() + " does not exist");
  }
  // we load all files with the correct extension, that is .datfa or .datfp
  // we prefer to load the binary files if they exist (then ignore the ascii files), if not we load the ascii files (no binary files existing, that is the logic)
  // we analyze the .bin* (or .dat*) files that way:
  // Hy_MFS-06e_0033_C003__Hy_MFS-06e_0033_C003_8192Hz (cross spectrum)
  // Hx_MFS-12e_0033_C002__Hx_MFS-12e_0033_C002_8192Hz.binfa are split at the "__" and finally separate the sampling rate info at the last "_" (cloud be Hz or s)
  // we get fist part: Hx_MFS-12e_0033_C002 and second part Hx_MFS-12e_0033_C002 and a third part 8192Hz (in case of auto spectra we still have two parts, but they are the same)
  // ChannelType_Sensor_SystemSerial_ChannelNumber
  // the must be one or more JSON files like 033_ADU-11e_C002_THx_8192Hz.json 033_ADU-11e_C003_THy_8192Hz.json
  // the corresponding JSON file must be found with:
  // SystemSerial_SystemType_ChannelNumber_ChannelType_SampleRate.json

  //
  std::vector<std::filesystem::path> data_files;
  std::vector<std::filesystem::path> json_files;
  this->bin_extension.clear();
  this->ascii_extension.clear();
  bool has_binary = false;
  // first scan all data files:
  for (const auto &entry : std::filesystem::directory_iterator(top_dir)) {
    if (entry.is_regular_file()) {
      auto path = entry.path();
      if (path.extension() == ".binfa") {
        this->bin_extension = "fa"; // frequency amplitude
        has_binary = true;
      } else if (path.extension() == ".binfc") {
        this->bin_extension = "fc"; // frequency complex
        has_binary = true;
      } else if (path.extension() == ".binfp") {
        this->bin_extension = "fp"; // frequency phase
        has_binary = true;
      } else if (path.extension() == ".binfap") {
        this->bin_extension = "fap"; // frequency amplitude phase
        has_binary = true;
      } else if (path.extension() == ".datfa") {
        this->ascii_extension = "fa"; // frequency amplitude
      } else if (path.extension() == ".datfc") {
        this->ascii_extension = "fc"; // frequency complex
      } else if (path.extension() == ".datfp") {
        this->ascii_extension = "fp"; // frequency phase
      } else if (path.extension() == ".datfap") {
        this->ascii_extension = "fap"; // frequency amplitude phase
      } else {
        continue; // skip files with other extensions
      }
    }
  }
  // now we have the data files, we can load them, we prefer the binary files if they exist, otherwise we load the ascii files
  if (has_binary) {
    for (const auto &entry : std::filesystem::directory_iterator(top_dir)) {
      if (entry.is_regular_file() && entry.path().extension() == (".bin" + this->bin_extension)) {
        data_files.push_back(entry.path());
      }
    }
  } else {
    for (const auto &entry : std::filesystem::directory_iterator(top_dir)) {
      if (entry.is_regular_file() && entry.path().extension() == (".dat" + this->ascii_extension)) {
        data_files.push_back(entry.path());
      }
    }
  }
  // now check if we have the according JSON files!
  for (const auto data_file : data_files) {
    std::string json_x, json_y, json_sr;
    std::filesystem::path json_file = data_file;
    json_file.replace_extension(".json");
    // split the filename at the "__" to get the channel names and sampling rate info
    auto filename = data_file.stem().string();                                                          // get the filename without extension
    json_x = filename.substr(0, filename.find("__"));                                                   // get the first part before "__"
    json_y = filename.substr(filename.find("__") + 2, filename.rfind("_") - (filename.find("__") + 2)); // get the second part between "__" and the last "_"
    json_sr = filename.substr(filename.rfind("_") + 1);                                                 // get the sampling rate info after the last "_"
  }
}
