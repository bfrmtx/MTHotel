#include "cal_plotter.h"
#include "qcustomplot.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QThread>
#include <QTreeWidget>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace {

void require(const bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

bool near(const double lhs, const double rhs) {
  return std::abs(lhs - rhs) <= 1.0e-10 * std::max({1.0, std::abs(lhs), std::abs(rhs)});
}

template <typename T>
T *control(CalPlotter &window, const char *name) {
  T *result = window.findChild<T *>(name);
  require(result != nullptr, "missing synthetic calibration control");
  return result;
}

struct Controls {
  QComboBox *item;
  QDoubleSpinBox *minimum;
  QDoubleSpinBox *maximum;
  QDoubleSpinBox *resistance;
  QDoubleSpinBox *capacitance;
  QDoubleSpinBox *odr;
  QPushButton *division;
  QPushButton *chopper;
  QPushButton *generate;
  QTreeWidget *tree;
  QCustomPlot *amplitude;
  QCustomPlot *phase;
  QAction *clear = nullptr;

  explicit Controls(CalPlotter &window) :
      item(control<QComboBox>(window, "itemSelector")),
      minimum(control<QDoubleSpinBox>(window, "minimumFrequency")),
      maximum(control<QDoubleSpinBox>(window, "maximumFrequency")),
      resistance(control<QDoubleSpinBox>(window, "resistance")),
      capacitance(control<QDoubleSpinBox>(window, "capacitance")),
      odr(control<QDoubleSpinBox>(window, "outputDataRate")),
      division(control<QPushButton>(window, "divisionButton")),
      chopper(control<QPushButton>(window, "chopperButton")),
      generate(control<QPushButton>(window, "generateButton")),
      tree(window.findChild<QTreeWidget *>()) {
    const auto plots = window.findChildren<QCustomPlot *>();
    require(tree && plots.size() == 2, "missing calibration views");
    amplitude = plots.at(0);
    phase = plots.at(1);
    for (QAction *action : window.findChildren<QAction *>()) {
      if (action->text() == "&Clear")
        clear = action;
    }
    require(clear != nullptr, "missing clear action");
  }

  void wait() const {
    QElapsedTimer timer;
    timer.start();
    while (!generate->isEnabled()) {
      require(timer.elapsed() < 10000, "synthetic calibration generation timed out");
      QApplication::processEvents(QEventLoop::AllEvents, 10);
      QThread::msleep(1);
    }
  }

  void run() const {
    generate->click();
    require(!generate->isEnabled(), "generation did not start asynchronously");
    wait();
  }

  void checkMode(const bool system, const int count) const {
    require(tree->topLevelItemCount() == count && amplitude->graphCount() == count && phase->graphCount() == count,
            "incorrect calibration count");
    require(amplitude->yAxis->scaleType() == (system ? QCPAxis::stLinear : QCPAxis::stLogarithmic),
            "incorrect amplitude scale");
    require(amplitude->xAxis->scaleType() == QCPAxis::stLogarithmic &&
                phase->xAxis->scaleType() == QCPAxis::stLogarithmic && phase->yAxis->scaleType() == QCPAxis::stLinear,
            "frequency/phase scale changed");
    if (system) {
      require(near(amplitude->yAxis->range().lower, 0.0) && near(amplitude->yAxis->range().upper, 1.0),
              "system amplitude range is not 0 to 1");
      require(amplitude->yAxis->label() == "Amplitude (gain)", "system gain has sensor units");
    } else {
      require(amplitude->yAxis->label() == "Amplitude (mV/nT)", "sensor amplitude units were not restored");
    }
  }
};

void check_transfer(const Controls &ui, const int index, const std::vector<double> &frequency,
                    const std::vector<std::complex<double>> &transfer) {
  QCPGraph *amplitude = ui.amplitude->graph(index);
  QCPGraph *phase = ui.phase->graph(index);
  require(static_cast<size_t>(amplitude->dataCount()) == frequency.size() &&
              static_cast<size_t>(phase->dataCount()) == frequency.size() && transfer.size() == frequency.size(),
          "generated calibration lost samples");
  auto amp = amplitude->data()->constBegin();
  auto phz = phase->data()->constBegin();
  for (size_t i = 0; i < frequency.size(); ++i, ++amp, ++phz) {
    require(near(amp->key, frequency[i]) && near(phz->key, frequency[i]) &&
                near(amp->value, std::abs(transfer[i])) && near(phz->value, std::arg(transfer[i]) * 180.0 / M_PI),
            "plotted calibration disagrees with the requested transfer function");
  }
}

void check_samples(const Controls &ui, const int index, const std::vector<double> &frequency,
                   const std::vector<double> &amplitude, const std::vector<double> &phase) {
  std::vector<std::complex<double>> transfer;
  for (size_t i = 0; i < frequency.size(); ++i)
    transfer.push_back(std::polar(amplitude[i], phase[i] * M_PI / 180.0));
  check_transfer(ui, index, frequency, transfer);
}

QString write_fixture(const QTemporaryDir &directory, const QString &name, const QByteArray &data) {
  const QString path = directory.filePath(name);
  QFile file(path);
  require(file.open(QIODevice::WriteOnly), "could not create import fixture");
  require(file.write(data) == data.size(), "could not write import fixture");
  return path;
}

QByteArray read_export(const QString &path) {
  QFile file(path);
  require(file.open(QIODevice::ReadOnly), "could not read exported file");
  return file.readAll();
}

void check_export(const QString &path, const calibration &expected, const bool json) {
  if (json) {
    const auto actual = nlohmann::ordered_json::parse(read_export(path).toStdString());
    const auto expectedJson = expected.toJson_embedd();
    for (const auto &[key, value] : expectedJson.at("sensor_calibration").items()) {
      if (actual.at("sensor_calibration").at(key) != value) {
        throw std::runtime_error("JSON export changed " + key + " in " + path.toStdString());
      }
    }
    require(actual == expectedJson, "JSON export changed data or metadata");
    require(actual.at("sensor_calibration").size() == 11, "comparison export added JSON fields");
    return;
  }
  const auto lines = read_export(path).trimmed().split('\n');
  require(static_cast<size_t>(lines.size()) == expected.f.size(), "CSV export lost calibration samples");
  for (qsizetype index = 0; index < lines.size(); ++index) {
    const auto columns = lines.at(index).split(',');
    require(columns.size() == 3, "CSV export does not have three columns");
    const double values[] = {expected.f.at(index), expected.a.at(index), expected.p.at(index)};
    for (int column = 0; column < 3; ++column) {
      bool valid = false;
      const double value = columns.at(column).toDouble(&valid);
      require(valid && value == values[column], "CSV export changed a stored numeric value");
    }
  }
}

void test_exports() {
  QTemporaryDir directory;
  require(directory.isValid(), "could not create export fixture directory");
  CalPlotter window;
  Controls ui(window);
  QAction *json = control<QAction>(window, "exportJsonAction");
  QAction *csv = control<QAction>(window, "exportCsvAction");
  const auto menus = window.menuBar()->actions();
  require(menus.size() >= 2 && menus.at(0)->text() == "&File" && menus.at(1)->text() == "&Export",
          "Export menu is not next to File");
  const QDir home(QDir::homePath());
  json->trigger();
  require(window.statusBar()->currentMessage().contains("No calibrations"), "empty export was not reported");
  require(home.entryList(QDir::Files).isEmpty(), "empty export created files");

  calibration sensor("MFS-06e", 123, ChopperStatus::on);
  sensor.f = {1.0, 2.0, 10.0};
  sensor.a = {0.0, 2.123456789012345, 3.0};
  sensor.p = {-10.0, 0.0, 20.0};
  const QString fixture = write_fixture(directory, "sensor_123.json",
                                        QByteArray::fromStdString(sensor.toJson_embedd().dump()));
  window.loadFiles({fixture, fixture});
  ui.tree->topLevelItem(0)->setCheckState(0, Qt::Unchecked);
  require(ui.amplitude->graph(0)->dataCount() == 2, "fixture did not exercise plot-only filtering");
  for (const bool asJson : {true, false}) {
    const QString extension = asJson ? "json" : "csv";
    const QString base = "MFS-06e_0123_chopper_on";
    const QString original = home.filePath(base + '.' + extension);
    QFile existing(original);
    require(existing.open(QIODevice::WriteOnly | QIODevice::NewOnly), "could not create collision fixture");
    require(existing.write("keep me") == 7, "could not write collision fixture");
    existing.close();
    (asJson ? json : csv)->trigger();
    require(read_export(original) == "keep me", "export overwrote an existing file");
    for (const int suffix : {2, 3}) {
      check_export(home.filePath(base + QString("_%1.").arg(suffix) + extension), sensor, asJson);
    }
    require(window.statusBar()->currentMessage().contains("Exported 2 of 2"),
            "export omitted a hidden curve or failed to report success");
  }
  require(ui.tree->topLevelItem(0)->checkState(0) == Qt::Unchecked &&
              !ui.amplitude->graph(0)->visible(),
          "export changed plot visibility");

  ui.clear->trigger();
  ui.run();
  calibration synthetic;
  synthetic.set_item("MFS-06e");
  synthetic.set_int_chopper(static_cast<int>(ChopperStatus::on));
  synthetic.gen_freqs(1.0, 10000.0);
  synthetic.gen_synthetic_cal({});
  synthetic.theo_to_data();
  synthetic.set_format(CalibrationType::mtx);
  json->trigger();
  csv->trigger();
  check_export(home.filePath("MFS-06e_0000_chopper_on.json"), synthetic, true);
  check_export(home.filePath("MFS-06e_0000_chopper_on.csv"), synthetic, false);

  ui.item->setCurrentText("ADU-11e");
  ui.resistance->setValue(5000.0);
  ui.capacitance->setValue(2.5);
  ui.odr->setValue(16384.0);
  ui.division->setChecked(true);
  ui.chopper->setChecked(false);
  ui.run();
  calibration system;
  system.set_item("ADU-11e");
  system.set_resistance(5000.0);
  system.set_capacitance(2.5e-9);
  system.set_sampling_rate(16384.0);
  system.set_div(8);
  system.set_int_chopper(static_cast<int>(ChopperStatus::off));
  system.gen_freqs(1.0, 10000.0);
  system.gen_synthetic_cal({});
  system.theo_to_data();
  system.units_frequency = "Hz";
  system.units_amplitude = "1";
  system.units_phase = "degrees";
  json->trigger();
  csv->trigger();
  const QString systemJson = home.filePath("ADU-11e_0000_chopper_off.json");
  check_export(systemJson, system, true);
  check_export(home.filePath("ADU-11e_0000_chopper_off.csv"), system, false);
  ui.checkMode(true, 1);
  check_samples(ui, 0, system.f, system.a, system.p);
  json->trigger();
  check_export(home.filePath("ADU-11e_0000_chopper_off_2.json"), system, true);

  calibration invalidSensor = sensor;
  invalidSensor.set_sensor(std::string(300, 'x'));
  const QString invalid = write_fixture(directory, "sensor_124.json",
                                        QByteArray::fromStdString(invalidSensor.toJson_embedd().dump()));
  window.loadFiles({invalid, fixture});
  json->trigger();
  require(window.statusBar()->currentMessage().contains("Exported 1 of 2") &&
              window.statusBar()->currentMessage().contains("Could not export"),
          "partial export failure was hidden or stopped other calibrations");
  check_export(home.filePath("MFS-06e_0123_chopper_on_4.json"), sensor, true);
}

QByteArray xml_fixture(const bool includeOff = true, const bool descendingLast = false) {
  QString xml = "<measurement><calibration_sensors>";
  for (int channel = 0; channel < 3; ++channel) {
    xml += QString("<channel id=\"%1\"><calibration><calibrated_item>"
                   "<ci>MFS-06e</ci><ci_serial_number>%2</ci_serial_number>"
                   "<ci_date>2026-10-18</ci_date><ci_time>12:34:56</ci_time>"
                   "</calibrated_item>").arg(channel).arg(111 + channel);
    for (const bool on : {true, false}) {
      if (!on && !includeOff)
        continue;
      for (int point = 0; point < 2; ++point) {
        const double frequency = (point == 0) != (descendingLast && channel == 2) ? 1.0 : 10.0;
        const double amplitude = (channel + 1) * (on ? (point + 1) * 0.001 : (point + 3) * 0.001);
        const double phase = (on ? 1 : -1) * (channel + 1) * (point + 1) * 10.0;
        xml += QString("<caldata chopper=\"%1\"><c1 unit=\"Hz\">%2</c1>"
                       "<c2 unit=\"V/(nT*Hz)\">%3</c2><c3 unit=\"deg\">%4</c3></caldata>")
                   .arg(on ? "on" : "off").arg(frequency).arg(amplitude).arg(phase);
      }
    }
    xml += "</calibration></channel>";
  }
  xml += "<channel id=\"3\"><calibration><calibrated_item>"
         "<ci>EFP-06</ci><ci_serial_number>999</ci_serial_number>"
         "<ci_date>2026-10-18</ci_date><ci_time>12:34:56</ci_time>"
         "</calibrated_item></calibration></channel></calibration_sensors></measurement>";
  return xml.toUtf8();
}

void test_file_imports(const QString &sample) {
  QTemporaryDir directory;
  require(directory.isValid(), "could not create import fixture directory");
  const QByteArray onText =
      "Calibration measurement with NumetriQ\n"
      "Magnetometer MFS06e#123 Date: 26/10/18 Time: 12:34:56\n"
      "FREQUENCY MAGNITUDE PHASE\nHz V/(nT*Hz) deg\nChopper On\n"
      "1 0.002 10\n10 0.003 20\n";
  const QString txt = write_fixture(directory, "MFS06e_123.TXT",
                                    onText + "\nFREQUENCY MAGNITUDE PHASE\nHz V/(nT*Hz) deg\n"
                                             "Chopper Off\n1 0.004 -10\n10 0.005 -20\n");
  const QString xml = write_fixture(directory, "measurement.XML", xml_fixture());
  const QString onOnlyXml = write_fixture(directory, "on_only.xml", xml_fixture(false));
  const QString invalidXml = write_fixture(directory, "unsorted.xml", xml_fixture(true, true));
  const QString onOnlyTxt = write_fixture(directory, "MFS06e_124.txt", onText);
  const QString unsupported = write_fixture(directory, "unsupported.bin", "not a calibration");
  const QString uppercaseJson = directory.filePath("sensor_123.JSON");
  require(QFile::copy(sample, uppercaseJson), "could not copy JSON fixture");

  {
    CalPlotter window;
    Controls ui(window);
    window.loadFiles({txt});
    ui.checkMode(false, 1);
    check_samples(ui, 0, {1.0, 10.0}, {2.0, 30.0}, {10.0, 20.0});
    require(ui.amplitude->graph(0)->name().contains("MFS-06e #123, chopper on") &&
                ui.tree->topLevelItem(0)->toolTip(0) == txt,
            "TXT sensor identity or input path was lost");

    ui.chopper->click();
    window.loadFiles({txt});
    ui.checkMode(false, 2);
    check_samples(ui, 1, {1.0, 10.0}, {4.0, 50.0}, {-10.0, -20.0});
    require(ui.amplitude->graph(1)->name().contains("chopper off"), "TXT ignored selected chopper state");
    window.loadFiles({uppercaseJson});
    ui.checkMode(false, 3);
    calibration expected;
    expected.read_file(std::filesystem::path(sample.toStdString()));
    require(expected.is_chopper_on(), "JSON test fixture must be chopper on");
    check_samples(ui, 2, expected.f, expected.a, expected.p);

    QCPGraph *original = ui.amplitude->graph(0);
    window.loadFiles({onOnlyTxt});
    require(ui.amplitude->graphCount() == 3 && ui.amplitude->graph(0) == original &&
                window.statusBar()->currentMessage().contains("chopper off"),
            "missing TXT chopper state changed plots or hid its error");
    window.loadFiles({unsupported, txt});
    ui.checkMode(false, 4);
    require(window.statusBar()->currentMessage().contains("Unsupported file extension"),
            "a successful import hid another file's error");
    window.loadFiles({directory.filePath("missing.xml")});
    ui.checkMode(false, 4);
    require(window.statusBar()->currentMessage().contains("does not exist"), "missing file error was hidden");
  }

  {
    CalPlotter window;
    Controls ui(window);
    window.loadFiles({xml});
    ui.checkMode(false, 3);
    for (int channel = 0; channel < 3; ++channel) {
      const double base = channel + 1;
      check_samples(ui, channel, {1.0, 10.0}, {base, 20.0 * base}, {10.0 * base, 20.0 * base});
      require(ui.amplitude->graph(channel)->name().contains(QString("#%1, chopper on").arg(111 + channel)) &&
                  ui.tree->topLevelItem(channel)->toolTip(0) == xml,
              "XML channel identity or input path was lost");
    }
    ui.tree->topLevelItem(0)->setCheckState(0, Qt::Unchecked);
    ui.chopper->click();
    window.loadFiles({xml});
    ui.checkMode(false, 6);
    require(!ui.amplitude->graph(0)->visible(), "XML append changed existing visibility");
    for (int channel = 0; channel < 3; ++channel) {
      const double base = channel + 1;
      check_samples(ui, channel + 3, {1.0, 10.0}, {3.0 * base, 40.0 * base}, {-10.0 * base, -20.0 * base});
      require(ui.amplitude->graph(channel + 3)->name().contains("chopper off"), "XML included the wrong chopper state");
    }
    QCPGraph *original = ui.amplitude->graph(0);
    window.loadFiles({onOnlyXml});
    require(ui.amplitude->graphCount() == 6 && ui.amplitude->graph(0) == original &&
                window.statusBar()->currentMessage().contains("chopper off"),
            "missing XML chopper state changed plots or hid its error");
    window.loadFiles({invalidXml});
    require(ui.amplitude->graphCount() == 6 && ui.amplitude->graph(0) == original &&
                window.statusBar()->currentMessage().contains("not increasing"),
            "invalid XML was partially appended or its error was hidden");
  }

  {
    CalPlotter window;
    Controls ui(window);
    ui.chopper->click();
    window.loadFiles({uppercaseJson, txt, xml});
    ui.checkMode(false, 5);
    require(window.statusBar()->currentMessage().contains("Loaded 3 calibration file(s)"),
            "mixed JSON/TXT/XML batch did not load all files");
  }
}

void test_generation_and_modes(const QString &sample) {
  CalPlotter window;
  Controls ui(window);
  require(near(ui.resistance->value(), 1000.0) && near(ui.capacitance->value(), 0.1) && near(ui.odr->value(), 32768.0),
          "incorrect R/C/ODR presets");
  require(!ui.division->isChecked() && ui.division->text() == "DIV_1" &&
              ui.chopper->isChecked() && ui.chopper->text() == "Chopper on",
          "incorrect divider/chopper defaults");
  ui.generate->click();
  require(!ui.generate->isEnabled(), "generation did not start asynchronously");
  ui.resistance->setValue(2000.0);
  ui.wait();
  ui.checkMode(false, 1);
  require(ui.tree->topLevelItem(0)->toolTip(0).contains("R: 1000") &&
              ui.tree->topLevelItem(0)->toolTip(0).contains("C: 0.1 nF"),
          "generation did not snapshot inputs or store capacitance in farads");
  calibration grid;
  grid.gen_freqs(1.0, 10000.0);
  check_transfer(ui, 0, grid.f, gen_trf_mfs06e(grid.f, ChopperStatus::on));

  ui.tree->topLevelItem(0)->setCheckState(0, Qt::Unchecked);
  ui.item->setCurrentText("MFS-07e");
  ui.chopper->click();
  ui.division->click();
  require(ui.division->text() == "DIV_8" && ui.chopper->text() == "Chopper off", "toggle labels were not updated");
  ui.minimum->setValue(0.1);
  ui.maximum->setValue(100.0);
  ui.run();
  ui.checkMode(false, 2);
  require(ui.tree->topLevelItem(0)->checkState(0) == Qt::Unchecked && !ui.amplitude->graph(0)->visible(),
          "appending a sensor lost previous visibility");
  require(ui.amplitude->graph(1)->name().contains("chopper off, DIV_8"), "divider/chopper metadata was not applied");
  grid.gen_freqs(0.1, 100.0);
  check_transfer(ui, 1, grid.f, gen_trf_mfs07e(grid.f, ChopperStatus::off));

  ui.item->setCurrentText("ADU-11e");
  ui.resistance->setValue(1000.0);
  ui.maximum->setValue(10000.0);
  ui.minimum->setValue(1.0);
  ui.run();
  ui.checkMode(true, 1);
  require(ui.amplitude->graph(0)->name().contains("ADU-11e"), "system did not replace the sensor tree entry");
  grid.gen_freqs(1.0, 10000.0);
  check_transfer(ui, 0, grid.f, gen_trf_ad_7771_lf(grid.f, 32768.0, 1000.0, 0.1));

  ui.resistance->setValue(5000.0);
  ui.capacitance->setValue(2.5);
  ui.odr->setValue(16384.0);
  ui.run();
  ui.checkMode(true, 1);
  check_transfer(ui, 0, grid.f, gen_trf_ad_7771_lf(grid.f, 16384.0, 5000.0, 2.5));

  ui.item->setCurrentText("MFS-06e");
  ui.run();
  ui.checkMode(false, 1);
  window.loadFiles({sample});
  ui.checkMode(false, 2);

  ui.item->setCurrentText("ADU-11e");
  ui.minimum->setValue(0.000001);
  ui.maximum->setValue(0.001);
  ui.run();
  ui.checkMode(true, 1);
  grid.gen_freqs(0.000001, 0.001);
  check_transfer(ui, 0, grid.f, gen_trf_ad_7771_lf(grid.f, 16384.0, 5000.0, 2.5));
  window.loadFiles({sample});
  ui.checkMode(false, 1);
  calibration loaded;
  loaded.read_file(std::filesystem::path(sample.toStdString()));
  std::vector<double> frequency;
  std::vector<std::complex<double>> transfer;
  loaded.get_cplx_cal(frequency, transfer);
  check_transfer(ui, 0, frequency, transfer);

  ui.amplitude->xAxis->setRange(0.2, 20.0);
  require(ui.phase->xAxis->range() == ui.amplitude->xAxis->range(), "amplitude-to-phase frequency sync failed");
  ui.phase->xAxis->setRange(1.0, 100.0);
  require(ui.phase->xAxis->range() == ui.amplitude->xAxis->range(), "phase-to-amplitude frequency sync failed");
}

void test_errors_and_lifetime(const QString &sample) {
  CalPlotter window;
  Controls ui(window);
  window.loadFiles({sample});
  QCPGraph *original = ui.amplitude->graph(0);
  ui.minimum->setValue(10.0);
  ui.maximum->setValue(10.0);
  ui.generate->click();
  require(ui.generate->isEnabled() && ui.amplitude->graph(0) == original &&
              window.statusBar()->currentMessage().contains("f_min"),
          "equal frequency bounds were not rejected without changing plots");
  ui.maximum->setValue(1.0);
  ui.generate->click();
  require(ui.generate->isEnabled() && ui.amplitude->graph(0) == original, "reversed frequency bounds were accepted");

  ui.item->setCurrentText("ADU-11e");
  ui.minimum->setValue(1.0);
  ui.maximum->setValue(10000.0);
  ui.capacitance->setValue(0.0);
  ui.run();
  require(ui.amplitude->graph(0) == original && window.statusBar()->currentMessage().contains("positive"),
          "failed generation changed plots or hid the model error");
  ui.checkMode(false, 1);

  ui.capacitance->setValue(0.1);
  ui.generate->click();
  require(!ui.generate->isEnabled(), "worker did not start");
  ui.clear->trigger();
  ui.wait();
  ui.checkMode(false, 0);

  ui.generate->click();
  require(!ui.generate->isEnabled(), "worker did not start after clear");
  window.loadFiles({sample});
  ui.wait();
  ui.checkMode(false, 1);
  require(!ui.amplitude->graph(0)->name().contains("theoretical"), "pending worker replaced a newly loaded file");

  auto closing = std::make_unique<CalPlotter>();
  control<QComboBox>(*closing, "itemSelector")->setCurrentText("ADU-11e");
  control<QPushButton>(*closing, "generateButton")->click();
  require(!control<QPushButton>(*closing, "generateButton")->isEnabled(), "shutdown worker did not start");
  closing.reset();
  QApplication::processEvents();
}

} // namespace

int main(int argc, char *argv[]) {
  try {
    QTemporaryDir home;
    require(home.isValid(), "could not create isolated export HOME");
    require(qputenv("HOME", QFile::encodeName(home.path())), "could not configure export HOME");
    QApplication application(argc, argv);
    require(QDir::homePath() == home.path(), "export tests would write outside the isolated HOME");
    const QString sample = QString::fromUtf8(MTHOTEL_TEST_SOURCE_DIR) +
                           QStringLiteral("/cpp/doc/new_survey_structure/Northern_Mining/stations/Sar\u0131\u00e7am/"
                                          "run_001/084_ADU-07e_C003_THy_128Hz.json");
    require(qputenv("MTHotel_data", QByteArray(MTHOTEL_TEST_SOURCE_DIR "/cpp/data")), "could not configure test sensor database");
    test_exports();
    test_file_imports(sample);
    test_generation_and_modes(sample);
    test_errors_and_lifetime(sample);
    std::cout << "cal-plot export, import and synthetic calibration tests passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
