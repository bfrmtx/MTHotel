#include "cal_plotter.h"

#include "qcustomplot.h"
#include "read_cal_xml.h"

#include <QAction>
#include <QColor>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMenuBar>
#include <QMimeData>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTextStream>
#include <QThread>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {
constexpr int calibrationIndexRole = Qt::UserRole;

QColor colorForSeries(const int index) {
  return QColor::fromHsv((index * 137) % 360, 180, 210);
}

QCPRange paddedLogRange(const double minimum, const double maximum) {
  return {minimum / 1.1, maximum * 1.1};
}

bool isPlottable(const calibration &source, const size_t index) {
  const bool validAmplitude = source.get_system().empty() ? source.a[index] > 0.0 : source.a[index] >= 0.0;
  return std::isfinite(source.f[index]) && std::isfinite(source.a[index]) && std::isfinite(source.p[index]) &&
         source.f[index] > 0.0 && validAmplitude;
}

QString calibrationName(const calibration &source) {
  if (!source.file_path_in.empty()) {
    const QString fileName = QString::fromStdString(source.file_path_in.filename().string());
    if (fileName.endsWith(".txt", Qt::CaseInsensitive) || fileName.endsWith(".xml", Qt::CaseInsensitive)) {
      return QString("%1 | %2, chopper %3")
          .arg(fileName, QString::fromStdString(source.get_sensor_and_serial()), QString::fromStdString(source.chopper2string()));
    }
    return fileName;
  }
  const std::string item = source.get_system().empty() ? source.get_sensor() : source.get_system();
  return QString("%1 (theoretical, chopper %2, DIV_%3)")
      .arg(QString::fromStdString(item), QString::fromStdString(source.chopper2string()))
      .arg(source.DIV);
}

struct GenerationResult {
  calibration data;
  QString error;
};

void calculateCalibration(calibration &source, const double minimum, const double maximum) {
  source.gen_freqs(minimum, maximum);
  source.gen_synthetic_cal({});
  source.theo_to_data();
  if (!source.get_system().empty()) {
    source.units_frequency = "Hz";
    source.units_amplitude = "1";
    source.units_phase = "degrees";
  } else {
    source.set_format(CalibrationType::mtx);
  }
  if (source.f.empty() || source.a.size() != source.f.size() || source.p.size() != source.f.size()) {
    throw std::runtime_error("Generated calibration vectors are empty or have inconsistent sizes");
  }
  for (size_t index = 0; index < source.f.size(); ++index) {
    if (!isPlottable(source, index)) {
      throw std::runtime_error("Generated calibration contains a non-finite or invalid value at index " + std::to_string(index));
    }
  }
}
} // namespace

