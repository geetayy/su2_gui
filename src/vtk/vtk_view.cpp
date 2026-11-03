#include "../mainwindow.hpp"
#include <QVTKOpenGLNativeWidget.h>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QTimer>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPoints.h>
#include <vtkIntArray.h>
#include <vtkCellData.h>
#include <vtkIdTypeArray.h>
#include <vtkCellType.h>
#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkDataSetMapper.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkLookupTable.h>
#include <vtkTextProperty.h>
#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkCoordinate.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPointPicker.h>
#include <vtkCellPicker.h>
#include <vtkSphereSource.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkDataSetMapper.h>
#include <vtkCell.h>
#include <vtkProperty.h>
#include <cmath>
#include <algorithm>
#include <limits>
#include <vtkType.h>
#include <vtkUnstructuredGrid.h>


void MainWindow::setupVTK()
{
    renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    renderer = vtkSmartPointer<vtkRenderer>::New();

    renderer->SetGradientBackground(true);
    // renderer->SetBackground(74.0 / 255.0, 120.0 / 255.0, 168.0 / 255.0);
    // renderer->SetBackground2(159.0 / 255.0, 165.0 / 255.0, 171.0 / 255.0);

    renderer->SetBackground2(74.0 / 255.0, 120.0 / 255.0, 168.0 / 255.0);
    renderer->SetBackground(159.0 / 255.0, 165.0 / 255.0, 171.0 / 255.0);

    renderWindow->AddRenderer(renderer);
    vtkWidget->setRenderWindow(renderWindow);

    meshVisualization.initialize(renderer, renderWindow, vtkWidget->interactor());

    scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();
    vtuLookupTable = vtkSmartPointer<vtkLookupTable>::New();
    vtuLookupTable->SetHueRange(0.667, 0.0);
    vtuLookupTable->SetSaturationRange(1.0, 1.0);
    vtuLookupTable->SetValueRange(1.0, 1.0);
    vtuLookupTable->SetNumberOfTableValues(256);
    vtuLookupTable->Build();

    scalarBar->SetLookupTable(vtuLookupTable);
    scalarBar->SetTitle("Result");
    scalarBar->SetNumberOfLabels(7);
    // Strong, readable ParaView-style legend typography.
    auto *titleText = scalarBar->GetTitleTextProperty();
    titleText->SetColor(1.0, 1.0, 1.0);
    titleText->SetFontFamilyToArial();
    titleText->SetFontSize(18);
    titleText->SetBold(false);
    titleText->SetItalic(false);
    titleText->SetShadow(false);

    auto *labelText = scalarBar->GetLabelTextProperty();
    labelText->SetColor(1.0, 1.0, 1.0);
    labelText->SetFontFamilyToArial();
    labelText->SetFontSize(15);
    labelText->SetBold(true);
    labelText->SetItalic(false);
    labelText->SetShadow(false);


    // Reduce the actual colored strip thickness without shrinking its labels.
    // scalarBar->SetVerticalTitleSeparation(12);
    // scalarBar->SetTextPad(5);
    scalarBar->SetTextPad(10);
    scalarBar->SetBarRatio(0.12);
    scalarBar->SetUnconstrainedFontSize(true);
    scalarBar->SetNumberOfLabels(7);
    scalarBar->SetLabelFormat("%-#6.3g");
    scalarBar->SetVisibility(false);

    // Use VTK's interactive scalar-bar widget (the same interaction model
    // used by ParaView): drag the bar to move it and drag its handles to resize.
    // The widget representation, not renderer->AddActor2D(), owns the overlay.
    scalarBarWidget = vtkSmartPointer<vtkScalarBarWidget>::New();
    scalarBarWidget->SetInteractor(vtkWidget->interactor());
    scalarBarWidget->SetDefaultRenderer(renderer);
    scalarBarWidget->SetScalarBarActor(scalarBar);
    scalarBarWidget->SetRepositionable(1);
    scalarBarWidget->SetResizable(1);
    // IMPORTANT: selectable=1 consumes left-clicks inside the bar for
    // selection. Turn it off so left-dragging the legend body MOVES it.
    // (The border/corners still resize it; middle-drag also translates it.)
    scalarBarWidget->SetSelectable(0);
    scalarBarWidget->SetEnabled(0);

    if (auto *barRep = scalarBarWidget->GetScalarBarRepresentation())
    {
        // Keep the whole legend comfortably inside the viewport.
        barRep->SetPosition(0.90, 0.01);
        barRep->SetPosition2(0.1, 0.50);
        barRep->SetMoving(1);
        barRep->SetAutoOrient(true);
        barRep->SetEnforceNormalizedViewportBounds(1);
        barRep->SetProportionalResize(0);
    }

    // Clamp the legend's normalized viewport rectangle after a drag/resize,
    // so it cannot be left partially or completely outside the render area.
    auto legendBoundsCallback = vtkSmartPointer<vtkCallbackCommand>::New();
    legendBoundsCallback->SetCallback(
        [](vtkObject *caller, unsigned long, void *, void *)
        {
            auto *widget = vtkScalarBarWidget::SafeDownCast(caller);
            if (!widget)
                return;
            auto *rep = widget->GetScalarBarRepresentation();
            if (!rep || !rep->GetPositionCoordinate() || !rep->GetPosition2Coordinate())
                return;

            double *position = rep->GetPositionCoordinate()->GetValue();
            double *size = rep->GetPosition2Coordinate()->GetValue();
            if (!position || !size)
                return;

            constexpr double margin = 0.015;
            const double width = std::clamp(size[0], 0.06, 1.0 - 2.0 * margin);
            const double height = std::clamp(size[1], 0.06, 1.0 - 2.0 * margin);
            const double x = std::clamp(position[0], margin, 1.0 - margin - width);
            const double y = std::clamp(position[1], margin, 1.0 - margin - height);
            rep->SetPosition(x, y);
            rep->SetPosition2(width, height);
        });
    scalarBarWidget->AddObserver(vtkCommand::EndInteractionEvent, legendBoundsCallback);

    // Dedicated pickers for VTU point and edge selection. The SU2 mesh
    // selection path remains owned by MeshVisualization.
    vtuPointPicker = vtkSmartPointer<vtkPointPicker>::New();
    vtuPointPicker->SetTolerance(0.015);
    vtuCellPicker = vtkSmartPointer<vtkCellPicker>::New();
    vtuCellPicker->SetTolerance(0.005);

    vtuPickCallback = vtkSmartPointer<vtkCallbackCommand>::New();
    vtuPickCallback->SetClientData(this);
    vtuPickCallback->SetCallback(
        [](vtkObject *caller, unsigned long, void *clientData, void *)
        {
            auto *self = static_cast<MainWindow *>(clientData);
            auto *interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
            if (!self || !interactor || !self->postProcessingActor ||
                !self->vtuReader || !self->vtuReader->GetOutput() ||
                !self->vtuSelectionEnabled || !self->selectionModeCombo ||
                !self->renderer || !self->vtuCellPicker)
                return;

            self->vtuPickCallback->SetAbortFlag(0);
            const int *position = interactor->GetEventPosition();
            const int mode = self->selectionModeCombo->currentIndex();
            vtkUnstructuredGrid *grid = self->vtuReader->GetOutput();
            if (!self->vtuCellPicker->Pick(position[0], position[1], 0.0,
                                           self->renderer))
                return;

            const vtkIdType cellId = self->vtuCellPicker->GetCellId();
            if (cellId < 0)
                return;

            if (self->vtuSelectionActor)
            {
                self->renderer->RemoveActor(self->vtuSelectionActor);
                self->vtuSelectionActor = nullptr;
            }

            vtkSmartPointer<vtkActor> highlight = vtkSmartPointer<vtkActor>::New();
            vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
            highlight->GetProperty()->SetColor(1.0, 0.85, 0.0);
            highlight->GetProperty()->SetAmbient(0.35);
            highlight->GetProperty()->SetDiffuse(0.65);

            vtkCell *cell = grid->GetCell(cellId);
            if (!cell)
                return;
            double picked[3];
            self->vtuCellPicker->GetPickPosition(picked);

            if (mode == 0) // Node: nearest point belonging to the picked cell.
            {
                vtkIdType bestPointId = -1;
                double bestDistance = std::numeric_limits<double>::max();
                for (vtkIdType i = 0; i < cell->GetNumberOfPoints(); ++i)
                {
                    const vtkIdType pointId = cell->GetPointId(i);
                    double point[3];
                    grid->GetPoint(pointId, point);
                    const double dx = point[0] - picked[0];
                    const double dy = point[1] - picked[1];
                    const double dz = point[2] - picked[2];
                    const double d2 = dx*dx + dy*dy + dz*dz;
                    if (d2 < bestDistance) { bestDistance = d2; bestPointId = pointId; }
                }

                if (bestPointId < 0) return;
                auto points = vtkSmartPointer<vtkPoints>::New();
                auto vertices = vtkSmartPointer<vtkCellArray>::New();
                double point[3];
                grid->GetPoint(bestPointId, point);
                vtkIdType localId = points->InsertNextPoint(point);
                vertices->InsertNextCell(1, &localId);
                auto selectedNode = vtkSmartPointer<vtkPolyData>::New();
                selectedNode->SetPoints(points);
                selectedNode->SetVerts(vertices);
                mapper->SetInputData(selectedNode);
                highlight->GetProperty()->SetRepresentationToPoints();
                highlight->GetProperty()->SetPointSize(5.0);
                self->appendOutput(QString("Selected VTU node: %1\n").arg(bestPointId));
            }
            else if (mode == 1) // Edge: nearest topological edge of the picked cell.
            {
                if (cell->GetNumberOfEdges() == 0) return;
                double bestDistance = std::numeric_limits<double>::max();
                double bestA[3] = {0,0,0}, bestB[3] = {0,0,0};
                vtkIdType bestEdge = -1;
                for (int edgeIndex = 0; edgeIndex < cell->GetNumberOfEdges(); ++edgeIndex)
                {
                    vtkCell *edge = cell->GetEdge(edgeIndex);
                    if (!edge || edge->GetNumberOfPoints() < 2) continue;
                    double a[3], b[3];
                    grid->GetPoint(edge->GetPointId(0), a);
                    grid->GetPoint(edge->GetPointId(1), b);
                    const double ab[3] = {b[0]-a[0], b[1]-a[1], b[2]-a[2]};
                    const double ap[3] = {picked[0]-a[0], picked[1]-a[1], picked[2]-a[2]};
                    const double len2 = ab[0]*ab[0]+ab[1]*ab[1]+ab[2]*ab[2];
                    double t = len2 > 0.0 ? (ap[0]*ab[0]+ap[1]*ab[1]+ap[2]*ab[2])/len2 : 0.0;
                    t = std::max(0.0, std::min(1.0, t));
                    const double q[3] = {a[0]+t*ab[0],a[1]+t*ab[1],a[2]+t*ab[2]};
                    const double ex=picked[0]-q[0], ey=picked[1]-q[1], ez=picked[2]-q[2];
                    const double d2=ex*ex+ey*ey+ez*ez;
                    if (d2 < bestDistance)
                    {
                        bestDistance=d2; bestEdge=edgeIndex;
                        std::copy(a,a+3,bestA); std::copy(b,b+3,bestB);
                    }
                }
                if (bestEdge < 0) return;
                auto line = vtkSmartPointer<vtkLineSource>::New();
                line->SetPoint1(bestA); line->SetPoint2(bestB);
                mapper->SetInputConnection(line->GetOutputPort());
                highlight->GetProperty()->SetLineWidth(6.0);
                self->appendOutput(QString("Selected VTU edge %1 (cell %2)\n").arg(bestEdge).arg(cellId));
            }
            else // Face / Element: show the picked cell as a translucent overlay.
            {
                auto selectedGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
                selectedGrid->SetPoints(grid->GetPoints());
                selectedGrid->Allocate(1);
                selectedGrid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
                mapper->SetInputData(selectedGrid);
                highlight->GetProperty()->SetRepresentationToSurface();
                highlight->GetProperty()->SetEdgeVisibility(true);
                highlight->GetProperty()->SetOpacity(0.35);
                self->appendOutput(QString("Selected VTU cell: %1\n").arg(cellId));
            }

            highlight->SetMapper(mapper);
            self->vtuSelectionActor = highlight;
            self->renderer->AddActor(highlight);
            self->vtuPickCallback->SetAbortFlag(1);
            if (self->renderWindow) self->renderWindow->Render();
        });

    vtuPickObserverId = vtkWidget->interactor()->AddObserver(
        vtkCommand::LeftButtonPressEvent, vtuPickCallback, 1.0);

    vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
    axes->SetAxisLabels(true);
    axes->GetXAxisTipProperty()->SetColor(1.0, 0.0, 0.0);
    axes->GetXAxisShaftProperty()->SetColor(1.0, 0.0, 0.0);
    axes->GetYAxisTipProperty()->SetColor(0.0, 1.0, 0.0);
    axes->GetYAxisShaftProperty()->SetColor(0.0, 1.0, 0.0);
    axes->GetZAxisTipProperty()->SetColor(0.0, 0.0, 1.0);
    axes->GetZAxisShaftProperty()->SetColor(0.0, 0.0, 1.0);

    orientationWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    orientationWidget->SetOrientationMarker(axes);
    orientationWidget->SetInteractor(vtkWidget->interactor());
    orientationWidget->SetViewport(0.0, 0.0, 0.20, 0.20);
    orientationWidget->InteractiveOff();
    orientationWidget->SetEnabled(1);
    orientationWidget->SetInteractive(false);
    orientationWidget->SetPriority(0.0);
    orientationWidget->SetKeyPressActivation(false);

    renderer->GetActiveCamera()->SetPosition(0.0, 0.0, 1.0);
    renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.0, 0.0);
    renderer->GetActiveCamera()->SetViewUp(0.0, 1.0, 0.0);
    renderer->ResetCamera();
    renderWindow->Render();
}

