#include "pstplotter.h"
#include "ui_pstplotter.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMetaObject>
#include <QMimeData>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <exception>
#include <set>

pstplotter::pstplotter(QWidget *parent) : QMainWindow(parent), ui(new Ui::pstplotter) {
  this->ui->setupUi(this);
  const auto thread_count = std::max(2u, std::thread::hardware_concurrency());
  this->pool = std::make_shared<BS::thread_pool<BS::tp::none>>(thread_count);
  this->spectra_plot = new pst_spectra_plot(this->ui->tab_stacked_spectra);
  this->ui->stacked_spectra_layout->addWidget(this->spectra_plot);
  this->ui->status_bar->showMessage("Drop or open a survey");
}

pstplotter::~pstplotter() {
  this->closing = true;
  ++this->request_generation;
  if (this->coordinator.joinable()) {
    this->coordinator.join();
  }
  this->pool->wait();
  delete this->ui;
}

void pstplotter::dragEnterEvent(QDragEnterEvent *event) {
  if (!this->processing && event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void pstplotter::dropEvent(QDropEvent *event) {
  if (this->processing) {
    return;
  }
  const auto urls = event->mimeData()->urls();
  for (const auto &url : urls) {
    const QFileInfo info(url.toLocalFile());
    if (info.isDir() && is_survey_dir(info.absoluteFilePath().toStdString())) {
      this->open_survey(info.absoluteFilePath());
      event->acceptProposedAction();
      return;
    }
  }

  const auto json_files = normalize_atss_json(urls);
  if (json_files.empty()) {
    this->ui->status_bar->showMessage("Drop a survey or ATSS/JSON stream files", 5000);
    return;
  }
  this->open_channels(json_files);
  event->acceptProposedAction();
}

bool pstplotter::is_survey_dir(const std::filesystem::path &directory) {
  const auto stations = directory / "stations";
  if (!std::filesystem::is_directory(stations)) {
    return false;
  }
  for (const auto &entry : std::filesystem::directory_iterator(stations)) {
    if (entry.is_directory()) {
      return true;
    }
  }
  return false;
}

std::vector<std::filesystem::path> pstplotter::normalize_atss_json(const QList<QUrl> &urls) {
  std::set<std::filesystem::path> json_files;
  for (const auto &url : urls) {
    const QFileInfo info(url.toLocalFile());
    if (!info.isFile()) {
      continue;
    }

    auto path = std::filesystem::path(info.absoluteFilePath().toStdString());
    const auto extension = path.extension().string();
    if (QString::fromStdString(extension).compare(".atss", Qt::CaseInsensitive) == 0) {
      path.replace_extension(".json");
    } else if (QString::fromStdString(extension).compare(".json", Qt::CaseInsensitive) != 0) {
      continue;
    }
    if (std::filesystem::is_regular_file(path)) {
      json_files.insert(std::move(path));
    }
  }
  return {json_files.begin(), json_files.end()};
}

void pstplotter::on_actionOpen_Survey_triggered() {
  const QString directory = QFileDialog::getExistingDirectory(this, "Open Survey", QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (!directory.isEmpty()) {
    this->open_survey(directory);
  }
}

void pstplotter::open_survey(const QString &survey_directory) {
  if (!is_survey_dir(survey_directory.toStdString())) {
    this->fail_processing(this->request_generation, "The selected directory is not a survey");
    return;
  }
  if (this->survey_dock == nullptr) {
    this->survey_tree = new QtSurveyTree(this);
    this->survey_dock = new QDockWidget("Survey selection", this);
    this->survey_dock->setObjectName("survey_dock");
    this->survey_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->survey_dock->setWidget(this->survey_tree);
    this->addDockWidget(Qt::LeftDockWidgetArea, this->survey_dock);
    connect(this->survey_tree, &QtSurveyTree::channelsSelected, this, [this](const auto &channels) {
      this->ui->button_plot_selected->setEnabled(!this->processing && !channels.empty());
    });
  }

  try {
    this->survey_tree->set_survey_dir(survey_directory.toStdString(), this->pool);
  } catch (const std::exception &error) {
    this->fail_processing(this->request_generation, QString::fromStdString(error.what()));
    return;
  }
  this->survey_dock->show();
  this->survey_dock->raise();
  this->ui->button_plot_selected->setEnabled(false);
  this->ui->status_bar->showMessage("Select matching PST runs and magnetic sensors");
}

void pstplotter::open_channels(const std::vector<std::filesystem::path> &json_files) {
  try {
    std::vector<std::shared_ptr<channel>> channels;
    channels.reserve(json_files.size());
    for (const auto &json_file : json_files) {
      channels.push_back(std::make_shared<channel>(json_file));
    }
    this->process_channels(std::move(channels));
  } catch (const std::exception &error) {
    this->fail_processing(this->request_generation, QString::fromStdString(error.what()));
  }
}

void pstplotter::on_button_plot_selected_clicked() {
  if (this->processing || this->survey_tree == nullptr) {
    return;
  }
  auto channels = this->survey_tree->getSelectedChannels();
  if (channels.empty()) {
    this->fail_processing(this->request_generation, "Select channels in the survey tree first");
    return;
  }
  this->process_channels(std::move(channels));
}

void pstplotter::process_channels(std::vector<std::shared_ptr<channel>> channels) {
  if (channels.empty()) {
    return;
  }
  if (this->coordinator.joinable()) {
    this->coordinator.join();
  }

  const uint64_t request = ++this->request_generation;
  this->set_processing(true);
  this->ui->status_bar->showMessage(QString("Processing %1 selected channels...").arg(channels.size()));
  this->coordinator = std::jthread([this, request, channels = std::move(channels)]() mutable {
    try {
      pst_spectra_processor processor(this->pool);
      auto spectra = processor.process(channels);
      if (this->closing || request != this->request_generation) {
        return;
      }
      QMetaObject::invokeMethod(this, [this, request, spectra = std::move(spectra)]() mutable { this->complete_processing(request, std::move(spectra)); }, Qt::QueuedConnection);
    } catch (const std::exception &error) {
      if (this->closing || request != this->request_generation) {
        return;
      }
      const QString message = QString::fromStdString(error.what());
      QMetaObject::invokeMethod(this, [this, request, message]() { this->fail_processing(request, message); }, Qt::QueuedConnection);
    }
  });
}

void pstplotter::set_processing(const bool active) {
  this->processing = active;
  this->ui->actionOpen_Survey->setEnabled(!active);
  this->ui->button_plot_selected->setEnabled(!active && this->survey_tree != nullptr && !this->survey_tree->getSelectedChannels().empty());
  this->ui->button_plot_selected->setText(active ? "Processing..." : "Plot selected");
  if (active) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
  } else {
    QApplication::restoreOverrideCursor();
  }
}

void pstplotter::complete_processing(const uint64_t request, std::vector<pst_sensor_spectrum> spectra) {
  if (request != this->request_generation) {
    return;
  }
  this->spectra_plot->set_spectra(spectra);
  this->set_processing(false);
  this->ui->tabs->setCurrentWidget(this->ui->tab_stacked_spectra);
  this->ui->status_bar->showMessage(QString("Plotted %1 sensors").arg(spectra.size()));
}

void pstplotter::fail_processing(const uint64_t request, const QString &message) {
  if (request != this->request_generation) {
    return;
  }
  if (this->processing) {
    this->set_processing(false);
  }
  this->ui->status_bar->showMessage(message, 10000);
  QMessageBox::warning(this, "PST plotter", message);
}
