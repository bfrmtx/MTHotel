#include "pst_spectra_plot.h"

#include <QBrush>
#include <QColor>
#include <QPen>
#include <QVector>

#include <array>

pst_spectra_plot::pst_spectra_plot(QWidget *parent) : QCustomPlot(parent) {
  this->xAxis->setScaleType(QCPAxis::stLogarithmic);
  this->yAxis->setScaleType(QCPAxis::stLogarithmic);
  this->xAxis->setTicker(QSharedPointer<QCPAxisTickerLog>(new QCPAxisTickerLog));
  this->yAxis->setTicker(QSharedPointer<QCPAxisTickerLog>(new QCPAxisTickerLog));
  this->xAxis->setLabel("Frequency [Hz]");
  this->yAxis->setLabel("ASD [nT/sqrt(Hz)]");
  this->legend->setVisible(true);
  this->legend->setBrush(QBrush(QColor(255, 255, 255, 220)));
  this->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

void pst_spectra_plot::set_spectra(const std::vector<pst_sensor_spectrum> &spectra) {
  static const std::array<QColor, 10> colors = {
      QColor("#0072B2"), QColor("#D55E00"), QColor("#009E73"), QColor("#CC79A7"), QColor("#E69F00"),
      QColor("#56B4E9"), QColor("#000000"), QColor("#8C564B"), QColor("#7F7F7F"), QColor("#BCBD22")};

  this->clearGraphs();
  for (size_t i = 0; i < spectra.size(); ++i) {
    const auto &spectrum = spectra[i];
    auto *graph = this->addGraph();
    graph->setPen(QPen(colors[i % colors.size()], 2.0));
    graph->setName(QString::fromStdString(spectrum.sensor.label()));
    const QVector<double> frequencies(spectrum.frequencies.begin(), spectrum.frequencies.end());
    const QVector<double> amplitudes(spectrum.amplitudes.begin(), spectrum.amplitudes.end());
    graph->setData(frequencies, amplitudes, true);
  }

  if (!spectra.empty()) {
    this->rescaleAxes();
  }
  this->replot(QCustomPlot::rpQueuedReplot);
}