void MainWindow::loadSU2Mesh(const QString &filename)
{
    const SU2MeshLoadResult result = loadSU2MeshFile(filename);
    if (!result.success)
    {
        appendOutput("ERROR: " + result.error + "\n");
        return;
    }

    // A SU2 mesh load replaces any dataset currently displayed for post-processing.
    if (animationTimer) animationTimer->stop();
    if (scalarBarWidget) scalarBarWidget->SetEnabled(0);
    if (scalarBar) scalarBar->SetVisibility(false);
    if (vtuSelectionActor && renderer)
        renderer->RemoveActor(vtuSelectionActor);
    vtuSelectionActor = nullptr;
    vtuSelectionEnabled = false;
    if (postProcessingActor && renderer)
        renderer->RemoveActor(postProcessingActor);
    postProcessingActor = nullptr;
    postProcessingMapper = nullptr;
    vtuReader = nullptr;

    meshData = result.mesh;

    appendOutput(QString("Loaded SU2 mesh: %1\n").arg(filename));
    appendOutput(QString("Dimension: %1D\n").arg(meshData.dimension));
    appendOutput(QString("Points: %1\n").arg(meshData.points.size()));
    appendOutput(QString("Volume elements: %1\n").arg(meshData.volumeElements.size()));
    appendOutput(QString("Boundary markers: %1\n").arg(meshData.boundaries.size()));

    meshVisualization.setMeshData(&meshData);
    updateBoundaryFamilies();

    // Keep the first milestone deliberately simple: the user can change this
    // from the View toolbar at any time.
    meshVisualization.setDisplayMode(MeshVisualization::DisplayMode::Surface);
    meshVisualization.setSelectionMode(MeshVisualization::SelectionMode::Node);
}

