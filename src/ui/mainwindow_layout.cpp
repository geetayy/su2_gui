#include "../mainwindow.hpp"
#include "../mesh/su2_mesh.hpp"
#include "../widgets/convergence_widget.hpp"
#include <QAction>
#include <QApplication>
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFileInfo>
#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVTKOpenGLNativeWidget.h>
#include <QVBoxLayout>
#include <QApplication>
#include <QResizeEvent>
#include <QTimer>
#include <QFileDialog>
#include <QDir>
#include <QEventLoop>
#include <QFrame>
#include <QToolBar>
#include <QSize>
#include <QComboBox>
#include <QActionGroup>
#include <QTabWidget>
#include <QListWidgetItem>
#include <QColorDialog>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QSlider>
#include <QToolButton>
#include <QCollator>
#include <algorithm>
#include <vtkActor.h>
#include <vtkDataSet.h>
#include <vtkDataSetMapper.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSetAttributes.h>
#include <vtkProperty.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkMapper.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkScalarBarActor.h>
#include <vtkTextProperty.h>
#include <vtkLookupTable.h>
#include <vtkCell.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkSphereSource.h>
#include <vtkLineSource.h>
#include <vtkProperty.h>
#include <vtkPointPicker.h>
#include <vtkCellPicker.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkScalarBarWidget.h>
#include <vtkScalarBarRepresentation.h>

void MainWindow::createMenus()
{
    menuBar()->clear();

    fileMenu = menuBar()->addMenu("File");
    newAction = fileMenu->addAction("New");
    saveAction = fileMenu->addAction("Save");
    fileSeparatorAction = fileMenu->addSeparator();
    screenshotAction = fileMenu->addAction("Screenshot");
    screenshotSeparatorAction = fileMenu->addSeparator();
    exitAction = fileMenu->addAction("Exit");

    connect(newAction, &QAction::triggered, this, &MainWindow::newCase);
    connect(saveAction, &QAction::triggered, this, [this]()
    {
        if (configFile.isEmpty())
        {
            appendOutput("No configuration is loaded.\n");
            return;
        }
        appendOutput("CFG editing/save support will be enabled with the parameter editor.\n");
    });
    connect(screenshotAction, &QAction::triggered,
            this, &MainWindow::saveViewportScreenshot);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    viewMenu = menuBar()->addMenu("View");
    viewMenu->addAction("Rotate X +90", this, &MainWindow::rotateXPlus90);
    viewMenu->addAction("Rotate X -90", this, &MainWindow::rotateXMinus90);
    viewMenu->addAction("Rotate Y +90", this, &MainWindow::rotateYPlus90);
    viewMenu->addAction("Rotate Y -90", this, &MainWindow::rotateYMinus90);
    viewMenu->addAction("Rotate Z +90", this, &MainWindow::rotateZPlus90);
    viewMenu->addAction("Rotate Z -90", this, &MainWindow::rotateZMinus90);
    viewMenu->addSeparator();
    viewMenu->addAction("Reset View", this, &MainWindow::resetView);
    viewMenu->addSeparator();
    viewportControlAction = viewMenu->addAction("ViewPort Control");
    connect(viewportControlAction, &QAction::triggered,
            this, &MainWindow::showViewportControlDialog);

    toolsMenu = menuBar()->addMenu("Tools");
    singleSolverAction = toolsMenu->addAction("Run SU2 - Single CPU");
    connect(singleSolverAction, &QAction::triggered,
            this, &MainWindow::runSolverSingle);

    QMenu *mpiMenu = toolsMenu->addMenu("Run SU2 - Multiple CPU");
    mpiMenuAction = mpiMenu->menuAction();
    QWidget *mpiWidget = new QWidget(mpiMenu);
    QHBoxLayout *mpiLayout = new QHBoxLayout(mpiWidget);
    mpiLayout->setContentsMargins(8, 4, 8, 4);
    QLabel *mpiLabel = new QLabel("Number of MPI Ranks:", mpiWidget);
    mpiRanks = new QSpinBox(mpiWidget);
    mpiRanks->setRange(1, 1024);
    mpiRanks->setValue(4);
    mpiLayout->addWidget(mpiLabel);
    mpiLayout->addWidget(mpiRanks);
    QWidgetAction *mpiRanksAction = new QWidgetAction(mpiMenu);
    mpiRanksAction->setDefaultWidget(mpiWidget);
    mpiMenu->addAction(mpiRanksAction);
    QAction *mpiRunAction = mpiMenu->addAction("Run");
    connect(mpiRunAction, &QAction::triggered,
            this, &MainWindow::runSolverMPI);

    postProcessingSeparatorAction = toolsMenu->addSeparator();
    postProcessingAction = toolsMenu->addAction("Post Processing");
    connect(postProcessingAction, &QAction::triggered, this, [this]()
    {
        if (navigationTabs)
            navigationTabs->setCurrentIndex(1);
    });

    vtkToolsSeparatorAction = toolsMenu->addSeparator();
    representationMenu = toolsMenu->addMenu("Representation");
    const QStringList representations = {
        "Surface", "Surface with Edges", "Wireframe", "Points"
    };
    for (int i = 0; i < representations.size(); ++i)
    {
        QAction *action = representationMenu->addAction(representations[i]);
        connect(action, &QAction::triggered, this,
                [this, i]() { setVtuRepresentation(i); });
    }

    colorByMenu = toolsMenu->addMenu("Color By");
    solidColorAction = colorByMenu->addAction("Solid Color");
    connect(solidColorAction, &QAction::triggered, this, [this]()
    {
        if (!postProcessingMapper || !postProcessingActor)
            return;
        postProcessingMapper->ScalarVisibilityOff();
        postProcessingActor->GetProperty()->SetColor(0.73, 0.18, 0.24);
        if (scalarBarWidget) scalarBarWidget->SetEnabled(0);
        if (scalarBar) scalarBar->SetVisibility(false);
        if (renderWindow) renderWindow->Render();
    });

    rescaleDataRangeAction = toolsMenu->addAction("Rescale to Data Range");
    connect(rescaleDataRangeAction, &QAction::triggered,
            this, &MainWindow::rescaleVtuToDataRange);
    rescaleVisibleRangeAction = toolsMenu->addAction("Rescale to Visible Data Range");
    connect(rescaleVisibleRangeAction, &QAction::triggered,
            this, &MainWindow::rescaleVtuToVisibleRange);
    QMenu *scalarLegendMenu = toolsMenu->addMenu("Scalar Legend");
    QAction *legendPropertiesAction = scalarLegendMenu->addAction("Properties...");
    connect(legendPropertiesAction, &QAction::triggered, this, [this]()
    {
        if (!scalarBar || !scalarBarWidget ||
            !scalarBarWidget->GetScalarBarRepresentation())
            return;

        auto *barRep = scalarBarWidget->GetScalarBarRepresentation();
        auto *titleProp = scalarBar->GetTitleTextProperty();
        auto *labelProp = scalarBar->GetLabelTextProperty();

        QDialog dialog(this);
        dialog.setWindowTitle("Scalar Legend Properties");
        QFormLayout form(&dialog);

        auto *orientationBox = new QComboBox(&dialog);
        orientationBox->addItems({"Automatic (follow viewport edge)",
                                  "Vertical", "Horizontal"});
        orientationBox->setCurrentIndex(barRep->GetAutoOrient()
                                            ? 0
                                            : (scalarBar->GetOrientation() == VTK_ORIENT_HORIZONTAL ? 2 : 1));
        form.addRow("Orientation:", orientationBox);

        auto *fontFamilyBox = new QComboBox(&dialog);
        fontFamilyBox->addItems({"Arial", "Courier", "Times"});
        const QString currentFamily = QString::fromLatin1(titleProp->GetFontFamilyAsString());
        int familyIndex = fontFamilyBox->findText(currentFamily, Qt::MatchFixedString);
        fontFamilyBox->setCurrentIndex(familyIndex >= 0 ? familyIndex : 0);
        form.addRow("Font family:", fontFamilyBox);

        auto *titleSize = new QSpinBox(&dialog);
        titleSize->setRange(6, 48);
        titleSize->setValue(titleProp->GetFontSize());
        form.addRow("Title font size:", titleSize);

        auto *labelSize = new QSpinBox(&dialog);
        labelSize->setRange(6, 48);
        labelSize->setValue(labelProp->GetFontSize());
        form.addRow("Label font size:", labelSize);

        auto *labelCount = new QSpinBox(&dialog);
        labelCount->setRange(2, 20);
        labelCount->setValue(scalarBar->GetNumberOfLabels());
        form.addRow("Number of labels:", labelCount);

        auto *titleBold = new QCheckBox("Bold title", &dialog);
        titleBold->setChecked(titleProp->GetBold());
        form.addRow(titleBold);
        auto *titleItalic = new QCheckBox("Italic title", &dialog);
        titleItalic->setChecked(titleProp->GetItalic());
        form.addRow(titleItalic);
        auto *labelBold = new QCheckBox("Bold labels", &dialog);
        labelBold->setChecked(labelProp->GetBold());
        form.addRow(labelBold);
        auto *labelItalic = new QCheckBox("Italic labels", &dialog);
        labelItalic->setChecked(labelProp->GetItalic());
        form.addRow(labelItalic);

        auto *labelFormat = new QLineEdit(QString::fromUtf8(scalarBar->GetLabelFormat()), &dialog);
        form.addRow("Label format:", labelFormat);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                             Qt::Horizontal, &dialog);
        form.addRow(buttons);
        QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if (dialog.exec() != QDialog::Accepted)
            return;

        const QByteArray family = fontFamilyBox->currentText().toLatin1();
        titleProp->SetFontFamilyAsString(family.constData());
        labelProp->SetFontFamilyAsString(family.constData());
        titleProp->SetFontSize(titleSize->value());
        labelProp->SetFontSize(labelSize->value());
        titleProp->SetBold(titleBold->isChecked());
        titleProp->SetItalic(titleItalic->isChecked());
        labelProp->SetBold(labelBold->isChecked());
        labelProp->SetItalic(labelItalic->isChecked());
        titleProp->SetShadow(false);
        labelProp->SetShadow(false);
        scalarBar->SetUnconstrainedFontSize(true);
        scalarBar->SetNumberOfLabels(labelCount->value());
        const QByteArray format = labelFormat->text().toLatin1();
        scalarBar->SetLabelFormat(format.constData());

        if (orientationBox->currentIndex() == 0)
        {
            barRep->SetAutoOrient(true);
        }
        else
        {
            barRep->SetAutoOrient(false);
            if (orientationBox->currentIndex() == 1)
                scalarBar->SetOrientationToVertical();
            else
                scalarBar->SetOrientationToHorizontal();
            barRep->SetOrientation(scalarBar->GetOrientation());
        }

        scalarBar->Modified();
        if (scalarBar->GetVisibility())
            scalarBarWidget->SetEnabled(1);
        if (renderWindow)
            renderWindow->Render();
    });

    QAction *resetLegendLayoutAction = toolsMenu->addAction("Reset Scalar Legend Layout");
    connect(resetLegendLayoutAction, &QAction::triggered, this, [this]()
    {
        if (!scalarBarWidget || !scalarBarWidget->GetScalarBarRepresentation())
            return;
        auto *barRep = scalarBarWidget->GetScalarBarRepresentation();
        barRep->SetPosition(0.84, 0.12);
        barRep->SetPosition2(0.14, 0.76);
        barRep->SetMoving(1);
        barRep->SetAutoOrient(true);
        if (scalarBar) scalarBar->SetOrientationToVertical();
        if (scalarBar && scalarBar->GetVisibility())
            scalarBarWidget->SetEnabled(1);
        if (renderWindow) renderWindow->Render();
    });

    vtkToolTailSeparatorAction = toolsMenu->addSeparator();
    fitDataAction = toolsMenu->addAction("Fit Data");
    connect(fitDataAction, &QAction::triggered, this, [this]()
    {
        if (renderer && postProcessingActor)
        {
            renderer->ResetCamera();
            renderer->ResetCameraClippingRange();
            if (renderWindow) renderWindow->Render();
        }
        else
            meshVisualization.fitView();
    });

    showEdgesAction = toolsMenu->addAction("Toggle Surface Edges");
    showEdgesAction->setCheckable(true);
    showEdgesAction->setChecked(true);
    connect(showEdgesAction, &QAction::toggled, this, [this](bool checked)
    {
        if (!postProcessingActor)
            return;
        postProcessingActor->GetProperty()->SetEdgeVisibility(checked);
        if (renderWindow) renderWindow->Render();
    });

    updateMenusForCurrentTab();
}

