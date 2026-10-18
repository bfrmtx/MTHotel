#include "cal_plotter.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[]) {
  QApplication application(argc, argv);
  QCoreApplication::setApplicationName("cal-plot");

  QCommandLineParser parser;
  parser.setApplicationDescription("Plot sensor calibration JSON, TXT and measurement XML files");
  parser.addHelpOption();
  parser.addPositionalArgument("files", "Calibration JSON, TXT or measurement XML files to open.", "[files...]");
  parser.process(application);

  CalPlotter window;
  window.show();
  window.loadFiles(parser.positionalArguments());
  return application.exec();
}