CalPlotter::CalPlotter(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("Calibration Plot");
  setAcceptDrops(true);
  resize(1300, 850);

  auto *centralWidget = new QWidget(this);
  auto *layout = new QHBoxLayout(centralWidget);
  auto *sidebar = new QWidget(centralWidget);
  auto *sidebarLayout = new QVBoxLayout(sidebar);

  seriesTree = new QTreeWidget(sidebar);
  seriesTree->setHeaderLabel("Calibrations");
  sidebarLayout->addWidget(seriesTree, 1);

  auto *syntheticGroup = new QWidget(sidebar);
  auto *syntheticLayout = new QFormLayout(syntheticGroup);
  ItemSelector = new QComboBox(syntheticGroup);
  ItemSelector->setObjectName("itemSelector");
  ItemSelector->addItems({"MFS-06e", "MFS-07", "MFS-07e", "MFS-12e", "FGS-02", "FGS-03e", "FGS-05e", "SHFT-02e", "ADU-11e"});
  minimumFrequency = new QDoubleSpinBox(syntheticGroup);
  maximumFrequency = new QDoubleSpinBox(syntheticGroup);
  minimumFrequency->setObjectName("minimumFrequency");
  maximumFrequency->setObjectName("maximumFrequency");
  for (QDoubleSpinBox *spinBox : {minimumFrequency, maximumFrequency}) {
    spinBox->setDecimals(6);
    spinBox->setRange(0.000001, 1.0e9);
    spinBox->setSuffix(" Hz");
  }
  minimumFrequency->setValue(1);
  maximumFrequency->setValue(10000.0);
  syntheticLayout->addRow("Item", ItemSelector);
  syntheticLayout->addRow("f_min", minimumFrequency);
  syntheticLayout->addRow("f_max", maximumFrequency);

  resistance = new QDoubleSpinBox(syntheticGroup);
  resistance->setObjectName("resistance");
  resistance->setDecimals(3);
  resistance->setRange(0.0, 1.0e9);
  resistance->setValue(1000.0);
  capacitance = new QDoubleSpinBox(syntheticGroup);
  capacitance->setObjectName("capacitance");
  capacitance->setDecimals(6);
  capacitance->setRange(0.0, 1.0e6);
  capacitance->setValue(0.1);
  outputDataRate = new QDoubleSpinBox(syntheticGroup);
  outputDataRate->setObjectName("outputDataRate");
  outputDataRate->setDecimals(6);
  outputDataRate->setRange(0.000001, 1.0e9);
  outputDataRate->setValue(32768.0);
  syntheticLayout->addRow(QStringLiteral("R [\u03a9]"), resistance);
  syntheticLayout->addRow("C [nF]", capacitance);
  syntheticLayout->addRow("ODR [Hz]", outputDataRate);

  divisionButton = new QPushButton("DIV_1", syntheticGroup);
  divisionButton->setObjectName("divisionButton");
  divisionButton->setCheckable(true);
  connect(divisionButton, &QPushButton::toggled, this, [this](const bool checked) {
    divisionButton->setText(checked ? "DIV_8" : "DIV_1");
  });
  chopperButton = new QPushButton("Chopper on", syntheticGroup);
  chopperButton->setObjectName("chopperButton");
  chopperButton->setCheckable(true);
  chopperButton->setChecked(true);
  connect(chopperButton, &QPushButton::toggled, this, [this](const bool checked) {
    chopperButton->setText(checked ? "Chopper on" : "Chopper off");
  });
  syntheticLayout->addRow("Input divider", divisionButton);
  auto *chopperHint = new QLabel("set first for xml ot txt", syntheticGroup);
  chopperHint->setStyleSheet("color: red;");
  syntheticLayout->addRow(chopperHint);
  syntheticLayout->addRow("Chopper", chopperButton);

  generateButton = new QPushButton("Generate synthetic calibration", syntheticGroup);
  generateButton->setObjectName("generateButton");
  connect(generateButton, &QPushButton::clicked, this, &CalPlotter::generateCalibration);
  syntheticLayout->addRow(generateButton);
  sidebarLayout->addWidget(new QLabel("Synthetic calibration", sidebar));
  sidebarLayout->addWidget(syntheticGroup);

  auto *plotsWidget = new QWidget(centralWidget);
  auto *plotsLayout = new QVBoxLayout(plotsWidget);
  amplitudePlot = new QCustomPlot(plotsWidget);
  phasePlot = new QCustomPlot(plotsWidget);
  for (QCustomPlot *plot : {amplitudePlot, phasePlot}) {
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    plot->axisRect()->setRangeDrag(Qt::Horizontal);
    plot->axisRect()->setRangeZoom(Qt::Horizontal);
    plot->xAxis->setScaleType(QCPAxis::stLogarithmic);
    plot->xAxis->setTicker(QSharedPointer<QCPAxisTickerLog>::create());
    plot->xAxis->setLabel("Frequency (Hz)");
    plot->legend->setVisible(true);
    plotsLayout->addWidget(plot, 1);
  }
  amplitudePlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  amplitudePlot->yAxis->setLabel("Amplitude (mV/nT)");
  phasePlot->yAxis->setLabel("Phase (degrees)");

  connect(amplitudePlot->xAxis, qOverload<const QCPRange &>(&QCPAxis::rangeChanged), this, [this](const QCPRange &range) {
    phasePlot->xAxis->setRange(range);
    phasePlot->replot(QCustomPlot::rpQueuedReplot);
  });
  connect(phasePlot->xAxis, qOverload<const QCPRange &>(&QCPAxis::rangeChanged), this, [this](const QCPRange &range) {
    amplitudePlot->xAxis->setRange(range);
    amplitudePlot->replot(QCustomPlot::rpQueuedReplot);
  });
  connect(seriesTree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item) { updateVisibility(item); });

  layout->addWidget(sidebar, 1);
  layout->addWidget(plotsWidget, 4);
  setCentralWidget(centralWidget);

  auto *fileMenu = menuBar()->addMenu("&File");
  auto *openAction = fileMenu->addAction("&Open calibration files...");
  auto *clearAction = fileMenu->addAction("&Clear");
  connect(openAction, &QAction::triggered, this, [this] {
    loadFiles(QFileDialog::getOpenFileNames(this, "Open calibration files", {}, "Calibration files (*.json *.txt *.xml)"));
  });
  connect(clearAction, &QAction::triggered, this, [this] {
    ++generationVersion;
    calibrations.clear();
    theoretical = calibration();
    updateTree();
    rebuildPlots();
    statusBar()->showMessage("Cleared all calibrations");
  });

  auto *exportMenu = menuBar()->addMenu("&Export");
  auto *jsonAction = exportMenu->addAction("JSON");
  auto *csvAction = exportMenu->addAction("CSV");
  jsonAction->setObjectName("exportJsonAction");
  csvAction->setObjectName("exportCsvAction");
  connect(jsonAction, &QAction::triggered, this, [this] { exportCalibrations(ExportFormat::Json); });
  connect(csvAction, &QAction::triggered, this, [this] { exportCalibrations(ExportFormat::Csv); });

  statusBar()->showMessage("Drop calibration JSON, TXT or measurement XML files here or use File > Open calibration files...");
}

