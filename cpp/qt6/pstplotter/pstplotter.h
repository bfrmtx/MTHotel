#ifndef PSTPLOTTER_H
#define PSTPLOTTER_H

#include "BS_thread_pool.hpp"
#include "pst_spectra.hpp"
#include "pst_spectra_plot.h"
#include "qt_survey_tree.h"

#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMainWindow>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <thread>
#include <vector>

QT_BEGIN_NAMESPACE
namespace Ui {
class pstplotter;
}
QT_END_NAMESPACE

class pstplotter : public QMainWindow {
  Q_OBJECT

public:
  explicit pstplotter(QWidget *parent = nullptr);
  ~pstplotter() override;

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private slots:
  void on_actionOpen_Survey_triggered();
  void on_button_plot_selected_clicked();

private:
  static bool is_survey_dir(const std::filesystem::path &directory);
  static std::vector<std::filesystem::path> normalize_atss_json(const QList<QUrl> &urls);
  void open_survey(const QString &survey_directory);
  void open_channels(const std::vector<std::filesystem::path> &json_files);
  void process_channels(std::vector<std::shared_ptr<channel>> channels);
  void set_processing(bool active);
  void complete_processing(uint64_t request, std::vector<pst_sensor_spectrum> spectra);
  void fail_processing(uint64_t request, const QString &message);

  Ui::pstplotter *ui;
  pst_spectra_plot *spectra_plot = nullptr;
  QDockWidget *survey_dock = nullptr;
  QtSurveyTree *survey_tree = nullptr;
  std::shared_ptr<BS::thread_pool<BS::tp::none>> pool;
  std::jthread coordinator;
  std::atomic<uint64_t> request_generation{0};
  std::atomic_bool closing{false};
  bool processing = false;
};

#endif // PSTPLOTTER_H