void MainWindow::createNavigation()
{
    navigationTabs = new QTabWidget;
    navigationTabs->setObjectName("su2NavigationTabs");

    QWidget *su2Page = new QWidget(navigationTabs);
    QHBoxLayout *su2Layout = new QHBoxLayout(su2Page);
    su2Layout->setContentsMargins(4, 4, 4, 4);
    su2Layout->setSpacing(8);

    QGroupBox *setupBox = new QGroupBox("Setup", su2Page);
    QVBoxLayout *setupLayout = new QVBoxLayout(setupBox);
    setupLayout->setContentsMargins(4, 8, 4, 4);

    navigationWidget = new QTreeWidget(setupBox);
    navigationWidget->setObjectName("sectionList");
    navigationWidget->setHeaderHidden(true);
    navigationWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    navigationWidget->setIndentation(18);
    navigationWidget->setRootIsDecorated(true);
    navigationWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    navigationWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QTreeWidgetItem *setupItem = new QTreeWidgetItem(navigationWidget);
    setupItem->setText(0, "Setup");
    const QStringList sections = {
        "Working Directory", "Mesh", "Physics", "Numerics", "Boundaries"
    };
    for (const QString &section : sections)
    {
        QTreeWidgetItem *child = new QTreeWidgetItem(setupItem);
        child->setText(0, section);
    }
    setupItem->setExpanded(true);
    setupLayout->addWidget(navigationWidget);

    QGroupBox *boundaryBox = new QGroupBox("Boundary families", su2Page);
    QVBoxLayout *boundaryLayout = new QVBoxLayout(boundaryBox);
    boundaryLayout->setContentsMargins(4, 8, 4, 4);
    QLabel *boundaryHint = new QLabel(
        "Check a marker to show its boundary on the mesh.", boundaryBox);
    boundaryHint->setWordWrap(true);
    boundaryLayout->addWidget(boundaryHint);

    boundaryList = new QListWidget(boundaryBox);
    boundaryList->setObjectName("boundaryFamiliesList");
    boundaryList->setSelectionMode(QAbstractItemView::NoSelection);
    boundaryList->setAlternatingRowColors(true);
    boundaryLayout->addWidget(boundaryList, 1);

    su2Layout->addWidget(setupBox, 1);
    su2Layout->addWidget(boundaryBox, 1);
    navigationTabs->addTab(su2Page, "SU2");
    navigationTabs->addTab(createPostProcessingPage(), "Post Processing");

    connect(navigationWidget, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem *current, QTreeWidgetItem *)
    {
        if (!current || current->parent() == nullptr)
            return;
        showSection(current->text(0));
    });

    connect(boundaryList, &QListWidget::itemChanged, this,
            [this](QListWidgetItem *item)
    {
        if (!item) return;
        meshVisualization.setBoundaryVisible(
            item->data(Qt::UserRole).toString(),
            item->checkState() == Qt::Checked);
    });

    connect(navigationTabs, &QTabWidget::currentChanged,
            this, &MainWindow::updateMenusForCurrentTab);
}

QWidget *MainWindow::createPostProcessingPage()
{
    QWidget *page = new QWidget(navigationTabs);
    QHBoxLayout *pageLayout = new QHBoxLayout(page);
    pageLayout->setContentsMargins(4, 4, 4, 4);
    pageLayout->setSpacing(8);

    // Setup contains only the directory that holds the VTU files.
    QGroupBox *setupBox = new QGroupBox("Setup", page);
    QVBoxLayout *setupLayout = new QVBoxLayout(setupBox);
    setupLayout->setContentsMargins(6, 10, 6, 6);
    setupLayout->addWidget(new QLabel("Working directory:", setupBox));

    vtuDirectoryEdit = new QLineEdit(setupBox);
    vtuDirectoryEdit->setReadOnly(true);
    vtuDirectoryEdit->setPlaceholderText("Select a directory containing .vtu files");
    QPushButton *browseButton = new QPushButton("Browse...", setupBox);
    QHBoxLayout *directoryLayout = new QHBoxLayout;
    directoryLayout->addWidget(vtuDirectoryEdit, 1);
    directoryLayout->addWidget(browseButton);
    setupLayout->addLayout(directoryLayout);
    setupLayout->addStretch(1);

    QGroupBox *filesBox = new QGroupBox("VTU Files", page);
    QVBoxLayout *filesLayout = new QVBoxLayout(filesBox);
    QLabel *filesHint = new QLabel(
        "Select a .vtu file to load it into the viewport.", filesBox);
    filesHint->setWordWrap(true);
    filesLayout->addWidget(filesHint);
    vtuFileList = new QListWidget(filesBox);
    vtuFileList->setObjectName("vtuFileList");
    vtuFileList->setSelectionMode(QAbstractItemView::SingleSelection);
    filesLayout->addWidget(vtuFileList, 1);

    pageLayout->addWidget(setupBox, 2);
    pageLayout->addWidget(filesBox, 3);

    connect(browseButton, &QPushButton::clicked,
            this, &MainWindow::selectVtuDirectory);
    connect(vtuFileList, &QListWidget::itemClicked, this,
            [this](QListWidgetItem *item)
    {
        if (item) loadVtuFile(item->data(Qt::UserRole).toString());
    });
    return page;
}

void MainWindow::updateMenusForCurrentTab(int index)
{
    const int currentIndex = navigationTabs
        ? (index < 0 ? navigationTabs->currentIndex() : index) : 0;
    const bool postProcessing = currentIndex == 1;

    if (newAction) newAction->setVisible(!postProcessing);
    if (saveAction) saveAction->setVisible(!postProcessing);
    if (fileSeparatorAction) fileSeparatorAction->setVisible(!postProcessing);
    if (screenshotAction) screenshotAction->setVisible(postProcessing);
    if (screenshotSeparatorAction) screenshotSeparatorAction->setVisible(postProcessing);

    // View actions are shared by both modes; ViewPort Control is always available.
    if (viewportControlAction) viewportControlAction->setVisible(true);

    if (singleSolverAction) singleSolverAction->setVisible(!postProcessing);
    if (mpiMenuAction) mpiMenuAction->setVisible(!postProcessing);
    if (postProcessingSeparatorAction) postProcessingSeparatorAction->setVisible(!postProcessing);
    if (vtkToolsSeparatorAction) vtkToolsSeparatorAction->setVisible(false);
    if (vtkToolTailSeparatorAction) vtkToolTailSeparatorAction->setVisible(postProcessing);
    if (representationMenu) representationMenu->menuAction()->setVisible(postProcessing);
    if (colorByMenu) colorByMenu->menuAction()->setVisible(postProcessing);
    if (rescaleDataRangeAction) rescaleDataRangeAction->setVisible(postProcessing);
    if (rescaleVisibleRangeAction) rescaleVisibleRangeAction->setVisible(postProcessing);
    if (rescaleDataButton) rescaleDataButton->setVisible(postProcessing);
    if (rescaleVisibleButton) rescaleVisibleButton->setVisible(postProcessing);
    if (fitDataAction) fitDataAction->setVisible(postProcessing);
    if (showEdgesAction) showEdgesAction->setVisible(postProcessing);
    if (resultArrayLabel) resultArrayLabel->setVisible(postProcessing);
    if (resultArrayCombo) resultArrayCombo->setVisible(postProcessing);
    if (componentLabel) componentLabel->setVisible(postProcessing);
    if (componentCombo) componentCombo->setVisible(postProcessing);
    if (timeSlider) timeSlider->setVisible(postProcessing);
    if (playPauseButton) playPauseButton->setVisible(postProcessing);
    if (firstVtuFrameAction) firstVtuFrameAction->setVisible(postProcessing);
    if (previousVtuFrameAction) previousVtuFrameAction->setVisible(postProcessing);
    if (nextVtuFrameAction) nextVtuFrameAction->setVisible(postProcessing);
    if (lastVtuFrameAction) lastVtuFrameAction->setVisible(postProcessing);
    if (postProcessingAction)
    {
        postProcessingAction->setVisible(!postProcessing);
        postProcessingAction->setEnabled(!postProcessing);
    }
}

void MainWindow::selectVtuDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this, "Select Directory Containing VTU Files",
        postProcessingDirectory.isEmpty() ? QDir::homePath() : postProcessingDirectory);
    if (directory.isEmpty()) return;

    if (animationTimer) animationTimer->stop();
    postProcessingDirectory = QDir(directory).absolutePath();
    if (vtuDirectoryEdit) vtuDirectoryEdit->setText(postProcessingDirectory);
    refreshVtuFiles();
}

void MainWindow::refreshVtuFiles()
{
    if (!vtuFileList) return;
    vtuFileList->clear();
    vtuFiles.clear();
    currentVtuIndex = -1;
    if (postProcessingDirectory.isEmpty())
    {
        updateVtuAnimationControls();
        return;
    }

    const QDir directory(postProcessingDirectory);
    vtuFiles = directory.entryList(
        QStringList() << "*.vtu" << "*.VTU", QDir::Files, QDir::NoSort);
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(vtuFiles.begin(), vtuFiles.end(),
              [&collator](const QString &a, const QString &b)
              { return collator.compare(a, b) < 0; });

    for (const QString &name : vtuFiles)
    {
        auto *item = new QListWidgetItem(name, vtuFileList);
        item->setData(Qt::UserRole, directory.absoluteFilePath(name));
        item->setToolTip(directory.absoluteFilePath(name));
    }
    if (vtuFiles.isEmpty())
    {
        auto *item = new QListWidgetItem("No .vtu files found in this directory.", vtuFileList);
        item->setFlags(Qt::NoItemFlags);
    }
    updateVtuAnimationControls();
}