CalPlotter::~CalPlotter() {
  if (generationThread) {
    generationThread->wait();
  }
}

void CalPlotter::generateCalibration() {
  if (generationThread) {
    statusBar()->showMessage("A synthetic calibration is already being generated", 5000);
    return;
  }
  const double minimum = minimumFrequency->value();
  const double maximum = maximumFrequency->value();
  if (minimum >= maximum) {
    statusBar()->showMessage("f_min must be strictly less than f_max", 10000);
    return;
  }

  auto result = std::make_shared<GenerationResult>();
  try {
    calibration &request = result->data;
    request.set_item(ItemSelector->currentText().toStdString());
    request.set_capacitance(capacitance->value() * 1.0e-9);
    request.set_resistance(resistance->value());
    request.set_sampling_rate(outputDataRate->value());
    const ADU div = divisionButton->isChecked() ? ADU::div_8 : ADU::div_1;
    request.set_div(static_cast<int>(div));
    const ChopperStatus chopper = chopperButton->isChecked() ? ChopperStatus::on : ChopperStatus::off;
    request.set_int_chopper(static_cast<int>(chopper));
  } catch (const std::exception &error) {
    qWarning().noquote() << "Could not configure synthetic calibration:" << error.what();
    statusBar()->showMessage(QString("Could not configure synthetic calibration: %1").arg(QString::fromUtf8(error.what())), 10000);
    return;
  }

  const quint64 version = ++generationVersion;
  generationThread = QThread::create([result, minimum, maximum] {
    try {
      calculateCalibration(result->data, minimum, maximum);
    } catch (const std::exception &error) {
      result->error = QString::fromUtf8(error.what());
    }
  });
  generationThread->setParent(this);
  connect(generationThread, &QThread::finished, this, [this, result, version] {
    generationThread = nullptr;
    generateButton->setEnabled(true);
    if (version != generationVersion) {
      return;
    }
    if (!result->error.isEmpty()) {
      qWarning().noquote() << "Could not generate synthetic calibration:" << result->error;
      statusBar()->showMessage(QString("Could not generate synthetic calibration: %1").arg(result->error), 10000);
      return;
    }
    theoretical = std::move(result->data);
    appendCalibration(theoretical);
    updateTree();
    rebuildPlots();
    statusBar()->showMessage(QString("Generated %1").arg(calibrationName(theoretical)));
  });
  connect(generationThread, &QThread::finished, generationThread, &QObject::deleteLater);
  generateButton->setEnabled(false);
  statusBar()->showMessage("Generating synthetic calibration...");
  generationThread->start();
}

bool CalPlotter::isSystemMode() const {
  return !calibrations.isEmpty() && !calibrations.constFirst().get_system().empty();
}

void CalPlotter::appendCalibration(calibration source) {
  if (!source.get_system().empty() || isSystemMode()) {
    calibrations.clear();
    seriesTree->clear();
  }
  calibrations.append(std::move(source));
}

