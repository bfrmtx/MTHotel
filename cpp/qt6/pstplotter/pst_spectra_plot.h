#ifndef PST_SPECTRA_PLOT_H
#define PST_SPECTRA_PLOT_H

#include "pst_spectra.hpp"
#include "qcustomplot.h"

#include <vector>

class pst_spectra_plot : public QCustomPlot {
public:
  explicit pst_spectra_plot(QWidget *parent = nullptr);

  void set_spectra(const std::vector<pst_sensor_spectrum> &spectra);
};

#endif // PST_SPECTRA_PLOT_H