void MainWindow::loadVtuFile(const QString &filename)
{
    if (filename.isEmpty() || !QFileInfo::exists(filename)) return;

    const QString absoluteName = QFileInfo(filename).absoluteFilePath();
    const int index = vtuFiles.indexOf(QFileInfo(absoluteName).fileName());
    if (index >= 0 && vtuReader)
    {
        setVtuFrame(index);
        return;
    }

    auto reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
    const QByteArray encodedFilename = QFile::encodeName(absoluteName);
    reader->SetFileName(encodedFilename.constData());
    reader->Update();

    vtkUnstructuredGrid *grid = reader->GetOutput();
    if (!grid || grid->GetNumberOfPoints() == 0)
    {
        QMessageBox::warning(this, "VTU Load Failed",
            "The selected file could not be loaded or contains no points.");
        return;
    }

    meshVisualization.clear();
    if (vtuSelectionActor && renderer)
        renderer->RemoveActor(vtuSelectionActor);
    vtuSelectionActor = nullptr;
    if (postProcessingActor && renderer)
        renderer->RemoveActor(postProcessingActor);

    vtuReader = reader;
    currentVtuIndex = -1;
    postProcessingMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    postProcessingMapper->SetInputConnection(vtuReader->GetOutputPort());
    postProcessingMapper->ScalarVisibilityOff();

    postProcessingActor = vtkSmartPointer<vtkActor>::New();
    postProcessingActor->SetMapper(postProcessingMapper);
    if (vtuPointPicker) {
        vtuPointPicker->InitializePickList();
        vtuPointPicker->AddPickList(postProcessingActor);
        vtuPointPicker->PickFromListOn();
    }
    if (vtuCellPicker) {
        vtuCellPicker->InitializePickList();
        vtuCellPicker->AddPickList(postProcessingActor);
        vtuCellPicker->PickFromListOn();
    }
    // Enable VTU selection immediately after loading; the Select/ View buttons
    // can still toggle between picking and camera interaction.
    vtuSelectionEnabled = true;
    if (QAction *selectMode = findChild<QAction *>("vtuSelectModeAction"))
        selectMode->setChecked(true);
    postProcessingActor->GetProperty()->SetColor(0.73, 0.18, 0.24);
    // postProcessingActor->GetProperty()->SetEdgeVisibility(showEdgesAction ? showEdgesAction->isChecked() : true);
    const int displayMode = displayModeCombo ? displayModeCombo->currentIndex() : 0;
    setVtuRepresentation(displayMode);

    if (renderer)
    {
        renderer->AddActor(postProcessingActor);
        renderer->ResetCamera();
        renderer->ResetCameraClippingRange();
    }
    currentVtuIndex = index;
    rebuildResultArrayCombo();
    rebuildColorByMenu();
    if (timeSlider && currentVtuIndex >= 0)
    {
        QSignalBlocker blocker(timeSlider);
        timeSlider->setValue(currentVtuIndex);
    }
    updateVtuAnimationControls();
    if (renderWindow) renderWindow->Render();
    appendOutput(QString("Loaded VTU file: %1\n").arg(absoluteName));
}