void MainWindow::clearViewport()
{
    if (animationTimer) animationTimer->stop();
    if (scalarBarWidget) scalarBarWidget->SetEnabled(0);
    if (scalarBar) scalarBar->SetVisibility(false);
    if (vtuSelectionActor && renderer)
        renderer->RemoveActor(vtuSelectionActor);
    vtuSelectionActor = nullptr;
    currentVtuIndex = -1;
    if (postProcessingActor && renderer)
        renderer->RemoveActor(postProcessingActor);
    postProcessingActor = nullptr;
    postProcessingMapper = nullptr;
    vtuReader = nullptr;
    meshData = SU2Mesh{};
    meshVisualization.clear();
    updateBoundaryFamilies();
    if (renderer)
        renderer->ResetCamera();
    if (renderWindow)
        renderWindow->Render();
}

void MainWindow::rotateXPlus90()
{
    if (!renderer) return;
    renderer->GetActiveCamera()->Elevation(90.0);
    renderWindow->Render();
}

void MainWindow::rotateXMinus90()
{
    if (!renderer) return;
    renderer->GetActiveCamera()->Elevation(-90.0);
    renderWindow->Render();
}

void MainWindow::rotateYPlus90()
{
    if (!renderer) return;
    renderer->GetActiveCamera()->Azimuth(90.0);
    renderWindow->Render();
}

void MainWindow::rotateYMinus90()
{
    if (!renderer) return;
    renderer->GetActiveCamera()->Azimuth(-90.0);
    renderWindow->Render();
}

void MainWindow::rotateZPlus90()
{
    if (!renderer) return;
    renderer->GetActiveCamera()->Roll(90.0);
    renderWindow->Render();
}

void MainWindow::rotateZMinus90()
{
    if (!renderer) return;
    renderer->GetActiveCamera()->Roll(-90.0);
    renderWindow->Render();
}

void MainWindow::resetView()
{
    meshVisualization.resetView();
}

void MainWindow::viewXY()
{
    meshVisualization.viewXY();
}

void MainWindow::viewYZ()
{
    meshVisualization.viewYZ();
}

void MainWindow::viewXZ()
{
    meshVisualization.viewXZ();
}

void MainWindow::fitXY()
{
    // meshVisualization.fitView();
    meshVisualization.fitXY();
}

void MainWindow::fitYZ()
{
    // meshVisualization.fitView();
    meshVisualization.fitYZ();
}

void MainWindow::fitXZ()
{
    // meshVisualization.fitView();
    meshVisualization.fitXZ();
}