void CalPlotter::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

/// @brief handle drop event for calibration files, filters automatically *.txt, *.json, *.xml files
/// @param event the drop event containing the files
void CalPlotter::dropEvent(QDropEvent *event) {
  QStringList fileNames;
  for (const QUrl &url : event->mimeData()->urls()) {
    if (url.isLocalFile() && QFileInfo::exists(url.toLocalFile())) {
      if (url.toLocalFile().endsWith(".txt", Qt::CaseInsensitive) ||
          url.toLocalFile().endsWith(".json", Qt::CaseInsensitive) ||
          url.toLocalFile().endsWith(".xml", Qt::CaseInsensitive)) {
        fileNames.append(url.toLocalFile());
      }
    }
  }
  if (fileNames.isEmpty()) {
    statusBar()->showMessage("Drop local calibration JSON, TXT or measurement XML files", 5000);
    return;
  }

  loadFiles(fileNames);
  event->acceptProposedAction();
}

void CalPlotter::loadFiles(const QStringList &fileNames) {
  if (fileNames.isEmpty()) {
    return;
  }

  int loaded = 0;
  QStringList errors;
  for (const QString &fileName : fileNames) {
    QString error;
    if (loadFile(fileName, error)) {
      ++loaded;
    } else {
      errors.append(error);
    }
  }
  if (loaded > 0) {
    ++generationVersion;
    updateTree();
    rebuildPlots();
  }

  QString message = QString("Loaded %1 calibration file(s); %2 curve(s) available").arg(loaded).arg(calibrations.size());
  if (!errors.isEmpty()) {
    message += QString(" | %1").arg(errors.join(" | "));
  }
  statusBar()->showMessage(message, errors.isEmpty() ? 0 : 10000);
}

bool CalPlotter::loadFile(const QString &fileName, QString &error) {
  try {
    if (!QFileInfo(fileName).isFile()) {
      throw std::runtime_error("File does not exist or is not a regular file");
    }
    calibration source;
    QList<calibration> pending;
    const std::filesystem::path path(fileName.toStdString());
    const bool chopperOn = chopperButton->isChecked();
    const ChopperStatus chopper = chopperOn ? ChopperStatus::on : ChopperStatus::off;
    if (fileName.endsWith(".json", Qt::CaseInsensitive)) {
      source.read_file(path);
      pending.append(std::move(source));
    } else if (fileName.endsWith(".txt", Qt::CaseInsensitive)) {
      read_cal reader;
      const auto entry = reader.read_std_mtx_txt(path, chopper);
      if (!entry) {
        throw std::runtime_error("TXT reader returned no calibration");
      }
      source = *entry;
      if (source.is_chopper_on() != chopperOn) {
        throw std::runtime_error(std::string("No TXT calibration found for chopper ") + (chopperOn ? "on" : "off"));
      }
      pending.append(std::move(source));
    } else if (fileName.endsWith(".xml", Qt::CaseInsensitive)) {
      read_cal reader;
      std::string messages;
      const auto entries = reader.read_std_xml(path, messages);
      if (!messages.empty()) {
        qInfo().noquote() << QString::fromStdString(messages);
      }
      for (const auto &entry : entries) {
        if (!entry) {
          throw std::runtime_error("XML reader returned a null calibration");
        }
        if (entry->is_chopper_on() != chopperOn) {
          continue;
        }
        if (entry->f.empty() && entry->a.empty() && entry->p.empty()) {
          qInfo().noquote() << "Skipping metadata-only XML calibration"
                           << QString::fromStdString(entry->get_sensor_and_serial()) << "in" << fileName;
          continue;
        }
        source = *entry;
        pending.append(std::move(source));
      }
      if (pending.isEmpty()) {
        throw std::runtime_error(std::string("No plottable XML calibrations found for chopper ") + (chopperOn ? "on" : "off"));
      }
    } else {
      throw std::runtime_error("Unsupported file extension; expected .json, .txt or .xml");
    }

    for (calibration &entry : pending) {
      entry.file_path_in = path;
      entry.check_duplicates(path);
      bool hasPlottableData = false;
      for (size_t index = 0; index < entry.f.size(); ++index) {
        if (!std::isfinite(entry.f[index]) || !std::isfinite(entry.a[index]) || !std::isfinite(entry.p[index])) {
          throw std::runtime_error("Calibration contains a non-finite value at index " + std::to_string(index));
        }
        if (isPlottable(entry, index)) {
          hasPlottableData = true;
        }
      }
      if (!hasPlottableData) {
        throw std::runtime_error("Calibration has no finite positive frequency and amplitude values");
      }
    }

    for (calibration &entry : pending) {
      appendCalibration(std::move(entry));
    }
    return true;
  } catch (const std::exception &exception) {
    error = QString("Could not load %1: %2").arg(fileName, QString::fromUtf8(exception.what()));
    qWarning().noquote() << error;
    return false;
  }
}