void MainWindow::setVtuFrame(int index)
{
    if (!vtuReader || index < 0 || index >= vtuFiles.size() ||
        postProcessingDirectory.isEmpty()) return;

    const QString filename = QDir(postProcessingDirectory).absoluteFilePath(vtuFiles[index]);
    const QByteArray encodedFilename = QFile::encodeName(filename);
    vtuReader->SetFileName(encodedFilename.constData());
    vtuReader->Update();

    vtkUnstructuredGrid *grid = vtuReader->GetOutput();
    if (!grid || grid->GetNumberOfPoints() == 0)
    {
        appendOutput(QString("WARNING: Could not load VTU frame: %1\n").arg(filename));
        return;
    }

    currentVtuIndex = index;
    const QString previousArray = resultArrayCombo
        ? resultArrayCombo->currentData(Qt::UserRole + 1).toString() : QString();
    const int previousMode = resultArrayCombo
        ? resultArrayCombo->currentData(Qt::UserRole).toInt() : VTK_SCALAR_MODE_USE_POINT_FIELD_DATA;

    rebuildResultArrayCombo();
    if (resultArrayCombo && !previousArray.isEmpty())
    {
        for (int i = 0; i < resultArrayCombo->count(); ++i)
        {
            if (resultArrayCombo->itemData(i, Qt::UserRole).toInt() == previousMode &&
                resultArrayCombo->itemData(i, Qt::UserRole + 1).toString() == previousArray)
            {
                resultArrayCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    applyVtuColoring();

    if (timeSlider)
    {
        QSignalBlocker blocker(timeSlider);
        timeSlider->setValue(index);
    }
    if (vtuFileList)
    {
        QListWidgetItem *item = vtuFileList->findItems(vtuFiles[index], Qt::MatchExactly).value(0, nullptr);
        if (item) vtuFileList->setCurrentItem(item);
    }
    updateVtuAnimationControls();
    if (renderWindow) renderWindow->Render();
}

void MainWindow::rebuildResultArrayCombo()
{
    if (!resultArrayCombo || !vtuReader || !vtuReader->GetOutput()) return;
    const QString previousArray = resultArrayCombo->currentData(Qt::UserRole + 1).toString();
    const int previousMode = resultArrayCombo->currentData(Qt::UserRole).toInt();
    QSignalBlocker blocker(resultArrayCombo);
    resultArrayCombo->clear();
    // Solid Color is the default coloring mode.
    resultArrayCombo->addItem("Solid Color");
    resultArrayCombo->setItemData(0, -1, Qt::UserRole);
    resultArrayCombo->setItemData(0, "", Qt::UserRole + 1);

    vtkDataSet *data = vtuReader->GetOutput();
    auto addArrays = [this](vtkDataSetAttributes *attributes, int scalarMode,
                            const QString &prefix)
    {
        if (!attributes) return;
        for (int i = 0; i < attributes->GetNumberOfArrays(); ++i)
        {
            vtkDataArray *array = attributes->GetArray(i);
            if (!array || !array->GetName()) continue;
            const QString name = QString::fromUtf8(array->GetName());
            const int item = resultArrayCombo->count();
            resultArrayCombo->addItem(prefix + name);
            resultArrayCombo->setItemData(item, scalarMode, Qt::UserRole);
            resultArrayCombo->setItemData(item, name, Qt::UserRole + 1);
        }
    };
    addArrays(data->GetPointData(), VTK_SCALAR_MODE_USE_POINT_FIELD_DATA, "Point: ");
    addArrays(data->GetCellData(), VTK_SCALAR_MODE_USE_CELL_FIELD_DATA, "Cell: ");

    int restore = -1;
    for (int i = 0; i < resultArrayCombo->count(); ++i)
        if (resultArrayCombo->itemData(i, Qt::UserRole).toInt() == previousMode &&
            resultArrayCombo->itemData(i, Qt::UserRole + 1).toString() == previousArray)
            restore = i;
    if (restore >= 0) resultArrayCombo->setCurrentIndex(restore);
    else if (resultArrayCombo->count() > 0) resultArrayCombo->setCurrentIndex(0);
    updateComponentCombo();
    applyVtuColoring();
}

void MainWindow::updateComponentCombo()
{
    if (!componentCombo) return;
    QSignalBlocker blocker(componentCombo);
    componentCombo->clear();
    componentCombo->addItem("Magnitude", -1);

    if (!vtuReader || !vtuReader->GetOutput() || !resultArrayCombo ||
        resultArrayCombo->currentIndex() < 0) return;
    const QString name = resultArrayCombo->currentData(Qt::UserRole + 1).toString();
    const int mode = resultArrayCombo->currentData(Qt::UserRole).toInt();
    vtkDataSet *data = vtuReader->GetOutput();
    vtkDataArray *array = mode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA
        ? data->GetCellData()->GetArray(name.toUtf8().constData())
        : data->GetPointData()->GetArray(name.toUtf8().constData());
    if (!array) return;
    const int components = array->GetNumberOfComponents();
    if (components == 1)
    {
        componentCombo->setCurrentIndex(0);
        return;
    }
    for (int c = 0; c < components; ++c)
    {
        const QString label = array->GetComponentName(c)
            ? QString::fromUtf8(array->GetComponentName(c))
            : QString("Component %1").arg(c);
        componentCombo->addItem(label, c);
    }
    componentCombo->setCurrentIndex(0);
}

void MainWindow::applyVtuColoring()
{
    if (!postProcessingMapper || !postProcessingActor || !vtuReader ||
        !vtuReader->GetOutput() || !resultArrayCombo ||
        resultArrayCombo->currentIndex() < 0)
    {
        if (scalarBarWidget) scalarBarWidget->SetEnabled(0);
        if (scalarBar) scalarBar->SetVisibility(false);
        return;
    }

    // Solid Color mode
    if (resultArrayCombo->currentData(Qt::UserRole).toInt() == -1)
    {
        postProcessingMapper->ScalarVisibilityOff();
        postProcessingActor->GetProperty()->SetColor(0.73, 0.18, 0.24);
        if (scalarBarWidget)
            scalarBarWidget->SetEnabled(0);
        if (scalarBar)
            scalarBar->SetVisibility(false);
        if (renderWindow)
            renderWindow->Render();
        return;
    }

    const QString name = resultArrayCombo->currentData(Qt::UserRole + 1).toString();
    const int mode = resultArrayCombo->currentData(Qt::UserRole).toInt();
    const int component = componentCombo ? componentCombo->currentData().toInt() : -1;
    vtkDataSet *data = vtuReader->GetOutput();
    vtkDataArray *array = mode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA
        ? data->GetCellData()->GetArray(name.toUtf8().constData())
        : data->GetPointData()->GetArray(name.toUtf8().constData());
    if (!array)
    {
        if (scalarBarWidget) scalarBarWidget->SetEnabled(0);
        if (scalarBar) scalarBar->SetVisibility(false);
        appendOutput(QString("VTU result array not found: %1\n").arg(name));
        return;
    }

    double range[2];
    array->GetRange(range, component);
    if (range[0] == range[1])
    {
        range[0] -= 1.0;
        range[1] += 1.0;
    }
    postProcessingMapper->SetScalarMode(mode);
    postProcessingMapper->SelectColorArray(name.toUtf8().constData());
    postProcessingMapper->SetArrayComponent(component);
    postProcessingMapper->SetColorModeToMapScalars();

    if (!vtuLookupTable)
    {
        vtuLookupTable = vtkSmartPointer<vtkLookupTable>::New();
        vtuLookupTable->SetHueRange(0.667, 0.0);
        vtuLookupTable->SetSaturationRange(1.0, 1.0);
        vtuLookupTable->SetValueRange(1.0, 1.0);
        vtuLookupTable->SetNumberOfTableValues(256);
    }
    vtuLookupTable->SetRange(range);
    vtuLookupTable->Build();
    postProcessingMapper->SetLookupTable(vtuLookupTable);
    postProcessingMapper->SetScalarRange(range);
    postProcessingMapper->UseLookupTableScalarRangeOn();
    postProcessingMapper->ScalarVisibilityOn();
    postProcessingActor->GetProperty()->SetColor(1.0, 1.0, 1.0);

/*    if (scalarBar)
    {
        scalarBar->SetLookupTable(vtuLookupTable);
        const QString componentName = component < 0
            ? QString() : (componentCombo ? componentCombo->currentText() : QString());
        const QString legendTitle = componentName.isEmpty()
            ? name : QString("%1 - %2").arg(name, componentName);
        const QByteArray title = legendTitle.toUtf8();
        scalarBar->SetTitle(title.constData());
        scalarBar->SetVisibility(1);
        scalarBar->Modified();
        if (scalarBarWidget && !scalarBarWidget->GetEnabled())
            scalarBarWidget->SetEnabled(1);
    }
    if (renderer)
        renderer->ResetCameraClippingRange();
    if (renderWindow)
        renderWindow->Render();*/

    if (scalarBar)
    {
        scalarBar->SetLookupTable(vtuLookupTable);

        const QString componentName = component < 0
            ? QString()
            : (componentCombo ? componentCombo->currentText() : QString());

        const QString legendTitle = componentName.isEmpty()
            ? name
            : QString("%1 - %2").arg(name, componentName);

        // Detect the current orientation.
        const bool vertical =
            scalarBar->GetOrientation() == VTK_ORIENT_VERTICAL;

        if (vertical)
        {
            // Vertical legend: title reads bottom to top.
            scalarBar->SetOrientationToVertical();
            scalarBar->SetTextPositionToSucceedScalarBar();
            scalarBar->SetVerticalTitleSeparation(18);
            scalarBar->SetTitle(legendTitle.toUtf8().constData());
        }
        else
        {
            // Horizontal legend: title above numerical labels.
            scalarBar->SetOrientationToHorizontal();
            scalarBar->SetTextPositionToPrecedeScalarBar();

            // Allocate more room for the title.
            scalarBar->SetTitleRatio(0.35);
            scalarBar->SetTextPad(12);

            scalarBar->SetTitle(legendTitle.toUtf8().constData());
        }

        scalarBar->SetVisibility(1);
        scalarBar->Modified();

        if (scalarBarWidget && !scalarBarWidget->GetEnabled())
            scalarBarWidget->SetEnabled(1);
    }
}

void MainWindow::rescaleVtuToDataRange()
{
    if (!vtuReader || !vtuReader->GetOutput() || !resultArrayCombo ||
        resultArrayCombo->currentIndex() < 0 || !postProcessingMapper)
        return;

    const QString name = resultArrayCombo->currentData(Qt::UserRole + 1).toString();
    const int mode = resultArrayCombo->currentData(Qt::UserRole).toInt();
    const int component = componentCombo ? componentCombo->currentData().toInt() : -1;
    vtkDataSet *data = vtuReader->GetOutput();
    vtkDataArray *array = mode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA
        ? data->GetCellData()->GetArray(name.toUtf8().constData())
        : data->GetPointData()->GetArray(name.toUtf8().constData());
    if (!array)
        return;

    double range[2];
    array->GetRange(range, component);
    if (range[0] == range[1]) {
        range[0] -= 1.0;
        range[1] += 1.0;
    }

    if (vtuLookupTable) {
        vtuLookupTable->SetRange(range);
        vtuLookupTable->Build();
    }
    postProcessingMapper->SetScalarRange(range);
    if (scalarBar) {
        scalarBar->SetLookupTable(vtuLookupTable);
        scalarBar->SetVisibility(true);
    }
    if (scalarBarWidget && !scalarBarWidget->GetEnabled())
        scalarBarWidget->SetEnabled(1);
    if (renderWindow) renderWindow->Render();
}

void MainWindow::rescaleVtuToVisibleRange()
{
    // The current Post Processing view displays one complete VTU dataset.
    // With no clipping or hidden-block filters, the visible and full data
    // ranges are identical. Keep this as a separate command for later filters.
    rescaleVtuToDataRange();
}

void MainWindow::toggleVtuAnimation()
{
    if (!animationTimer)
    {
        animationTimer = new QTimer(this);
        animationTimer->setInterval(120);
        connect(animationTimer, &QTimer::timeout, this, [this]()
        {
            if (currentVtuIndex + 1 >= vtuFiles.size())
            {
                animationTimer->stop();
                updateVtuAnimationControls();
                return;
            }
            setVtuFrame(currentVtuIndex + 1);
        });
    }
    if (animationTimer->isActive())
        animationTimer->stop();
    else if (vtuFiles.size() > 1)
    {
        if (currentVtuIndex < 0) setVtuFrame(0);
        animationTimer->start();
    }
    updateVtuAnimationControls();
}

void MainWindow::updateVtuAnimationControls()
{
    const bool hasFrames = vtuFiles.size() > 0;
    if (timeSlider)
    {
        timeSlider->setEnabled(hasFrames);
        timeSlider->setRange(0, qMax(0, vtuFiles.size() - 1));
        if (currentVtuIndex >= 0) timeSlider->setValue(currentVtuIndex);
    }
    if (playPauseButton)
    {
        playPauseButton->setEnabled(vtuFiles.size() > 1);
        playPauseButton->setText(animationTimer && animationTimer->isActive() ? "Pause" : "Play");
    }
}

void MainWindow::setVtuRepresentation(int mode)
{
    if (!postProcessingActor) return;
    auto *property = postProcessingActor->GetProperty();
    switch (mode)
    {
    case 0:
        property->SetRepresentationToSurface();
        property->SetEdgeVisibility(false);
        if (showEdgesAction) showEdgesAction->setChecked(false);
        break;
    case 1:
        property->SetRepresentationToSurface();
        property->SetEdgeVisibility(true);
        if (showEdgesAction) showEdgesAction->setChecked(true);
        break;
    case 2: property->SetRepresentationToWireframe(); break;
    case 3: property->SetRepresentationToPoints(); break;
    default: break;
    }
    if (renderWindow) renderWindow->Render();
}

void MainWindow::rebuildColorByMenu()
{
    if (!colorByMenu) return;
    colorByMenu->clear();
    solidColorAction = colorByMenu->addAction("Solid Color");
    connect(solidColorAction, &QAction::triggered, this, [this]()
    {
        if (!postProcessingMapper || !postProcessingActor) return;
        postProcessingMapper->ScalarVisibilityOff();
        postProcessingActor->GetProperty()->SetColor(0.73, 0.18, 0.24);
        if (scalarBarWidget) scalarBarWidget->SetEnabled(0);
        if (scalarBar) scalarBar->SetVisibility(false);
        if (renderWindow) renderWindow->Render();
    });

    if (!vtuReader || !vtuReader->GetOutput()) return;
    vtkDataSet *data = vtuReader->GetOutput();
    auto addArrayActions = [this](QMenu *parentMenu, const QString &title,
                                  int scalarMode, int arrayCount, auto arrayNameAt)
    {
        if (arrayCount <= 0) return;
        QMenu *arrayMenu = parentMenu->addMenu(title);
        for (int i = 0; i < arrayCount; ++i)
        {
            const char *rawName = arrayNameAt(i);
            if (!rawName || !*rawName) continue;
            const QString name = QString::fromUtf8(rawName);
            QAction *action = arrayMenu->addAction(name);
            connect(action, &QAction::triggered, this, [this, name, scalarMode]()
            {
                if (resultArrayCombo)
                {
                    for (int item = 0; item < resultArrayCombo->count(); ++item)
                    {
                        if (resultArrayCombo->itemData(item, Qt::UserRole).toInt() == scalarMode &&
                            resultArrayCombo->itemData(item, Qt::UserRole + 1).toString() == name)
                        {
                            resultArrayCombo->setCurrentIndex(item);
                            updateComponentCombo();
                            applyVtuColoring();
                            if (renderWindow) renderWindow->Render();
                            break;
                        }
                    }
                }
            });
        }
    };

    if (data->GetPointData())
        addArrayActions(colorByMenu, "Point Data", VTK_SCALAR_MODE_USE_POINT_FIELD_DATA,
            data->GetPointData()->GetNumberOfArrays(),
            [data](int i) { return data->GetPointData()->GetArrayName(i); });
    if (data->GetCellData())
        addArrayActions(colorByMenu, "Cell Data", VTK_SCALAR_MODE_USE_CELL_FIELD_DATA,
            data->GetCellData()->GetNumberOfArrays(),
            [data](int i) { return data->GetCellData()->GetArrayName(i); });
}

void MainWindow::saveViewportScreenshot()
{
    if (!vtkWidget) return;
    const QString filename = QFileDialog::getSaveFileName(
        this, "Save Viewport Screenshot", "viewport.png",
        "PNG Image (*.png);;JPEG Image (*.jpg *.jpeg);;BMP Image (*.bmp)");
    if (filename.isEmpty()) return;
    if (!vtkWidget->grab().save(filename))
        QMessageBox::warning(this, "Screenshot Failed",
                             "Could not save the viewport screenshot.");
}

void MainWindow::applyViewportBackground(const QString &preset, const QColor &customColor)
{
    if (!renderer) return;
    if (preset == "Default")
    {
        renderer->SetGradientBackground(true);
        renderer->SetBackground2(74.0 / 255.0, 120.0 / 255.0, 168.0 / 255.0);
        renderer->SetBackground(159.0 / 255.0, 165.0 / 255.0, 171.0 / 255.0);
    }
    else
    {
        const QColor color = preset == "Black" ? QColor(Qt::black)
                           : preset == "White" ? QColor(Qt::white)
                           : (customColor.isValid() ? customColor : QColor(Qt::white));
        renderer->SetGradientBackground(false);
        renderer->SetBackground(color.redF(), color.greenF(), color.blueF());
        renderer->SetBackground2(color.redF(), color.greenF(), color.blueF());
    }
    if (renderWindow) renderWindow->Render();
}

void MainWindow::showViewportControlDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("ViewPort Control");
    dialog.setModal(true);
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *presetCombo = new QComboBox(&dialog);
    presetCombo->addItems({"Default", "Black", "White", "Custom"});
    auto *colorEdit = new QLineEdit("#9FA5AB", &dialog);
    colorEdit->setPlaceholderText("Color name or #RRGGBB");
    auto *colorButton = new QPushButton("Choose Color...", &dialog);
    auto *colorRow = new QWidget(&dialog);
    auto *colorRowLayout = new QHBoxLayout(colorRow);
    colorRowLayout->setContentsMargins(0, 0, 0, 0);
    colorRowLayout->addWidget(colorEdit, 1);
    colorRowLayout->addWidget(colorButton);
    form->addRow("Background:", presetCombo);
    form->addRow("Color:", colorRow);

    auto makeRgbSpin = [&dialog](int value)
    {
        auto *spin = new QSpinBox(&dialog);
        spin->setRange(0, 255);
        spin->setValue(value);
        return spin;
    };
    auto *redSpin = makeRgbSpin(159);
    auto *greenSpin = makeRgbSpin(165);
    auto *blueSpin = makeRgbSpin(171);
    auto *rgbRow = new QWidget(&dialog);
    auto *rgbLayout = new QHBoxLayout(rgbRow);
    rgbLayout->setContentsMargins(0, 0, 0, 0);
    rgbLayout->addWidget(new QLabel("R", rgbRow)); rgbLayout->addWidget(redSpin);
    rgbLayout->addWidget(new QLabel("G", rgbRow)); rgbLayout->addWidget(greenSpin);
    rgbLayout->addWidget(new QLabel("B", rgbRow)); rgbLayout->addWidget(blueSpin);
    form->addRow("RGB:", rgbRow);
    layout->addLayout(form);

    auto updateColorControls = [=](const QColor &color)
    {
        if (!color.isValid()) return;
        colorEdit->setText(color.name(QColor::HexRgb));
        redSpin->setValue(color.red());
        greenSpin->setValue(color.green());
        blueSpin->setValue(color.blue());
    };
    auto updateFromText = [=]()
    {
        const QColor color(colorEdit->text().trimmed());
        if (!color.isValid()) return;
        const QSignalBlocker b1(redSpin), b2(greenSpin), b3(blueSpin);
        redSpin->setValue(color.red());
        greenSpin->setValue(color.green());
        blueSpin->setValue(color.blue());
        presetCombo->setCurrentText("Custom");
    };
    connect(colorButton, &QPushButton::clicked, &dialog,
            [&, colorEdit, redSpin, greenSpin, blueSpin, updateColorControls,
             presetCombo]() {
              QColor initial(colorEdit->text().trimmed());
              if (!initial.isValid())
                initial = QColor(redSpin->value(), greenSpin->value(),
                                 blueSpin->value());
              const QColor chosen = QColorDialog::getColor(
                  initial, &dialog, "Choose Background Color",
                  QColorDialog::ShowAlphaChannel);
              updateColorControls(chosen);
              if (chosen.isValid())
                presetCombo->setCurrentText("Custom");
            });
    connect(colorEdit, &QLineEdit::editingFinished, &dialog, updateFromText);
    auto syncTextFromRgb = [=]()
    {
        const QColor color(redSpin->value(), greenSpin->value(), blueSpin->value());
        const QSignalBlocker blocker(colorEdit);
        colorEdit->setText(color.name(QColor::HexRgb));
        presetCombo->setCurrentText("Custom");
    };
    connect(redSpin, qOverload<int>(&QSpinBox::valueChanged), &dialog,
            [=](int) { syncTextFromRgb(); });
    connect(greenSpin, qOverload<int>(&QSpinBox::valueChanged), &dialog,
            [=](int) { syncTextFromRgb(); });
    connect(blueSpin, qOverload<int>(&QSpinBox::valueChanged), &dialog,
            [=](int) { syncTextFromRgb(); });
    connect(presetCombo, &QComboBox::currentTextChanged, &dialog,
            [=](const QString &preset)
    {
        const bool custom = preset == "Custom";
        colorEdit->setEnabled(custom); colorButton->setEnabled(custom);
        redSpin->setEnabled(custom); greenSpin->setEnabled(custom); blueSpin->setEnabled(custom);
    });
    presetCombo->setCurrentText("Default");

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted)
    {
        QColor custom(colorEdit->text().trimmed());
        if (!custom.isValid())
            custom = QColor(redSpin->value(), greenSpin->value(), blueSpin->value());
        applyViewportBackground(presetCombo->currentText(), custom);
    }
}

void MainWindow::updateBoundaryFamilies()
{
    if (!boundaryList)
        return;

    boundaryList->clear();
    if (!meshData.isValid()) {
        boundaryList->addItem("Load a mesh to detect boundary markers.");
        boundaryList->item(0)->setFlags(Qt::NoItemFlags);
        return;
    }

    if (meshData.boundaries.isEmpty()) {
        boundaryList->addItem("No boundary markers in this mesh.");
        boundaryList->item(0)->setFlags(Qt::NoItemFlags);
        return;
    }

    for (const SU2Boundary &boundary : meshData.boundaries) {
        auto *item = new QListWidgetItem(
            QString("%1  (%2 elements)")
                .arg(boundary.name)
                .arg(boundary.elements.size()), boundaryList);
        item->setData(Qt::UserRole, boundary.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setToolTip(QString("Show/hide boundary marker '%1' in the viewport")
                             .arg(boundary.name));
    }
}

void MainWindow::createCentralWidget()
{
  QWidget *central = new QWidget(this);

  QVBoxLayout *rootLayout = new QVBoxLayout(central);
  rootLayout->setContentsMargins(6, 6, 6, 6);
  rootLayout->setSpacing(5);

  QSplitter *verticalSplitter = new QSplitter(Qt::Vertical);
  verticalSplitter->setChildrenCollapsible(false);

  // =================================================
  // MAIN HORIZONTAL SPLITTER
  //
  // LEFT  = 2/5
  // RIGHT = 3/5
  // =================================================

  QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
  mainSplitter->setChildrenCollapsible(false);
  mainSplitter->setMinimumHeight(350);

  // =================================================
  // LEFT PROPERTY AREA
  // =================================================

  QWidget *propertyArea = new QWidget;
  propertyArea->setObjectName("propertyArea");

  propertyArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  QVBoxLayout *propertyAreaLayout = new QVBoxLayout(propertyArea);
  propertyAreaLayout->setContentsMargins(0, 0, 0, 0);
  propertyAreaLayout->setSpacing(5);

  // =================================================
  // PROPERTY CONTAINER
  // =================================================

  propertyContainer = new QFrame;
  propertyContainer->setObjectName("propertyContainer");

  propertyContainer->setFrameShape(QFrame::Panel);
  propertyContainer->setFrameShadow(QFrame::Sunken);
  propertyContainer->setLineWidth(2);
  propertyContainer->setMidLineWidth(1);

  propertyContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  propertyLayout = new QVBoxLayout(propertyContainer);
  propertyLayout->setContentsMargins(10, 8, 10, 8);
  propertyLayout->setSpacing(8);

  // =================================================
  // SECTION TITLE
  // =================================================

  sectionTitle = new QLabel;
  sectionTitle->setObjectName("sectionTitle");
  sectionTitle->setText("Properties");

  propertyLayout->addWidget(sectionTitle, 0);

  // =================================================
  // DIRECTORY ROW
  // =================================================

  directoryRow = new QWidget(propertyContainer);
  directoryRow->setObjectName("directoryRow");

  QHBoxLayout *directoryLayout = new QHBoxLayout(directoryRow);
  directoryLayout->setContentsMargins(0, 0, 0, 0);
  directoryLayout->setSpacing(5);

  QLabel *directoryLabel = new QLabel("Directory:");

  directoryEdit = new QLineEdit;
  directoryEdit->setReadOnly(true);
  directoryEdit->setPlaceholderText("Select working directory...");

  QPushButton *browseButton = new QPushButton("Browse...");

  directoryLayout->addWidget(directoryLabel);

  directoryLayout->addWidget(directoryEdit, 1);

  directoryLayout->addWidget(browseButton);

  propertyLayout->addWidget(directoryRow, 0);

  connect(browseButton, &QPushButton::clicked, this,
          &MainWindow::selectWorkingDirectory);

  // =================================================
  // DYNAMIC PROPERTY AREA
  // =================================================

  QWidget *dynamicProperties = new QWidget(propertyContainer);
  dynamicProperties->setObjectName("dynamicProperties");

  dynamicProperties->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Expanding);

  QVBoxLayout *dynamicLayout = new QVBoxLayout(dynamicProperties);
  dynamicLayout->setContentsMargins(0, 0, 0, 0);
  dynamicLayout->setSpacing(0);

  propertyPanel = new QPlainTextEdit(dynamicProperties);
  propertyPanel->setObjectName("propertyPanel");

  propertyPanel->setReadOnly(false);

  propertyPanel->setLineWrapMode(QPlainTextEdit::NoWrap);

  propertyPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  propertyPanel->setFrameShape(QFrame::Panel);

  propertyPanel->setFrameShadow(QFrame::Sunken);

  propertyPanel->setLineWidth(1);
  propertyPanel->setMidLineWidth(0);

  dynamicLayout->addWidget(propertyPanel, 1);

  propertyLayout->addWidget(dynamicProperties, 1);

  // =================================================
  // ADD PROPERTY CONTAINER
  // =================================================

  propertyAreaLayout->addWidget(propertyContainer, 1);

  // =================================================
  // ACTION GROUP
  //
  // Same width as propertyContainer because both
  // occupy the complete propertyArea width.
  // =================================================

  QGroupBox *actionGroup = new QGroupBox;
  actionGroup->setObjectName("actionGroup");

  actionGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  actionGroup->setFixedHeight(42);

  QHBoxLayout *actionLayout = new QHBoxLayout(actionGroup);

  actionLayout->setContentsMargins(8, 4, 8, 4);

  actionLayout->setSpacing(10);

  QPushButton *okButton = new QPushButton("OK");
  QPushButton *cancelButton = new QPushButton("Cancel");

  okButton->setMinimumWidth(85);
  cancelButton->setMinimumWidth(85);

  actionLayout->addStretch(1);

  actionLayout->addWidget(okButton);

  actionLayout->addWidget(cancelButton);

  actionLayout->addStretch(1);

  propertyAreaLayout->addWidget(actionGroup, 0);

  // =================================================
  // OK
  // =================================================

  connect(okButton, &QPushButton::clicked, this, [this]() {
    commitCurrentSection();
    goToNextSection();
  });

  // =================================================
  // CANCEL
  // =================================================

  connect(cancelButton, &QPushButton::clicked, this,
          [this]() { restoreSectionSnapshot(); });

  // =================================================
  // ADD LEFT SIDE
  // =================================================

  mainSplitter->addWidget(propertyArea);

  // =================================================
  // RIGHT SIDE
  // =================================================

  QWidget *viewportArea = new QWidget;
  viewportArea->setObjectName("viewportArea");

  viewportArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  QVBoxLayout *viewportLayout = new QVBoxLayout(viewportArea);
  viewportLayout->setContentsMargins(0, 0, 0, 0);
  viewportLayout->setSpacing(5);

  // =================================================
  // NAVIGATION
  // =================================================

  createNavigation();

  navigationTabs->setMinimumHeight(110);
  navigationTabs->setSizePolicy(QSizePolicy::Expanding,
                                QSizePolicy::Preferred);
  updateBoundaryFamilies();

  // =================================================
  // VTK VIEWPORT
  // =================================================

  vtkWidget = new QVTKOpenGLNativeWidget;

  vtkWidget->setMinimumHeight(250);

  vtkWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  // =================================================
  // RIGHT SIDE LAYOUT
  // =================================================

  QSplitter *rightSplitter = new QSplitter(Qt::Vertical);

  rightSplitter->setChildrenCollapsible(false);

  rightSplitter->addWidget(navigationTabs);

  rightSplitter->addWidget(vtkWidget);

  rightSplitter->setStretchFactor(0, 0);

  rightSplitter->setStretchFactor(1, 1);

  rightSplitter->setSizes({180, 640});

  viewportLayout->addWidget(rightSplitter, 1);

  // =================================================
  // ADD RIGHT SIDE
  // =================================================

  mainSplitter->addWidget(viewportArea);

  // =================================================
  // MAIN SPLITTER RATIO
  //
  // DO NOT USE:
  //
  // setSizes({2, 3})
  //
  // because setSizes() uses PIXELS.
  //
  // Stretch factors establish the 2:3 relationship.
  // =================================================

  mainSplitter->setStretchFactor(0, 2);

  mainSplitter->setStretchFactor(1, 3);

  // =================================================
  // INITIAL MAIN SPLITTER SIZE
  // =================================================

  mainSplitter->setSizes({450, 840});

  // =================================================
  // ADD MAIN SPLITTER
  // =================================================

  verticalSplitter->addWidget(mainSplitter);

  // =================================================
  // SHELL LOG CONTAINER
  // =================================================

  QWidget *shellLogContainer = new QWidget;

  shellLogContainer->setObjectName("shellLogContainer");

  QVBoxLayout *shellLogLayout = new QVBoxLayout(shellLogContainer);

  shellLogLayout->setContentsMargins(0, 0, 0, 0);

  shellLogLayout->setSpacing(0);

  // =================================================
  // SHELL LOG TITLE BAR
  // =================================================

  QFrame *shellLogTitleBar = new QFrame;

  shellLogTitleBar->setObjectName("shellLogTitleBar");

  shellLogTitleBar->setFrameShape(QFrame::Panel);

  shellLogTitleBar->setFrameShadow(QFrame::Raised);

  shellLogTitleBar->setLineWidth(1);
  shellLogTitleBar->setMidLineWidth(0);

  shellLogTitleBar->setFixedHeight(25);

  QPalette shellPalette = shellLogTitleBar->palette();

  shellLogTitleBar->setPalette(shellPalette);

  shellLogTitleBar->setAutoFillBackground(true);

  QHBoxLayout *shellLogTitleLayout = new QHBoxLayout(shellLogTitleBar);

  shellLogTitleLayout->setContentsMargins(10, 0, 10, 0);

  shellLogTitleLayout->setSpacing(6);

  QLabel *shellLogTitle = new QLabel("shellLog");

  shellLogTitle->setObjectName("shellLogTitle");

  QPalette titlePalette = shellLogTitle->palette();

  titlePalette.setColor(QPalette::Window,
                        titlePalette.color(QPalette::Highlight));

  shellLogTitle->setPalette(titlePalette);

  QFont titleFont = shellLogTitle->font();

  titleFont.setBold(true);

  shellLogTitle->setFont(titleFont);

  shellLogTitleLayout->addWidget(shellLogTitle);

  shellLogTitleLayout->addStretch();

  shellLogLayout->addWidget(shellLogTitleBar);

  // =================================================
  // TERMINAL
  // =================================================

  terminal = new QPlainTextEdit;

  terminal->setReadOnly(true);

  terminal->setLineWrapMode(QPlainTextEdit::NoWrap);

  terminal->setObjectName("terminal");

  terminal->setFrameShape(QFrame::Panel);

  terminal->setFrameShadow(QFrame::Sunken);

  terminal->setLineWidth(2);
  terminal->setMidLineWidth(0);

  shellLogLayout->addWidget(terminal, 1);

  // =================================================
  // ADD SHELL LOG
  // =================================================

  verticalSplitter->addWidget(shellLogContainer);

  // =================================================
  // TERMINAL DEFAULT SIZE
  //
  // Very small initially.
  // User can still drag the splitter.
  // =================================================

  verticalSplitter->setSizes({820, 70});

  verticalSplitter->setStretchFactor(0, 1);

  verticalSplitter->setStretchFactor(1, 0);

  // =================================================
  // ROOT
  // =================================================

  rootLayout->addWidget(verticalSplitter, 1);

  setCentralWidget(central);
}

// void MainWindow::createViewToolbar()
// {
//     QToolBar *toolbar = new QToolBar("Mesh View", this);
//     toolbar->setMovable(false);
//     toolbar->setFloatable(false);
//     toolbar->setIconSize(QSize(16, 16));

//     QAction *viewAction = toolbar->addAction("View");
//     QAction *selectAction = toolbar->addAction("Select");
//     viewAction->setObjectName("vtuViewModeAction");
//     selectAction->setObjectName("vtuSelectModeAction");
//     viewAction->setCheckable(true);
//     selectAction->setCheckable(true);
//     QActionGroup *interactionGroup = new QActionGroup(toolbar);
//     interactionGroup->setExclusive(true);
//     interactionGroup->addAction(viewAction);
//     interactionGroup->addAction(selectAction);
//     viewAction->setChecked(true);

//     toolbar->addSeparator();
//     toolbar->addWidget(new QLabel("Display:"));
//     displayModeCombo = new QComboBox(toolbar);
//     displayModeCombo->addItems({
//         "Surface",
//         "Surface + Edges",
//         "Wireframe",
//         "Points"
//     });
//     displayModeCombo->setCurrentIndex(0);
//     meshVisualization.setDisplayMode(MeshVisualization::DisplayMode::Surface);

//     toolbar->addSeparator();
//     toolbar->addWidget(new QLabel("Selection:"));
//     selectionModeCombo = new QComboBox(toolbar);
//     selectionModeCombo->addItems({"Node", "Edge", "Face", "Element"});
//     selectionModeCombo->setCurrentIndex(0);
//     toolbar->addWidget(selectionModeCombo);

//     toolbar->addSeparator();
//     toolbar->addAction("Fit", this, [this]() { meshVisualization.fitView(); });
//     toolbar->addAction("XY", this, &MainWindow::viewXY);
//     toolbar->addAction("YZ", this, &MainWindow::viewYZ);
//     toolbar->addAction("XZ", this, &MainWindow::viewXZ);
//     toolbar->addAction("Fit+XY", this, &MainWindow::fitXY);
//     toolbar->addAction("Fit+YZ", this, &MainWindow::fitYZ);
//     toolbar->addAction("Fit+XZ", this, &MainWindow::fitXZ);
// /*    toolbar->addAction("Clear Selection", this, [this]() {
//         meshVisualization.clearSelection();
//     });*/
//     toolbar->addAction("Clear Selection", this, [this]() {
//         meshVisualization.clearSelection();
//         if (vtuSelectionActor && renderer) {
//             renderer->RemoveActor(vtuSelectionActor);
//             vtuSelectionActor = nullptr;
//         }
//         if (renderWindow)
//             renderWindow->Render();
//     });

//     // ParaView-like result controls. They are shown only on the Post Processing tab.
//     toolbar->addSeparator();
//     resultArrayLabel = new QLabel("Result:", toolbar);
//     resultArrayCombo = new QComboBox(toolbar);
//     resultArrayCombo->setMinimumWidth(150);
//     resultArrayCombo->setMaximumWidth(220);
//     componentLabel = new QLabel("Component:", toolbar);
//     componentCombo = new QComboBox(toolbar);
//     componentCombo->setMinimumWidth(110);
//     toolbar->addWidget(resultArrayLabel);
//     toolbar->addWidget(resultArrayCombo);
//     toolbar->addWidget(componentLabel);
//     toolbar->addWidget(componentCombo);
//     rescaleDataButton = new QToolButton(toolbar);
//     rescaleDataButton->setText("Rescale Data");
//     rescaleVisibleButton = new QToolButton(toolbar);
//     rescaleVisibleButton->setText("Rescale Visible");
//     toolbar->addWidget(rescaleDataButton);
//     toolbar->addWidget(rescaleVisibleButton);

//     toolbar->addSeparator();
//     firstVtuFrameAction = toolbar->addAction("|<");
//     previousVtuFrameAction = toolbar->addAction("<");
//     playPauseButton = new QToolButton(toolbar);
//     playPauseButton->setText("Play");
//     toolbar->addWidget(playPauseButton);
//     nextVtuFrameAction = toolbar->addAction(">");
//     lastVtuFrameAction = toolbar->addAction(">|");
//     timeSlider = new QSlider(Qt::Horizontal, toolbar);
//     timeSlider->setMinimumWidth(130);
//     timeSlider->setMaximumWidth(240);
//     toolbar->addWidget(timeSlider);

//     QWidget *controls[] = {
//         resultArrayLabel,
//         resultArrayCombo,
//         componentLabel,
//         componentCombo,
//         timeSlider,
//         playPauseButton
//     };

//     // for (QWidget *w : {static_cast<QWidget *>(resultArrayLabel), resultArrayCombo,
//     //                    componentLabel, componentCombo, timeSlider, playPauseButton})
//      for (QWidget *w : controls){  w->setVisible(false);}
//     rescaleDataButton->setVisible(false);
//     rescaleVisibleButton->setVisible(false);
//     firstVtuFrameAction->setVisible(false);
//     previousVtuFrameAction->setVisible(false);
//     nextVtuFrameAction->setVisible(false);
//     lastVtuFrameAction->setVisible(false);

//     connect(resultArrayCombo, qOverload<int>(&QComboBox::currentIndexChanged),
//             this, [this](int) { updateComponentCombo(); applyVtuColoring(); if (renderWindow) renderWindow->Render(); });
//     connect(componentCombo, qOverload<int>(&QComboBox::currentIndexChanged),
//             this, [this](int) { applyVtuColoring(); if (renderWindow) renderWindow->Render(); });
//     connect(rescaleDataButton, &QToolButton::clicked,
//             this, &MainWindow::rescaleVtuToDataRange);
//     connect(rescaleVisibleButton, &QToolButton::clicked,
//             this, &MainWindow::rescaleVtuToVisibleRange);
//     connect(playPauseButton, &QToolButton::clicked, this, &MainWindow::toggleVtuAnimation);
//     connect(firstVtuFrameAction, &QAction::triggered, this, [this]() { setVtuFrame(0); });
//     connect(previousVtuFrameAction, &QAction::triggered, this, [this]() { setVtuFrame(qMax(0, currentVtuIndex - 1)); });
//     connect(nextVtuFrameAction, &QAction::triggered, this, [this]() { setVtuFrame(qMin(vtuFiles.size() - 1, currentVtuIndex + 1)); });
//     connect(lastVtuFrameAction, &QAction::triggered, this, [this]() { setVtuFrame(vtuFiles.size() - 1); });
//     connect(timeSlider, &QSlider::valueChanged, this, &MainWindow::setVtuFrame);

//     addToolBar(Qt::TopToolBarArea, toolbar);

//     connect(displayModeCombo,
//             qOverload<int>(&QComboBox::currentIndexChanged),
//             this,
//             [this](int index)
//             {
//                 if (index < 0 || index > 3)
//                     return;

//                 if (postProcessingActor)
//                 {
//                     setVtuRepresentation(index);
//                     return;
//                 }

//                 using DM = MeshVisualization::DisplayMode;
//                 const DM modes[] = {DM::Surface, DM::SurfaceWithEdges,
//                                     DM::Wireframe, DM::Points};
//                 meshVisualization.setDisplayMode(modes[index]);
//             });

//     connect(selectionModeCombo,
//             qOverload<int>(&QComboBox::currentIndexChanged),
//             this,
//             [this](int index)
//             {
//                 if (postProcessingActor)
//                     return; // VTU picking is handled by the VTK pickers.
//                 using SM = MeshVisualization::SelectionMode;
//                 const SM modes[] = {SM::Node, SM::Edge, SM::Face, SM::Element};
//                 if (index >= 0 && index < 4)
//                     meshVisualization.setSelectionMode(modes[index]);
//             });

//     connect(viewAction, &QAction::triggered, this, [this]()
//     {
//         if (postProcessingActor)
//         {
//             vtuSelectionEnabled = false;
//             return;
//         }
//         meshVisualization.setInteractionMode(
//             MeshVisualization::InteractionMode::View);
//     });

//     connect(selectAction, &QAction::triggered, this, [this]()
//     {
//         if (postProcessingActor)
//         {
//             vtuSelectionEnabled = true;
//             return;
//         }
//         meshVisualization.setInteractionMode(
//             MeshVisualization::InteractionMode::Select);
//     });
// }


void MainWindow::createViewToolbar()
{
    QToolBar *toolbar = new QToolBar("Mesh View", this);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize(QSize(16, 16));

    // =================================================
    // VIEW / SELECT
    // =================================================

    QAction *viewAction = toolbar->addAction("View");
    QAction *selectAction = toolbar->addAction("Select");

    viewAction->setObjectName("vtuViewModeAction");
    selectAction->setObjectName("vtuSelectModeAction");

    viewAction->setCheckable(true);
    selectAction->setCheckable(true);

    QActionGroup *interactionGroup = new QActionGroup(toolbar);
    interactionGroup->setExclusive(true);
    interactionGroup->addAction(viewAction);
    interactionGroup->addAction(selectAction);

    viewAction->setChecked(true);

    // =================================================
    // DISPLAY MODE
    // =================================================

    toolbar->addSeparator();

    QLabel *displayLabel = new QLabel("Display:", toolbar);
    displayLabel->setFixedWidth(55);
    toolbar->addWidget(displayLabel);

    displayModeCombo = new QComboBox(toolbar);

    displayModeCombo->addItems({
        "Surface",
        "Surface + Edges",
        "Wireframe",
        "Points"
    });

    displayModeCombo->setCurrentIndex(0);
    displayModeCombo->setMinimumWidth(125);
    displayModeCombo->setMaximumWidth(145);

    toolbar->addWidget(displayModeCombo);

    meshVisualization.setDisplayMode(
        MeshVisualization::DisplayMode::Surface);

    // =================================================
    // SELECTION MODE
    // =================================================

    toolbar->addSeparator();

    QLabel *selectionLabel = new QLabel("Selection:", toolbar);
    selectionLabel->setFixedWidth(70);
    toolbar->addWidget(selectionLabel);

    selectionModeCombo = new QComboBox(toolbar);

    selectionModeCombo->addItems({
        "Node",
        "Edge",
        "Face",
        "Element"
    });

    selectionModeCombo->setCurrentIndex(0);
    selectionModeCombo->setMinimumWidth(100);
    selectionModeCombo->setMaximumWidth(120);

    toolbar->addWidget(selectionModeCombo);

    // =================================================
    // VIEW CONTROLS
    // =================================================

    toolbar->addSeparator();

    toolbar->addAction("Fit", this,
                       [this]() { meshVisualization.fitView(); });

    toolbar->addAction("XY", this, &MainWindow::viewXY);
    toolbar->addAction("YZ", this, &MainWindow::viewYZ);
    toolbar->addAction("XZ", this, &MainWindow::viewXZ);

    toolbar->addAction("Fit+XY", this, &MainWindow::fitXY);
    toolbar->addAction("Fit+YZ", this, &MainWindow::fitYZ);
    toolbar->addAction("Fit+XZ", this, &MainWindow::fitXZ);

    // =================================================
    // CLEAR SELECTION
    // =================================================

    toolbar->addAction("Clear Selection", this, [this]()
    {
        meshVisualization.clearSelection();

        if (vtuSelectionActor && renderer)
        {
            renderer->RemoveActor(vtuSelectionActor);
            vtuSelectionActor = nullptr;
        }

        if (renderWindow)
            renderWindow->Render();
    });

    // =================================================
    // POST-PROCESSING RESULT CONTROLS
    // =================================================

    toolbar->addSeparator();

    resultArrayLabel = new QLabel("Result:", toolbar);

    resultArrayCombo = new QComboBox(toolbar);
    resultArrayCombo->setMinimumWidth(150);
    resultArrayCombo->setMaximumWidth(220);

    componentLabel = new QLabel("Component:", toolbar);

    componentCombo = new QComboBox(toolbar);
    componentCombo->setMinimumWidth(110);

    toolbar->addWidget(resultArrayLabel);
    toolbar->addWidget(resultArrayCombo);
    toolbar->addWidget(componentLabel);
    toolbar->addWidget(componentCombo);

    rescaleDataButton = new QToolButton(toolbar);
    rescaleDataButton->setText("Rescale Data");

    rescaleVisibleButton = new QToolButton(toolbar);
    rescaleVisibleButton->setText("Rescale Visible");

    toolbar->addWidget(rescaleDataButton);
    toolbar->addWidget(rescaleVisibleButton);

    // =================================================
    // VTU ANIMATION CONTROLS
    // =================================================

    toolbar->addSeparator();

    firstVtuFrameAction = toolbar->addAction("|<");
    previousVtuFrameAction = toolbar->addAction("<");

    playPauseButton = new QToolButton(toolbar);
    playPauseButton->setText("Play");
    toolbar->addWidget(playPauseButton);

    nextVtuFrameAction = toolbar->addAction(">");
    lastVtuFrameAction = toolbar->addAction(">|");

    timeSlider = new QSlider(Qt::Horizontal, toolbar);
    timeSlider->setMinimumWidth(130);
    timeSlider->setMaximumWidth(240);

    toolbar->addWidget(timeSlider);

    // =================================================
    // HIDE POST-PROCESSING CONTROLS INITIALLY
    // =================================================

    QWidget *controls[] = {
        resultArrayLabel,
        resultArrayCombo,
        componentLabel,
        componentCombo,
        timeSlider,
        playPauseButton
    };

    for (QWidget *w : controls)
        w->setVisible(false);

    rescaleDataButton->setVisible(false);
    rescaleVisibleButton->setVisible(false);

    firstVtuFrameAction->setVisible(false);
    previousVtuFrameAction->setVisible(false);
    nextVtuFrameAction->setVisible(false);
    lastVtuFrameAction->setVisible(false);

    // =================================================
    // RESULT SIGNALS
    // =================================================

    connect(resultArrayCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int)
            {
                updateComponentCombo();
                applyVtuColoring();

                if (renderWindow)
                    renderWindow->Render();
            });

    connect(componentCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int)
            {
                applyVtuColoring();

                if (renderWindow)
                    renderWindow->Render();
            });

    connect(rescaleDataButton, &QToolButton::clicked,
            this, &MainWindow::rescaleVtuToDataRange);

    connect(rescaleVisibleButton, &QToolButton::clicked,
            this, &MainWindow::rescaleVtuToVisibleRange);

    connect(playPauseButton, &QToolButton::clicked,
            this, &MainWindow::toggleVtuAnimation);

    connect(firstVtuFrameAction, &QAction::triggered,
            this, [this]() { setVtuFrame(0); });

    connect(previousVtuFrameAction, &QAction::triggered,
            this, [this]()
            {
                setVtuFrame(qMax(0, currentVtuIndex - 1));
            });

    connect(nextVtuFrameAction, &QAction::triggered,
            this, [this]()
            {
                setVtuFrame(qMin(vtuFiles.size() - 1,
                                 currentVtuIndex + 1));
            });

    connect(lastVtuFrameAction, &QAction::triggered,
            this, [this]()
            {
                setVtuFrame(vtuFiles.size() - 1);
            });

    connect(timeSlider, &QSlider::valueChanged,
            this, &MainWindow::setVtuFrame);

    // =================================================
    // ADD TOOLBAR
    // =================================================

    addToolBar(Qt::TopToolBarArea, toolbar);

    // =================================================
    // DISPLAY MODE CONNECTION
    // =================================================

    connect(displayModeCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int index)
            {
                if (index < 0 || index > 3)
                    return;

                if (postProcessingActor)
                {
                    setVtuRepresentation(index);
                    return;
                }

                using DM = MeshVisualization::DisplayMode;

                const DM modes[] = {
                    DM::Surface,
                    DM::SurfaceWithEdges,
                    DM::Wireframe,
                    DM::Points
                };

                meshVisualization.setDisplayMode(modes[index]);
            });

    // =================================================
    // SELECTION MODE CONNECTION
    // =================================================

    connect(selectionModeCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int index)
            {
                if (postProcessingActor)
                    return;

                using SM = MeshVisualization::SelectionMode;

                const SM modes[] = {
                    SM::Node,
                    SM::Edge,
                    SM::Face,
                    SM::Element
                };

                if (index >= 0 && index < 4)
                    meshVisualization.setSelectionMode(modes[index]);
            });

    // =================================================
    // VIEW / SELECT CONNECTIONS
    // =================================================

    connect(viewAction, &QAction::triggered, this, [this]()
    {
        if (postProcessingActor)
        {
            vtuSelectionEnabled = false;
            return;
        }

        meshVisualization.setInteractionMode(
            MeshVisualization::InteractionMode::View);
    });

    connect(selectAction, &QAction::triggered, this, [this]()
    {
        if (postProcessingActor)
        {
            vtuSelectionEnabled = true;
            return;
        }

        meshVisualization.setInteractionMode(
            MeshVisualization::InteractionMode::Select);
    });
}

