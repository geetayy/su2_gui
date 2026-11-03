#pragma once

#include <QMainWindow>
#include <QProcess>
#include <QVector>
#include <QStringList>
#include <QWidget>
#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QProgressDialog>
#include <QTreeWidget>
#include <QColor>

#include <vtkSmartPointer.h>
#include "mesh/su2_mesh.hpp"
#include "vtk/mesh_visualization.hpp"

class QLabel;
class QLineEdit;
class QComboBox;
class QPlainTextEdit;
class QSpinBox;
class QListWidget;
class QVBoxLayout;
class QWidget;
class QGroupBox;
class QPushButton;
class QToolBar;
class QSlider;
class QToolButton;
class QTimer;
class QFrame;
class QTabWidget;
class QMenu;
class QAction;
class vtkActor;
class vtkDataSetMapper;
class vtkXMLUnstructuredGridReader;
class vtkScalarBarActor;
class vtkScalarBarWidget;
class vtkLookupTable;
class vtkCallbackCommand;
class vtkPointPicker;
class vtkCellPicker;

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class vtkOrientationMarkerWidget;
class QVTKOpenGLNativeWidget;
class ConvergenceWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void newCase();
    void selectWorkingDirectory();
    void selectConfiguration();

    void runSolverSingle();
    void runSolverMPI();
    void stopSolver();
    void readStandardOutput();
    void readStandardError();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void showWorkingDirectorySection();
    void showMeshSection();
    void showPhysicsSection();
    void showNumericsSection();
    void showBoundariesSection();

    void rotateXPlus90();
    void rotateXMinus90();
    void rotateYPlus90();
    void rotateYMinus90();
    void rotateZPlus90();
    void rotateZMinus90();
    void resetView();

    // Post-processing workflow
    void selectVtuDirectory();
    void refreshVtuFiles();
    void loadVtuFile(const QString &filename);
    void updateMenusForCurrentTab(int index = -1);
    void saveViewportScreenshot();
    void showViewportControlDialog();
    void setVtuRepresentation(int mode);
    void rebuildColorByMenu();
    void rebuildResultArrayCombo();
    void updateComponentCombo();
    void applyVtuColoring();
    void rescaleVtuToDataRange();
    void rescaleVtuToVisibleRange();
    void setVtuFrame(int index);
    void toggleVtuAnimation();
    void updateVtuAnimationControls();
    void applyViewportBackground(const QString &preset, const QColor &customColor);

