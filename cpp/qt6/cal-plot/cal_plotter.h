#pragma once

#include "cal_base.hpp"

#include <QList>
#include <QMainWindow>

class QComboBox;
class QDoubleSpinBox;
class QDragEnterEvent;
class QDropEvent;
class QCustomPlot;
class QPushButton;
class QThread;
class QTreeWidget;
class QTreeWidgetItem;

class CalPlotter final : public QMainWindow {
  Q_OBJECT

public:
  explicit CalPlotter(QWidget *parent = nullptr);
  ~CalPlotter() override;

  void loadFiles(const QStringList &fileNames);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  enum class ExportFormat { Json, Csv };

  bool loadFile(const QString &fileName, QString &error);
  void exportCalibrations(ExportFormat format);
  void generateCalibration();
  void appendCalibration(calibration source);
  bool isSystemMode() const;
  void updateTree();
  void rebuildPlots();
  void updateVisibility(QTreeWidgetItem *item);
  void rescalePlots();
  void rescaleFrequencyAxes();

  QCustomPlot *amplitudePlot = nullptr;
  QCustomPlot *phasePlot = nullptr;
  QTreeWidget *seriesTree = nullptr;
  QComboBox *ItemSelector = nullptr;
  QDoubleSpinBox *minimumFrequency = nullptr;
  QDoubleSpinBox *maximumFrequency = nullptr;
  QDoubleSpinBox *resistance = nullptr;
  QDoubleSpinBox *capacitance = nullptr;
  QDoubleSpinBox *outputDataRate = nullptr;
  QPushButton *divisionButton = nullptr;
  QPushButton *chopperButton = nullptr;
  QPushButton *generateButton = nullptr;
  QThread *generationThread = nullptr;
  quint64 generationVersion = 0;
  QList<calibration> calibrations;
  calibration mastercal_coil; ///< The master calibration coil can be loaded from (relative path) ../data/master_calibration.db (sqlite3); table name is sensor name; f a p must be present (frequency, amplitude, phase); chopper 1 == ChopperStatus::on (else off),
  calibration theoretical;    ///< Latest successfully generated theoretical calibration.
};