void MainWindow::showWorkingDirectorySection()
{
    showSection("Working Directory");
}

void MainWindow::showMeshSection()
{
    showSection("Mesh");
}

void MainWindow::showPhysicsSection()
{
    showSection("Physics");
}

void MainWindow::showNumericsSection()
{
    showSection("Numerics");
}

void MainWindow::showBoundariesSection()
{
    showSection("Boundaries");
}

void MainWindow::showSection(const QString &section)
{
    if (!propertyContainer || !propertyLayout)
        return;

    if (sectionTitle)
        sectionTitle->setText(section);

    if (directoryRow)
        directoryRow->setVisible(section == "Working Directory");

    // Rebuild the section-specific area as one child widget.  This is safer
    // than manually deleting widgets from nested layouts because Qt owns
    // those widgets through the parent/layout hierarchy.
    QWidget *dynamicProperties = propertyContainer->findChild<QWidget *>("dynamicProperties");

    if (dynamicProperties)
    {
        int index = propertyLayout->indexOf(dynamicProperties);
        if (index < 0)
            return;

        QLayoutItem *oldItem = propertyLayout->takeAt(index);
        QWidget *oldWidget = oldItem ? oldItem->widget() : nullptr;
        delete oldItem;
        delete oldWidget;
    }

    dynamicProperties = new QWidget(propertyContainer);
    dynamicProperties->setObjectName("dynamicProperties");
    dynamicProperties->setSizePolicy(QSizePolicy::Expanding,
                                     QSizePolicy::Expanding);

    QVBoxLayout *dynamicLayout = new QVBoxLayout(dynamicProperties);
    dynamicLayout->setContentsMargins(0, 0, 0, 0);
    dynamicLayout->setSpacing(8);

    propertyLayout->addWidget(dynamicProperties, 1);

    QVBoxLayout *layout = dynamicLayout;

    auto addField = [dynamicProperties](QFormLayout *form,
                                        const QString &label,
                                        const QString &value)
    {
        QLineEdit *edit = new QLineEdit(dynamicProperties);
        edit->setReadOnly(true);
        edit->setText(value);
        form->addRow(label, edit);
    };

    auto addHeading = [dynamicProperties](QVBoxLayout *layout,
                                          const QString &text)
    {
        QLabel *label = new QLabel(text, dynamicProperties);
        label->setObjectName("propertyHeading");
        layout->addWidget(label);
    };

    if (section == "Working Directory")
    {
        QFormLayout *form = new QFormLayout;
        form->setContentsMargins(4, 4, 4, 4);
        form->setHorizontalSpacing(12);
        form->setVerticalSpacing(8);

        addField(form,
                 "Configuration",
                 configFile.isEmpty()
                     ? "<not selected>"
                     : QFileInfo(configFile).fileName());

        dynamicProperties->layout()->addItem(form);
        dynamicProperties->layout()->addItem(
            new QSpacerItem(0, 0,
                            QSizePolicy::Minimum,
                            QSizePolicy::Expanding));
        return;
    }

    // Hide the directory controls for every other section.
    if (section == "Mesh")
    {
        QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(
            dynamicProperties->layout());

        addHeading(layout, "Mesh Information");

        QFormLayout *form = new QFormLayout;
        form->setHorizontalSpacing(12);
        form->setVerticalSpacing(8);

        addField(form,
                 "File",
                 meshFile.isEmpty() ? "<not loaded>" : meshFile);
        addField(form, "Format", "SU2");

        if (meshData.isValid())
        {
            addField(form,
                     "Dimension",
                     QString("%1D").arg(meshData.dimension));
            addField(form,
                     "Points",
                     QString::number(meshData.points.size()));
            addField(form,
                     "Volume elements",
                     QString::number(meshData.volumeElements.size()));
        }
        else
        {
            addField(form, "Mesh data", "<not loaded>");
        }

        layout->addLayout(form);

        if (meshData.isValid())
        {
            QGroupBox *typesBox = new QGroupBox("Volume Element Types",
                                                 dynamicProperties);
            QFormLayout *typesLayout = new QFormLayout(typesBox);

            const int volumeTypes[] = {1, 3, 5, 9, 10, 12, 13, 14};

            for (int type : volumeTypes)
            {
                const int count = meshData.elementCountByType(type);
                if (count > 0)
                {
                    typesLayout->addRow(
                        SU2Mesh::elementTypeName(type),
                        new QLabel(QString::number(count), typesBox));
                }
            }

            layout->addWidget(typesBox);

            QGroupBox *boundaryBox = new QGroupBox("Boundary Markers",
                                                   dynamicProperties);
            QVBoxLayout *boundaryLayout = new QVBoxLayout(boundaryBox);

            QLabel *markerCount = new QLabel(
                QString("Markers: %1").arg(meshData.boundaries.size()),
                boundaryBox);
            boundaryLayout->addWidget(markerCount);

            for (const SU2Boundary &boundary : meshData.boundaries)
            {
                QLabel *boundaryLabel = new QLabel(
                    QString("%1   (%2 elements)")
                        .arg(boundary.name)
                        .arg(boundary.elements.size()),
                    boundaryBox);
                boundaryLayout->addWidget(boundaryLabel);
            }

            layout->addWidget(boundaryBox);
        }

        layout->addItem(new QSpacerItem(0, 0,
                                        QSizePolicy::Minimum,
                                        QSizePolicy::Expanding));
        return;
    }

    if (section == "Physics")
    {
        QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(
            dynamicProperties->layout());
        addHeading(layout, "Physics Parameters");

        QFormLayout *form = new QFormLayout;
        form->setHorizontalSpacing(12);
        form->setVerticalSpacing(8);

        addField(form, "Solver", readConfigValue("SOLVER"));
        addField(form, "Turbulence Model",
                 readConfigValue("KIND_TURB_MODEL"));
        addField(form, "Density Init",
                 readConfigValue("INC_DENSITY_INIT"));
        addField(form, "Velocity Init",
                 readConfigValue("INC_VELOCITY_INIT"));
        addField(form, "Mu Constant",
                 readConfigValue("MU_CONSTANT"));
        addField(form, "Time Domain",
                 readConfigValue("TIME_DOMAIN"));
        addField(form, "Time Marching",
                 readConfigValue("TIME_MARCHING"));

        layout->addLayout(form);
        layout->addItem(new QSpacerItem(0, 0,
                                        QSizePolicy::Minimum,
                                        QSizePolicy::Expanding));
        return;
    }

    if (section == "Numerics")
    {
        QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(
            dynamicProperties->layout());
        addHeading(layout, "Numerical Parameters");

        QFormLayout *form = new QFormLayout;
        form->setHorizontalSpacing(12);
        form->setVerticalSpacing(8);

        addField(form, "Gradient Method",
                 readConfigValue("NUM_METHOD_GRAD"));
        addField(form, "CFL Number",
                 readConfigValue("CFL_NUMBER"));
        addField(form, "Flow Conv. Method",
                 readConfigValue("CONV_NUM_METHOD_FLOW"));
        addField(form, "MUSCL Flow",
                 readConfigValue("MUSCL_FLOW"));
        addField(form, "Time Discretization",
                 readConfigValue("TIME_DISCRE_FLOW"));
        addField(form, "Linear Solver",
                 readConfigValue("LINEAR_SOLVER"));
        addField(form, "Linear Solver Preconditioner",
                 readConfigValue("LINEAR_SOLVER_PREC"));

        layout->addLayout(form);
        layout->addItem(new QSpacerItem(0, 0,
                                        QSizePolicy::Minimum,
                                        QSizePolicy::Expanding));
        return;
    }

    if (section == "Boundaries")
    {
        QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(
            dynamicProperties->layout());
        addHeading(layout, "Boundary Markers");

        if (!meshData.isValid())
        {
            QFormLayout *form = new QFormLayout;
            addField(form, "Mesh data", "<not loaded>");
            layout->addLayout(form);
            layout->addItem(new QSpacerItem(0, 0,
                                            QSizePolicy::Minimum,
                                            QSizePolicy::Expanding));
            return;
        }

        QLabel *countLabel = new QLabel(
            QString("Detected markers: %1")
                .arg(meshData.boundaries.size()),
            dynamicProperties);
        layout->addWidget(countLabel);

        for (const SU2Boundary &boundary : meshData.boundaries)
        {
            QGroupBox *box = new QGroupBox(boundary.name,
                                            dynamicProperties);
            QFormLayout *form = new QFormLayout(box);

            form->addRow("Elements",
                         new QLabel(QString::number(boundary.elements.size()),
                                    box));

            const int boundaryTypes[] = {1, 3, 5, 9, 10, 12, 13, 14};
            for (int type : boundaryTypes)
            {
                int count = 0;
                for (const SU2Element &element : boundary.elements)
                {
                    if (element.type == type)
                        ++count;
                }

                if (count > 0)
                {
                    form->addRow(
                        SU2Mesh::elementTypeName(type),
                        new QLabel(QString::number(count), box));
                }
            }

            layout->addWidget(box);
        }

        layout->addItem(new QSpacerItem(0, 0,
                                        QSizePolicy::Minimum,
                                        QSizePolicy::Expanding));
    }
}

