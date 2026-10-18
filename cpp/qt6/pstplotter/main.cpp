#include "pstplotter.h"

#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication application(argc, argv);
  pstplotter window;
  window.show();
  return application.exec();
}