void CalPlotter::exportCalibrations(const ExportFormat format) {
  if (calibrations.isEmpty()) {
    statusBar()->showMessage("No calibrations to export", 5000);
    return;
  }

  const QDir home(QDir::homePath());
  const QString extension = format == ExportFormat::Json ? "json" : "csv";
  qsizetype exported = 0;
  QStringList errors;
  for (const calibration &source : std::as_const(calibrations)) {
    try {
      QString item = QString::fromStdString(source.get_system().empty() ? source.get_sensor() : source.get_system());
      if (item.isEmpty()) {
        throw std::runtime_error("Calibration has no sensor or system name");
      }
      for (QChar &character : item) {
        if (character.unicode() < 32 || QStringLiteral("/\\:*?\"<>|").contains(character)) {
          character = '_';
        }
      }
      const QString baseName = QString("%1_%2_chopper_%3")
                                   .arg(item, QString::fromStdString(source.get_serial_string(4)),
                                        source.is_chopper_on() ? "on" : "off");
      QByteArray contents;
      if (format == ExportFormat::Json) {
        contents = QByteArray::fromStdString(source.toJson_embedd().dump(2)) + '\n';
      } else {
        QString csv;
        QTextStream stream(&csv);
        stream.setLocale(QLocale::c());
        stream.setRealNumberNotation(QTextStream::ScientificNotation);
        stream.setRealNumberPrecision(std::numeric_limits<double>::max_digits10);
        for (size_t index = 0; index < source.f.size(); ++index) {
          stream << source.f.at(index) << ',' << source.a.at(index) << ',' << source.p.at(index) << '\n';
        }
        stream.flush();
        contents = csv.toUtf8();
      }

      QFile file;
      for (quint64 suffix = 1;; ++suffix) {
        const QString name = baseName + (suffix == 1 ? QString() : QString("_%1").arg(suffix)) + '.' + extension;
        file.setFileName(home.filePath(name));
        if (file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
          break;
        }
        const QFileInfo existing(file.fileName());
        if (!existing.exists() && !existing.isSymLink()) {
          throw std::runtime_error(file.errorString().toStdString());
        }
      }
      if (file.write(contents) != contents.size() || !file.flush()) {
        QString error = file.errorString();
        if (!file.remove()) {
          error += QString("; could not remove incomplete file %1: %2").arg(file.fileName(), file.errorString());
        }
        throw std::runtime_error(error.toStdString());
      }
      file.close();
      ++exported;
    } catch (const std::exception &exception) {
      const QString error = QString("Could not export %1: %2")
                                .arg(calibrationName(source), QString::fromUtf8(exception.what()));
      qWarning().noquote() << error;
      errors.append(error);
    }
  }

  QString message = QString("Exported %1 of %2 calibration(s) as %3 to %4")
                        .arg(exported).arg(calibrations.size()).arg(extension.toUpper(), home.absolutePath());
  if (!errors.isEmpty()) {
    message += QString(" | %1").arg(errors.join(" | "));
  }
  statusBar()->showMessage(message, errors.isEmpty() ? 0 : 10000);
}

