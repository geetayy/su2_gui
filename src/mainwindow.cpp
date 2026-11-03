#include "mainwindow.hpp"

#include "widgets/convergence_widget.hpp"

#include <QApplication>
#include <QVTKOpenGLNativeWidget.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      sectionTitle(nullptr),
      caseStatus(nullptr),
      directoryEdit(nullptr),
      configEdit(nullptr),
      meshEdit(nullptr),
      mpiRanks(nullptr),
      propertyPanel(nullptr),
      propertyContainer(nullptr),
      directoryRow(nullptr),
      navigationWidget(nullptr),
      propertyLayout(nullptr),
      terminal(nullptr),
      convergenceWidget(nullptr),
      process(new QProcess(this)),
      vtkWidget(nullptr)
{
    setWindowTitle("SU2 GUI");
    resize(1500, 950);

    createMenus();
    createViewToolbar();
    createCentralWidget();
    setupVTK();

    connect(process,
            &QProcess::readyReadStandardOutput,
            this,
            &MainWindow::readStandardOutput);

    connect(process,
            &QProcess::readyReadStandardError,
            this,
            &MainWindow::readStandardError);

    connect(process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &MainWindow::processFinished);

    showSection("Working Directory");
    updateCaseState();
}