void MainWindow::saveSectionSnapshot()
{
    if (!propertyPanel)
        return;

    currentSection =
        sectionTitle
            ? sectionTitle->text()
            : QString();

    sectionSnapshot =
        propertyPanel->toPlainText();
}

void MainWindow::restoreSectionSnapshot()
{
    if (!propertyPanel)
        return;

    if (currentSection.isEmpty())
        return;

    propertyPanel->setPlainText(
        sectionSnapshot
    );

    propertyPanel->document()->setModified(
        false
    );

    appendOutput(
        QString("Cancelled changes in %1.\n")
            .arg(currentSection)
    );
}

void MainWindow::commitCurrentSection()
{
    if (!propertyPanel)
        return;

    if (currentSection.isEmpty())
        return;

    /*
     * For now, the edited values are accepted in
     * the GUI and become the new snapshot.
     *
     * The next step can write these KEY = VALUE
     * changes back into the SU2 .cfg file.
     */
    sectionSnapshot =
        propertyPanel->toPlainText();

    propertyPanel->document()->setModified(
        false
    );

    appendOutput(
        QString("Accepted changes in %1.\n")
            .arg(currentSection)
    );
}

void MainWindow::goToNextSection()
{
    if (!navigationWidget)
        return;

    QTreeWidgetItem *current =
        navigationWidget->currentItem();

    if (!current)
        return;

    QTreeWidgetItem *parent =
        current->parent();

    if (!parent)
        return;

    const int currentIndex =
        parent->indexOfChild(current);

    const int nextIndex =
        currentIndex + 1;

    if (nextIndex >= parent->childCount())
    {
        appendOutput(
            "All sections completed.\n"
        );

        return;
    }

    QTreeWidgetItem *nextItem =
        parent->child(nextIndex);

    if (!nextItem)
        return;

    navigationWidget->setCurrentItem(
        nextItem
    );
}

void MainWindow::updateWorkingDirectoryPanel()
{
    if (!propertyContainer)
        return;

    // Keep the existing directory field in sync.
    if (directoryEdit)
    {
        directoryEdit->setText(
            caseDirectory.isEmpty() ? QString() : caseDirectory);
    }

    // Rebuild the small configuration section when the working directory
    // information changes.
    if (sectionTitle && sectionTitle->text() == "Working Directory")
        showSection("Working Directory");
}

void MainWindow::showLoading(const QString &message)
{
    if (!loadingOverlay || !loadingBar || !loadingLabel)
        return;

    loadingLabel->setText(message);

    loadingOverlay->adjustSize();

    loadingOverlay->move(
        (centralWidget()->width() - loadingOverlay->width()) / 2,
        (centralWidget()->height() - loadingOverlay->height()) / 2);

    loadingOverlay->show();
    loadingOverlay->raise();

    QApplication::processEvents();
}

void MainWindow::hideLoading()
{
    if (!loadingOverlay)
        return;

    loadingOverlay->hide();

    QApplication::processEvents();
}
