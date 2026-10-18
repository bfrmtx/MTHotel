#include "ats2atss.hpp"

#include <chrono>
#include <limits>

namespace {

void require(const bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

template <typename Function>
void require_rejected(Function function) {
  bool threw = false;
  try {
    function();
  } catch (const std::runtime_error &) {
    threw = true;
  }
  require(threw, "invalid OSR was accepted");
}

struct temporary_directory {
  std::filesystem::path path = std::filesystem::temp_directory_path() /
                               ("mthotel-channel-osr-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

  temporary_directory() {
    require(std::filesystem::create_directory(path), "could not create test directory");
  }

  ~temporary_directory() {
    std::error_code error;
    std::filesystem::remove_all(path, error);
    if (error)
      std::cerr << "Could not clean up test directory: " << error.message() << '\n';
  }
};

jsn read_json(const std::filesystem::path &path) {
  std::ifstream file(path);
  require(file.is_open(), "could not read test JSON");
  return jsn::parse(file);
}

void write_json(const std::filesystem::path &path, const jsn &head) {
  std::ofstream file(path);
  file << head;
  require(file.good(), "could not write test JSON");
}

void test_filter_and_setter() {
  channel chan;
  require(chan.get_osr() == 0.0, "default OSR is not unknown");
  chan.filter = "ADB-HF,div-1,gain_1_1,gain_2_1";
  require(chan.set_osr(4096.0) == 4096.0, "set_osr return value mismatch");
  require(chan.filter == "ADB-HF,div-1,gain_1_1,gain_2_1,osr_4096", "integer OSR token mismatch");
  chan.set_osr(4096.0);
  require(chan.filter == "ADB-HF,div-1,gain_1_1,gain_2_1,osr_4096", "OSR token duplicated");
  chan.set_osr(0.5);
  require(chan.filter.ends_with(",osr_0.5"), "fractional OSR was truncated");
  require(chan.filter.find("osr_4096") == std::string::npos, "old OSR token was retained");
  chan.set_sample_rate(0.125);
  require(chan.get_osr() == 0.5, "changing sample rate changed OSR");
  chan.set_filter();
  require(chan.get_osr() == 0.5 && chan.filter.ends_with(",osr_0.5"), "filter normalization dropped OSR");
  chan.filter = "ADB-HF,gain_1_1,gain_2_1,osr_4096";
  chan.set_filter();
  require(chan.get_osr() == 4096.0 && chan.filter.ends_with(",osr_4096"), "OSR token was not parsed");
  chan.reset_filters();
  require(chan.get_osr() == 4096.0 && chan.filter == "osr_4096", "resetting hardware filters lost OSR");
  chan.set_osr(0.0);
  require(chan.get_osr() == 0.0 && chan.filter.empty(), "unknown OSR token was retained");
  chan.set_filter();
  require(chan.filter.empty(), "empty filter normalization failed");
  chan.filter = "osr_0.5";
  chan.set_filter();
  require(chan.get_osr() == 0.5 && chan.filter == "osr_0.5", "OSR-only filter failed");
}

void test_invalid_osr() {
  channel chan;
  chan.set_osr(4096.0);
  for (const auto rate : {-1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
    require_rejected([&]() { chan.set_osr(rate); });
  require(chan.get_osr() == 4096.0 && chan.filter == "osr_4096", "invalid setter mutated OSR");
  for (const auto *token : {"osr_", "osr_invalid", "osr_4096junk", "osr_-1", "osr_nan", "osr_inf", "osr_1e999"}) {
    chan.filter = token;
    require_rejected([&]() { chan.set_filter(); });
  }
}

void test_divider_spellings(const std::filesystem::path &directory) {
  for (const auto *token : {"div-1", "div_1", "div-8", "div_8"}) {
    const bool divide_by_eight = std::string_view(token).back() == '8';
    const auto expected_divider = divide_by_eight ? ADU::div_8 : ADU::div_1;
    const std::string canonical_token = divide_by_eight ? "div-8" : "div-1";
    channel chan("Hx", 128.0);
    chan.set_system("ADU-08e");
    chan.filter = "ADB-LF," + std::string(token) + ",osr_4096";
    chan.set_filter();
    require(chan.input_divider == expected_divider, "channel did not parse divider spelling");
    require(chan.filter == "ADB-LF," + canonical_token + ",osr_4096", "divider spelling was not normalized");
    chan.set_filter();
    require(chan.input_divider == expected_divider && chan.filter == "ADB-LF," + canonical_token + ",osr_4096",
            "repeated normalization lost divider");
    chan.set_dir(directory);
    chan.write_data({1.0});
    chan.close_outfile();
    const auto path = chan.write_header();
    channel loaded(path);
    require(loaded.input_divider == expected_divider && loaded.filter == chan.filter, "JSON reload lost divider");
    auto head = read_json(path);
    head["filter"] = "ADB-LF," + std::string(token) + ",osr_4096";
    write_json(path, head);
    channel legacy(path);
    require(legacy.input_divider == expected_divider && legacy.filter == chan.filter, "legacy JSON divider was not normalized");

    for (const auto *system : {"ADU-08e", "ADU-07"}) {
      ADU radio = ADU::off, lp = ADU::off, hp = ADU::off, divider = ADU::off, gain1 = ADU::off, gain2 = ADU::off;
      xset_filter(system, token, radio, lp, hp, divider, gain1, gain2);
      require(divider == expected_divider, "xset_filter did not parse divider spelling");
    }
  }
}

void test_copy_and_move() {
  auto original = std::make_shared<channel>("Hx", 128.0);
  original->set_osr(4096.0);
  original->cal = std::make_shared<calibration>();
  channel copied(*original);
  channel pointer_copy(original);
  channel assigned;
  assigned = *original;
  channel moved(std::move(copied));
  channel move_assigned;
  move_assigned = std::move(assigned);
  for (const auto *chan : {&pointer_copy, &moved, &move_assigned})
    require(chan->get_osr() == 4096.0 && chan->filter == "osr_4096", "copy or move lost OSR");
  require(copied.get_osr() == 0.0 && assigned.get_osr() == 0.0, "moved-from OSR was not reset");
}

void test_json_round_trip(const std::filesystem::path &directory) {
  channel original("Hx", 128.0);
  original.set_system("ADU-08e");
  original.set_serial(84);
  original.set_channel_no(2);
  original.filter = "ADB-HF,gain_1_1,gain_2_1";
  original.set_osr(4096.0);
  original.set_dir(directory);
  original.write_data({1.0, 2.0});
  original.close_outfile();
  const auto path = original.write_header();
  auto head = read_json(path);
  require(!head.contains("osr"), "write_header emitted a standalone OSR entry");
  require(head["filter"] == "ADB-HF,gain_1_1,gain_2_1,osr_4096", "JSON filter shape mismatch");
  channel loaded(path);
  require(loaded.get_osr() == 4096.0 && loaded.filter == original.filter, "JSON round trip lost OSR");

  loaded.set_sample_rate(32.0);
  const auto saved = directory / "saved.json";
  loaded.save(saved);
  const auto saved_head = read_json(saved);
  require(!saved_head.contains("osr") && saved_head["filter"] == original.filter, "save or downsampling changed filter-only OSR");

  head["filter"] = "ADB-HF,gain_1_1,gain_2_1,osr_0.5";
  write_json(path, head);
  channel fractional(path);
  require(fractional.get_osr() == 0.5 && fractional.filter.ends_with(",osr_0.5"), "fractional OSR token was not read");

  head["filter"] = "ADB-HF,osr_4096";
  head["osr"] = 0.5;
  write_json(path, head);
  channel conflicting(path);
  require(conflicting.get_osr() == 4096.0 && conflicting.filter == "ADB-HF,osr_4096", "standalone OSR entry overrode the filter token");

  head.erase("osr");
  head["filter"] = "";
  write_json(path, head);
  channel legacy(path);
  require(legacy.get_osr() == 0.0 && legacy.filter.empty(), "old header without OSR was not supported");
}

void test_ats_conversion(const std::filesystem::path &directory, const std::string &system, const std::string &board) {
  ATSHeader_80 header{};
  header.header_length = sizeof(header);
  header.header_version = 80;
  header.samples = 2;
  header.sample_rate = 128.0F;
  header.orig_sample_rate = 4096.0F;
  header.lsbval = 0.5;
  header.serial_number = 84;
  header.channel_number = 2;
  header.gain_stage1 = 1.0F;
  header.gain_stage2 = 1.0F;
  std::copy_n("Hx", 2, header.channel_type);
  std::copy_n("MFS06e", 6, header.sensor_type);
  std::copy(system.begin(), system.end(), header.SystemType);
  std::copy(board.begin(), board.end(), header.ADB_board_type);
  std::copy_n("missing.xml", 11, header.comments.XmlHeader);
  const auto path = directory / "input.ats";
  {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(&header), sizeof(header));
    const int32_t samples[] = {2, 4};
    file.write(reinterpret_cast<const char *>(samples), sizeof(samples));
    require(file.good(), "could not write ATS fixture");
  }
  atsfile ats(path);
  require(ats.get_orig_sample_rate() == 4096.0, "ATS OSR getter mismatch");
  const bool is_adu11 = system == "ADU11" || system == "ADU11e";
  const std::string board_prefix = is_adu11 ? "" : "ADB-" + board + ",";
  require(ats.atsheader["SystemType"] == (is_adu11 ? "ADU-11e" : "ADU-08e"), "ATS system name was not normalized");
  require(ats.get_ats_filter("ADB-") == board_prefix + "div-1,gain_1_1,gain_2_1", "ATS board token or separators mismatch");
  ats.atsheader["LF_filters"] = "LF-RF-1";
  ats.atsheader["HF_filters"] = "HF-HP-1Hz";
  ats.atsheader["InputDivOn"] = 1;
  require(ats.get_ats_filter("ADB-") == board_prefix + "LF-RF-1,HF-HP-1Hz,div-8,gain_1_1,gain_2_1", "ATS filter tokens or separators changed");
  ats2atss conversion(path);
  auto chan = conversion.convert_header();
  require(chan->get_sample_rate() == 128.0 && chan->get_osr() == 4096.0, "ATS conversion used current rate as OSR");
  require(chan->filter.ends_with(",osr_4096"), "ATS conversion did not add OSR token");
  chan->set_dir(directory);
  const auto json_path = chan->write_header();
  ats2atss data_conversion(path, chan);
  data_conversion.convert_data();
  const auto head = read_json(json_path);
  require(!head.contains("osr") && head["filter"].get<std::string>().ends_with(",osr_4096"), "converted JSON did not use filter-only OSR");
  require(head["filter"] == board_prefix + "div-1,gain_1_1,gain_2_1,osr_4096", "converted JSON board token mismatch");
  channel loaded(json_path);
  require(loaded.get_osr() == 4096.0 && loaded.samples() == 2, "converted ATSS did not reload");
  require(loaded.read_all_at_once() == std::vector<double>({1.0, 2.0}), "ATS data conversion changed");
}

} // namespace

int main() {
  try {
    temporary_directory directory;
    test_filter_and_setter();
    test_invalid_osr();
    test_divider_spellings(directory.path);
    test_copy_and_move();
    test_json_round_trip(directory.path);
    test_ats_conversion(directory.path, "ADU08", "HF");
    test_ats_conversion(directory.path, "ADU08", "LF");
    test_ats_conversion(directory.path, "ADU11", "HF");
    test_ats_conversion(directory.path, "ADU11e", "LF");
    std::cout << "channel OSR tests passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