private:
    void createMenus();
    void createCentralWidget();
    void createViewToolbar();
    void createNavigation();
    QWidget *createPostProcessingPage();
    void updateBoundaryFamilies();
    void showSection(const QString &section);
    void updateWorkingDirectoryPanel();

    // Section editing
    void saveSectionSnapshot();
    void restoreSectionSnapshot();
    void commitCurrentSection();
    void goToNextSection();

    void updateCaseFiles();
    void updateMeshFromConfig();
    void updateCaseState();
    QString readConfigValue(const QString &key) const;
    QStringList readConfigValues(const QString &key) const;

    void setupVTK();
    void loadSU2Mesh(const QString &filename);
    void clearViewport();

    void appendOutput(const QString &text);
    void parseConvergence(const QString &text);

    QWidget *loadingOverlay = nullptr;
    QLabel *loadingLabel = nullptr;
    QProgressBar *loadingBar = nullptr;
    QProgressDialog *loadingDialog = nullptr;

    void showLoading(const QString &message);
    void hideLoading();

    void viewXY();
    void viewYZ();
    void viewXZ();

    void fitXY();
    void fitYZ();
    void fitXZ();

    SU2Mesh meshData;
    MeshVisualization meshVisualization;

    QString caseDirectory;
    QString configFile;
    QString meshFile;

    QLabel *sectionTitle;
    QLabel *caseStatus;
    QLineEdit *directoryEdit;
    QLineEdit *configEdit;
    QLineEdit *meshEdit;
    QSpinBox *mpiRanks;
    QPlainTextEdit *propertyPanel;
    QListWidget *boundaryList = nullptr;
    QTabWidget *navigationTabs = nullptr;
    QComboBox *selectionModeCombo = nullptr;
    QComboBox *displayModeCombo = nullptr;

    // Post-processing tab: directory of VTU files and selected dataset.
    QLineEdit *vtuDirectoryEdit = nullptr;
    QListWidget *vtuFileList = nullptr;
    QString postProcessingDirectory;
    QStringList vtuFiles;
    int currentVtuIndex = -1;
    QLabel *resultArrayLabel = nullptr;
    QLabel *componentLabel = nullptr;
    QComboBox *resultArrayCombo = nullptr;
    QComboBox *componentCombo = nullptr;
    QSlider *timeSlider = nullptr;
    QToolButton *playPauseButton = nullptr;
    QToolButton *rescaleDataButton = nullptr;
    QToolButton *rescaleVisibleButton = nullptr;
    QTimer *animationTimer = nullptr;
    vtkSmartPointer<vtkScalarBarActor> scalarBar;
    vtkSmartPointer<vtkScalarBarWidget> scalarBarWidget;
    vtkSmartPointer<vtkLookupTable> vtuLookupTable;
    vtkSmartPointer<vtkCallbackCommand> vtuPickCallback;
    vtkSmartPointer<vtkPointPicker> vtuPointPicker;
    vtkSmartPointer<vtkCellPicker> vtuCellPicker;
    vtkSmartPointer<vtkActor> vtuSelectionActor;
    unsigned long vtuPickObserverId = 0;
    bool vtuSelectionEnabled = false;
    vtkSmartPointer<vtkXMLUnstructuredGridReader> vtuReader;
    vtkSmartPointer<vtkActor> postProcessingActor;
    vtkSmartPointer<vtkDataSetMapper> postProcessingMapper;
    QMenu *fileMenu = nullptr;
    QMenu *viewMenu = nullptr;
    QMenu *toolsMenu = nullptr;
    QMenu *representationMenu = nullptr;
    QMenu *colorByMenu = nullptr;
    QAction *newAction = nullptr;
    QAction *saveAction = nullptr;
    QAction *fileSeparatorAction = nullptr;
    QAction *postProcessingSeparatorAction = nullptr;
    QAction *vtkToolsSeparatorAction = nullptr;
    QAction *vtkToolTailSeparatorAction = nullptr;
    QAction *screenshotAction = nullptr;
    QAction *screenshotSeparatorAction = nullptr;
    QAction *exitAction = nullptr;
    QAction *singleSolverAction = nullptr;
    QAction *mpiMenuAction = nullptr;
    QAction *postProcessingAction = nullptr;
    QAction *viewportControlAction = nullptr;
    QAction *fitDataAction = nullptr;
    QAction *showEdgesAction = nullptr;
    QAction *solidColorAction = nullptr;
    QAction *rescaleDataRangeAction = nullptr;
    QAction *rescaleVisibleRangeAction = nullptr;
    QAction *firstVtuFrameAction = nullptr;
    QAction *previousVtuFrameAction = nullptr;
    QAction *nextVtuFrameAction = nullptr;
    QAction *lastVtuFrameAction = nullptr;

    QFrame *propertyContainer;
    QWidget *directoryRow;
    QTreeWidget *navigationWidget;
    QVBoxLayout *propertyLayout;

    QPlainTextEdit *terminal;
    ConvergenceWidget *convergenceWidget;

    QProcess *process;

    QVTKOpenGLNativeWidget *vtkWidget;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkOrientationMarkerWidget> orientationWidget;

    // -------------------------------------------------
    // Current section edit/snapshot state
    // -------------------------------------------------

    QString currentSection;
    QString sectionSnapshot;
};