#ifndef UI_PSTPLOTTER_H
#define UI_PSTPLOTTER_H

#include <QAction>
#include <QCoreApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

QT_BEGIN_NAMESPACE

class Ui_pstplotter {
public:
  QWidget *central_widget;
  QVBoxLayout *main_layout;
  QHBoxLayout *header_layout;
  QLabel *title_label;
  QPushButton *button_plot_selected;
  QTabWidget *tabs;
  QWidget *tab_stacked_spectra;
  QVBoxLayout *stacked_spectra_layout;
  QWidget *tab_coherency;
  QWidget *tab_noise;
  QWidget *tab_calibration;
  QMenuBar *menu_bar;
  QMenu *menu_file;
  QAction *actionOpen_Survey;
  QAction *actionQuit;
  QStatusBar *status_bar;

  void setupUi(QMainWindow *window) {
    if (window->objectName().isEmpty()) {
      window->setObjectName("pstplotter");
    }
    window->resize(1180, 760);
    window->setAcceptDrops(true);

    central_widget = new QWidget(window);
    central_widget->setObjectName("central_widget");
    main_layout = new QVBoxLayout(central_widget);
    main_layout->setContentsMargins(8, 8, 8, 8);
    main_layout->setSpacing(8);

    header_layout = new QHBoxLayout();
    title_label = new QLabel(central_widget);
    QFont title_font;
    title_font.setPointSize(15);
    title_font.setBold(true);
    title_label->setFont(title_font);
    header_layout->addWidget(title_label);
    header_layout->addStretch();

    button_plot_selected = new QPushButton(central_widget);
    button_plot_selected->setObjectName("button_plot_selected");
    button_plot_selected->setIcon(window->style()->standardIcon(QStyle::SP_MediaPlay));
    button_plot_selected->setEnabled(false);
    header_layout->addWidget(button_plot_selected);
    main_layout->addLayout(header_layout);

    tabs = new QTabWidget(central_widget);
    tabs->setObjectName("tabs");

    tab_stacked_spectra = new QWidget();
    stacked_spectra_layout = new QVBoxLayout(tab_stacked_spectra);
    stacked_spectra_layout->setContentsMargins(0, 0, 0, 0);
    tabs->addTab(tab_stacked_spectra, QString());

    tab_coherency = new QWidget();
    tabs->addTab(tab_coherency, QString());
    tab_noise = new QWidget();
    tabs->addTab(tab_noise, QString());
    tab_calibration = new QWidget();
    tabs->addTab(tab_calibration, QString());

    main_layout->addWidget(tabs);
    window->setCentralWidget(central_widget);

    menu_bar = new QMenuBar(window);
    menu_file = new QMenu(menu_bar);
    actionOpen_Survey = new QAction(window);
    actionOpen_Survey->setObjectName("actionOpen_Survey");
    actionOpen_Survey->setIcon(window->style()->standardIcon(QStyle::SP_DirOpenIcon));
    actionQuit = new QAction(window);
    actionQuit->setObjectName("actionQuit");
    menu_file->addAction(actionOpen_Survey);
    menu_file->addSeparator();
    menu_file->addAction(actionQuit);
    menu_bar->addAction(menu_file->menuAction());
    window->setMenuBar(menu_bar);

    status_bar = new QStatusBar(window);
    window->setStatusBar(status_bar);

    retranslateUi(window);
    QObject::connect(actionQuit, &QAction::triggered, window, &QWidget::close);
    QMetaObject::connectSlotsByName(window);
  }

  void retranslateUi(QMainWindow *window) {
    window->setWindowTitle(QCoreApplication::translate("pstplotter", "Parallel Sensor Test Plotter"));
    title_label->setText(QCoreApplication::translate("pstplotter", "Parallel Sensor Test"));
    button_plot_selected->setText(QCoreApplication::translate("pstplotter", "Plot selected"));
    tabs->setTabText(tabs->indexOf(tab_stacked_spectra), QCoreApplication::translate("pstplotter", "Stacked spectra"));
    tabs->setTabText(tabs->indexOf(tab_coherency), QCoreApplication::translate("pstplotter", "Coherency"));
    tabs->setTabText(tabs->indexOf(tab_noise), QCoreApplication::translate("pstplotter", "Noise"));
    tabs->setTabText(tabs->indexOf(tab_calibration), QCoreApplication::translate("pstplotter", "Calibration"));
    menu_file->setTitle(QCoreApplication::translate("pstplotter", "File"));
    actionOpen_Survey->setText(QCoreApplication::translate("pstplotter", "Open Survey"));
    actionQuit->setText(QCoreApplication::translate("pstplotter", "Quit"));
  }
};

namespace Ui {
class pstplotter : public Ui_pstplotter {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PSTPLOTTER_H