void CalPlotter::updateTree() {
  const QSignalBlocker blocker(seriesTree);
  while (seriesTree->topLevelItemCount() > calibrations.size()) {
    delete seriesTree->takeTopLevelItem(seriesTree->topLevelItemCount() - 1);
  }
  for (qsizetype index = seriesTree->topLevelItemCount(); index < calibrations.size(); ++index) {
    const calibration &entry = calibrations.at(index);
    const QString name = calibrationName(entry);
    auto *item = new QTreeWidgetItem(seriesTree, {name});
    const QString description = entry.file_path_in.empty()
                                    ? QString("%1\nf: %2 to %3 Hz\nR: %4 Ohm, C: %5 nF, ODR: %6 Hz")
                                          .arg(name)
                                          .arg(entry.f_min())
                                          .arg(entry.f_max())
                                          .arg(entry.get_resistance())
                                          .arg(entry.get_capacitance() * 1.0e9)
                                          .arg(entry.get_sampling_rate())
                                    : QString::fromStdString(entry.file_path_in.string());
    item->setToolTip(0, description);
    item->setData(0, calibrationIndexRole, index);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, Qt::Checked);
  }
}

void CalPlotter::rebuildPlots() {
  amplitudePlot->clearGraphs();
  phasePlot->clearGraphs();
  if (isSystemMode()) {
    amplitudePlot->yAxis->setScaleType(QCPAxis::stLinear);
    amplitudePlot->yAxis->setTicker(QSharedPointer<QCPAxisTicker>::create());
    amplitudePlot->yAxis->setLabel("Amplitude (gain)");
  } else {
    amplitudePlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
    amplitudePlot->yAxis->setTicker(QSharedPointer<QCPAxisTickerLog>::create());
    amplitudePlot->yAxis->setLabel("Amplitude (mV/nT)");
  }
  for (qsizetype index = 0; index < calibrations.size(); ++index) {
    const calibration &entry = calibrations.at(index);
    const QString name = calibrationName(entry);
    const bool visible = seriesTree->topLevelItem(index)->checkState(0) == Qt::Checked;
    const QColor color = colorForSeries(index);
    QCPGraph *amplitudeGraph = amplitudePlot->addGraph();
    amplitudeGraph->setData(entry.f, entry.a, true);
    amplitudeGraph->setName(name);
    amplitudeGraph->setPen(QPen(color, 1.5));
    amplitudeGraph->setVisible(visible);

    QCPGraph *phaseGraph = phasePlot->addGraph();
    phaseGraph->setData(entry.f, entry.p, true);
    phaseGraph->setName(name);
    phaseGraph->setPen(QPen(color, 1.5));
    phaseGraph->setVisible(visible);

    for (size_t sample = 0; sample < entry.f.size(); ++sample) {
      if (!isPlottable(entry, sample)) {
        amplitudeGraph->data()->remove(entry.f[sample]);
        phaseGraph->data()->remove(entry.f[sample]);
      }
    }
  }
  rescalePlots();
}

void CalPlotter::rescalePlots() {
  if (isSystemMode()) {
    amplitudePlot->yAxis->setRange(0.0, 1.0);
  } else {
    amplitudePlot->yAxis->rescale(true);
  }
  phasePlot->yAxis->rescale(true);
  rescaleFrequencyAxes();
  amplitudePlot->replot();
  phasePlot->replot();
}

void CalPlotter::updateVisibility(QTreeWidgetItem *item) {
  const int index = item->data(0, calibrationIndexRole).toInt();
  const bool visible = item->checkState(0) == Qt::Checked;
  amplitudePlot->graph(index)->setVisible(visible);
  phasePlot->graph(index)->setVisible(visible);
  rescalePlots();
}

void CalPlotter::rescaleFrequencyAxes() {
  double minimum = std::numeric_limits<double>::infinity();
  double maximum = 0.0;
  for (qsizetype index = 0; index < calibrations.size(); ++index) {
    const QCPGraph *graph = amplitudePlot->graph(index);
    if (!graph->visible()) {
      continue;
    }
    const calibration &entry = calibrations.at(index);
    auto [lower, upper] = entry.f_range();
    if (!isPlottable(entry, 0) || !isPlottable(entry, entry.f.size() - 1)) {
      bool foundRange = false;
      const QCPRange range = graph->getKeyRange(foundRange, QCP::sdPositive);
      if (!foundRange) {
        continue;
      }
      lower = range.lower;
      upper = range.upper;
    }
    minimum = std::min(minimum, lower);
    maximum = std::max(maximum, upper);
  }
  if (std::isfinite(minimum) && maximum > 0.0) {
    const QCPRange range = paddedLogRange(minimum, maximum);
    amplitudePlot->xAxis->setRange(range);
    phasePlot->xAxis->setRange(range);
  }
}
