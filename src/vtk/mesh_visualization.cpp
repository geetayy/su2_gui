#include "mesh_visualization.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty2D.h>
#include <vtkCellArray.h>
#include <vtkCellPicker.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkLine.h>
#include <vtkPoints.h>
#include <vtkPointPicker.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkDataSetMapper.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCellType.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkCellData.h>
#include <vtkIntArray.h>
#include <vtkPolygon.h>
#include <vtkIdList.h>
#include <vtkCamera.h>
#include <vtkMath.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkInteractorStyleUser.h>
#include <vtkDataSetMapper.h>

namespace
{
constexpr double kMeshR = 187.0 / 255.0;
constexpr double kMeshG = 27.0 / 255.0;
constexpr double kMeshB = 61.0 / 255.0;

void addLine(vtkCellArray *lines, const QVector<int> &nodes)
{
    if (nodes.size() < 2)
        return;
    lines->InsertNextCell(nodes.size());
    for (int n : nodes)
        lines->InsertCellPoint(n);
}

void addPolygon(vtkCellArray *polys, const QVector<int> &nodes)
{
    if (nodes.size() < 3)
        return;
    polys->InsertNextCell(nodes.size());
    for (int n : nodes)
        polys->InsertCellPoint(n);
}

void setViewOrientation(
    double x, double y, double z,
    double upX, double upY, double upZ);
}

MeshVisualization::MeshVisualization()
{
    surfaceActor_ = vtkSmartPointer<vtkActor>::New();
    edgeActor_ = vtkSmartPointer<vtkActor>::New();
    wireframeActor_ = vtkSmartPointer<vtkActor>::New();
    pointActor_ = vtkSmartPointer<vtkActor>::New();
    selectionActor_ = vtkSmartPointer<vtkActor>::New();
    boundarySelectionActor_ = vtkSmartPointer<vtkActor>::New();
    edgePickActor_ = vtkSmartPointer<vtkActor>::New();
    facePickActor_ = vtkSmartPointer<vtkActor>::New();
    elementPickActor_ = vtkSmartPointer<vtkActor>::New();

    leftButtonCallback_ = vtkSmartPointer<vtkCallbackCommand>::New();
    leftButtonCallback_->SetClientData(this);
    leftButtonCallback_->SetCallback(&MeshVisualization::onLeftButtonPress);

    leftButtonReleaseCallback_ = vtkSmartPointer<vtkCallbackCommand>::New();
    leftButtonReleaseCallback_->SetClientData(this);
    leftButtonReleaseCallback_->SetCallback(&MeshVisualization::onLeftButtonRelease);

    mouseMoveCallback_ = vtkSmartPointer<vtkCallbackCommand>::New();
    mouseMoveCallback_->SetClientData(this);
    mouseMoveCallback_->SetCallback(&MeshVisualization::onMouseMove);

    selectionRectangleData_ = vtkSmartPointer<vtkPolyData>::New();
    auto rectangleMapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
    rectangleMapper->SetInputData(selectionRectangleData_);
    selectionRectangleActor_ = vtkSmartPointer<vtkActor2D>::New();
    selectionRectangleActor_->SetMapper(rectangleMapper);
    selectionRectangleActor_->GetProperty()->SetColor(1.0, 0.85, 0.0);
    selectionRectangleActor_->GetProperty()->SetLineWidth(2.5);
    selectionRectangleActor_->GetProperty()->SetOpacity(1.0);
    selectionRectangleActor_->SetVisibility(false);
}

MeshVisualization::~MeshVisualization()
{
    if (interactor_)
    {
        if (leftButtonCallback_) interactor_->RemoveObserver(leftButtonCallback_);
        if (leftButtonReleaseCallback_) interactor_->RemoveObserver(leftButtonReleaseCallback_);
        if (mouseMoveCallback_) interactor_->RemoveObserver(mouseMoveCallback_);
    }
}

void MeshVisualization::initialize(vtkRenderer *renderer,
                                    vtkGenericOpenGLRenderWindow *renderWindow,
                                    vtkRenderWindowInteractor *interactor)
{
    renderer_ = renderer;
    renderWindow_ = renderWindow;
    interactor_ = interactor;

    if (interactor_)
    {
        interactor_->AddObserver(vtkCommand::LeftButtonPressEvent,
                                 leftButtonCallback_, 1.0);
        interactor_->AddObserver(vtkCommand::LeftButtonReleaseEvent,
                                 leftButtonReleaseCallback_, 1.0);
        interactor_->AddObserver(vtkCommand::MouseMoveEvent,
                                 mouseMoveCallback_, 1.0);
        if (renderer_ && selectionRectangleActor_)
            renderer_->AddActor2D(selectionRectangleActor_);
        setInteractionMode(interactionMode_);
    }

}

void MeshVisualization::setMeshData(const SU2Mesh *mesh)
{
    mesh_ = mesh;
    selectedBoundary_.clear();
    visibleBoundaries_.clear();
    selectedNodes_.clear();
    selectedEdges_.clear();
    selectedFaces_.clear();
    selectedElements_.clear();

    rebuildTopology();
    rebuildActors();
    rebuildBoundaryActors();
    updateSelectionActor();
    updateBoundaryHighlight();
    fitView();
}

void MeshVisualization::clear()
{
    mesh_ = nullptr;
    edges_.clear();
    faces_.clear();
    elements_.clear();
    edgeLookup_.clear();
    faceLookup_.clear();
    boundaryFaceIds_.clear();
    boundaryActors_.clear();
    clearSelection();
    selectedBoundary_.clear();
    visibleBoundaries_.clear();

    if (renderer_)
    {
        renderer_->RemoveAllViewProps();
        if (selectionRectangleActor_)
            renderer_->AddActor2D(selectionRectangleActor_);
    }
    render();
}

void MeshVisualization::setDisplayMode(DisplayMode mode)
{
    displayMode_ = mode;
    updateDisplay();
}

void MeshVisualization::setSelectionMode(SelectionMode mode)
{
    selectionMode_ = mode;
    isBoxSelecting_ = false;
    if (selectionRectangleActor_)
        selectionRectangleActor_->SetVisibility(false);
    clearSelection();
    updateDisplay();
}

void MeshVisualization::setInteractionMode(InteractionMode mode)
{
    interactionMode_ = mode;

    if (!interactor_)
        return;

    if (mode == InteractionMode::View)
    {
        // Standard mouse-driven camera rotation, pan and zoom.
        auto style =
            vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();

        interactor_->SetInteractorStyle(style);
    }
    else
    {
        // No camera manipulation while selecting mesh entities.
        auto style =
            vtkSmartPointer<vtkInteractorStyleUser>::New();

        interactor_->SetInteractorStyle(style);
    }

    updateDisplay();
    render();
}


void MeshVisualization::clearSelection()
{
    // Clear all selected mesh entities.
    selectedNodes_.clear();
    selectedEdges_.clear();
    selectedFaces_.clear();
    selectedElements_.clear();

    // Stop any rectangle selection in progress.
    isBoxSelecting_ = false;

    // Hide the selection rectangle.
    if (selectionRectangleActor_)
        selectionRectangleActor_->SetVisibility(false);

    // Hide the selection highlight without rebuilding its geometry.
    if (selectionActor_)
        selectionActor_->SetVisibility(false);

    // Clear any selected boundary highlight.
    selectedBoundary_.clear();

    for (auto it = boundaryActors_.begin();
         it != boundaryActors_.end(); ++it)
    {
        if (it.value())
            it.value()->SetVisibility(false);
    }

    // Keep the mesh visible and refresh the display.
    if (renderWindow_)
        renderWindow_->Render();
}

void MeshVisualization::selectBoundary(const QString &name)
{
    if (!mesh_ || !boundaryActors_.contains(name))
        return;

    selectedBoundary_ = name;
    updateBoundaryHighlight();
}

void MeshVisualization::clearBoundarySelection()
{
    selectedBoundary_.clear();
    updateBoundaryHighlight();
}

void MeshVisualization::setBoundaryVisible(const QString &name, bool visible)
{
    if (!mesh_ || !boundaryActors_.contains(name))
        return;

    if (visible)
        visibleBoundaries_.insert(name);
    else
        visibleBoundaries_.remove(name);

    updateBoundaryHighlight();
}

void MeshVisualization::clearVisibleBoundaries()
{
    visibleBoundaries_.clear();
    updateBoundaryHighlight();
}

QStringList MeshVisualization::boundaryNames() const
{
    QStringList result;
    if (!mesh_)
        return result;

    for (const SU2Boundary &boundary : mesh_->boundaries)
        result.append(boundary.name);
    return result;
}

int MeshVisualization::boundaryElementCount(const QString &name) const
{
    if (!mesh_)
        return 0;
    for (const SU2Boundary &boundary : mesh_->boundaries)
        if (boundary.name == name)
            return boundary.elements.size();
    return 0;
}

QString MeshVisualization::selectionDescription() const
{
    switch (selectionMode_)
    {
    case SelectionMode::Node:
        if (selectedNodes_.isEmpty())
            return QStringLiteral("No node selected");

        if (selectedNodes_.size() == 1)
            return nodeDescription(*selectedNodes_.cbegin());

        return QStringLiteral("%1 nodes selected").arg(selectedNodes_.size());

    case SelectionMode::Edge:
        if (selectedEdges_.isEmpty())
            return QStringLiteral("No edge selected");

        if (selectedEdges_.size() == 1)
            return edgeDescription(*selectedEdges_.cbegin());

        return QStringLiteral("%1 edges selected").arg(selectedEdges_.size());

    case SelectionMode::Face:
        if (selectedFaces_.isEmpty())
            return QStringLiteral("No face selected");

        if (selectedFaces_.size() == 1)
            return faceDescription(*selectedFaces_.cbegin());

        return QStringLiteral("%1 faces selected").arg(selectedFaces_.size());

    case SelectionMode::Element:
        if (selectedElements_.isEmpty())
            return QStringLiteral("No element selected");

        if (selectedElements_.size() == 1)
            return elementDescription(*selectedElements_.cbegin());

        return QStringLiteral("%1 elements selected").arg(selectedElements_.size());
    }

    return QStringLiteral("No selection");
}

void MeshVisualization::setViewOrientation(double dx, double dy, double dz,
                                           double ux, double uy, double uz,
                                           bool fit)
{
    if (!renderer_)
        return;

    vtkCamera *camera = renderer_->GetActiveCamera();

    // Keep the current focal point and distance
    double fp[3];
    camera->GetFocalPoint(fp);
    double dist = camera->GetDistance();

    // Normalize direction (camera sits along this vector from the focal point)
    double len = std::sqrt(dx*dx + dy*dy + dz*dz);
    dx /= len; dy /= len; dz /= len;

    camera->SetPosition(fp[0] + dx * dist,
                        fp[1] + dy * dist,
                        fp[2] + dz * dist);
    camera->SetViewUp(ux, uy, uz);
    camera->OrthogonalizeViewUp();

    if (fit)
        renderer_->ResetCamera();   // recenter on bounds and fit

    renderer_->ResetCameraClippingRange();
    renderWindow_->Render();
}

void MeshVisualization::fitView()
{
    if (!renderer_)
        return;

    renderer_->ResetCamera();
    renderer_->ResetCameraClippingRange();
    renderWindow_->Render();
}

void MeshVisualization::viewXY() { setViewOrientation(0,0,1, 0,1,0, false); }
void MeshVisualization::viewYZ() { setViewOrientation(1,0,0, 0,0,1, false); }
void MeshVisualization::viewXZ() { setViewOrientation(0,-1,0, 0,0,1, false); }

void MeshVisualization::fitXY()  { setViewOrientation(0,0,1, 0,1,0, true); }
void MeshVisualization::fitYZ()  { setViewOrientation(1,0,0, 0,0,1, true); }
void MeshVisualization::fitXZ()  { setViewOrientation(0,-1,0, 0,0,1, true); }


void MeshVisualization::resetView()
{
    if (!renderer_) return;
    vtkCamera *camera = renderer_->GetActiveCamera();
    camera->SetPosition(0.0, 0.0, 1.0);
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    camera->SetViewUp(0.0, 1.0, 0.0);
    renderer_->ResetCamera();
    render();
}

void MeshVisualization::onLeftButtonPress(vtkObject *, unsigned long, void *clientData, void *)
{
    auto *self = static_cast<MeshVisualization *>(clientData);
    if (!self) return;
    self->handleLeftButtonPress();
    if (self->interactionMode_ == InteractionMode::Select && self->interactor_)
        self->leftButtonCallback_->SetAbortFlag(1);
}

void MeshVisualization::onLeftButtonRelease(vtkObject *, unsigned long, void *clientData, void *)
{
    auto *self = static_cast<MeshVisualization *>(clientData);
    if (!self) return;
    self->handleLeftButtonRelease();
    if (self->interactionMode_ == InteractionMode::Select && self->interactor_)
        self->leftButtonReleaseCallback_->SetAbortFlag(1);
}

void MeshVisualization::onMouseMove(vtkObject *, unsigned long, void *clientData, void *)
{
    auto *self = static_cast<MeshVisualization *>(clientData);
    if (!self) return;
    self->handleMouseMove();
    if (self->interactionMode_ == InteractionMode::Select && self->isBoxSelecting_)
        self->mouseMoveCallback_->SetAbortFlag(1);
}

void MeshVisualization::handleLeftButtonPress()
{
    if (interactionMode_ != InteractionMode::Select || !interactor_ || !mesh_ || !renderer_)
        return;

    int *pos = interactor_->GetEventPosition();
    selectionStartX_ = selectionCurrentX_ = pos[0];
    selectionStartY_ = selectionCurrentY_ = pos[1];
    isBoxSelecting_ = true;
    updateSelectionRectangle();
}

void MeshVisualization::handleMouseMove()
{
    if (!isBoxSelecting_ || !interactor_ || interactionMode_ != InteractionMode::Select)
        return;

    int *pos = interactor_->GetEventPosition();
    selectionCurrentX_ = pos[0];
    selectionCurrentY_ = pos[1];
    updateSelectionRectangle();
}

void MeshVisualization::updateSelectionRectangle()
{
    if (!selectionRectangleData_ || !selectionRectangleActor_ || !renderer_)
        return;

    auto points = vtkSmartPointer<vtkPoints>::New();
    auto lines = vtkSmartPointer<vtkCellArray>::New();
    points->InsertNextPoint(selectionStartX_, selectionStartY_, 0.0);
    points->InsertNextPoint(selectionCurrentX_, selectionStartY_, 0.0);
    points->InsertNextPoint(selectionCurrentX_, selectionCurrentY_, 0.0);
    points->InsertNextPoint(selectionStartX_, selectionCurrentY_, 0.0);
    points->InsertNextPoint(selectionStartX_, selectionStartY_, 0.0);
    vtkIdType ids[5] = {0, 1, 2, 3, 4};
    lines->InsertNextCell(5, ids);
    selectionRectangleData_->SetPoints(points);
    selectionRectangleData_->SetLines(lines);
    selectionRectangleData_->Modified();
    selectionRectangleActor_->SetVisibility(isBoxSelecting_);
    selectionRectangleActor_->Modified();
    render();
}

void MeshVisualization::handleLeftButtonRelease()
{
    if (!isBoxSelecting_ || !interactor_ || !mesh_ || !renderer_)
        return;

    int *pos = interactor_->GetEventPosition();
    selectionCurrentX_ = pos[0];
    selectionCurrentY_ = pos[1];
    const int minX = std::min(selectionStartX_, selectionCurrentX_);
    const int maxX = std::max(selectionStartX_, selectionCurrentX_);
    const int minY = std::min(selectionStartY_, selectionCurrentY_);
    const int maxY = std::max(selectionStartY_, selectionCurrentY_);
    const bool dragged = (maxX - minX) >= 5 || (maxY - minY) >= 5;

    auto insideRectangle = [&](const QVector<int> &nodeIds) -> bool {
        if (nodeIds.isEmpty()) return false;
        double wx = 0.0, wy = 0.0, wz = 0.0;
        int count = 0;
        for (int id : nodeIds)
        {
            if (id < 0 || id >= mesh_->points.size()) continue;
            const auto &p = mesh_->points[id];
            wx += p[0];
            wy += p[1];
            wz += (mesh_->dimension == 3 ? p[2] : 0.0);
            ++count;
        }
        if (!count) return false;
        renderer_->SetWorldPoint(wx / count, wy / count, wz / count, 1.0);
        renderer_->WorldToDisplay();
        const double *d = renderer_->GetDisplayPoint();
        return d[0] >= minX && d[0] <= maxX && d[1] >= minY && d[1] <= maxY;
    };

    if (dragged)
    {
        // Box selection is cumulative for every entity type. An entity is
        // selected when its center lies inside the dragged display rectangle.
        if (selectionMode_ == SelectionMode::Node)
        {
            for (int id = 0; id < mesh_->points.size(); ++id)
            {
                const auto &p = mesh_->points[id];
                renderer_->SetWorldPoint(p[0], p[1], mesh_->dimension == 3 ? p[2] : 0.0, 1.0);
                renderer_->WorldToDisplay();
                const double *d = renderer_->GetDisplayPoint();
                if (d[0] >= minX && d[0] <= maxX && d[1] >= minY && d[1] <= maxY)
                    selectedNodes_.insert(id);
            }
        }
        else if (selectionMode_ == SelectionMode::Edge)
        {
            for (int id = 0; id < edges_.size(); ++id)
                if (insideRectangle(QVector<int>{edges_[id].a, edges_[id].b}))
                    selectedEdges_.insert(id);
        }
        else if (selectionMode_ == SelectionMode::Face)
        {
            for (int id = 0; id < faces_.size(); ++id)
                if (insideRectangle(faces_[id].nodes))
                    selectedFaces_.insert(id);
        }
        else if (selectionMode_ == SelectionMode::Element)
        {
            for (int id = 0; id < elements_.size(); ++id)
                if (insideRectangle(elements_[id].nodes))
                    selectedElements_.insert(id);
        }
    }
    else
    {
        const int x = selectionCurrentX_;
        const int y = selectionCurrentY_;
        if (selectionMode_ == SelectionMode::Node)
        {
            auto picker = vtkSmartPointer<vtkPointPicker>::New();
            picker->SetTolerance(0.02);
            if (picker->Pick(x, y, 0.0, renderer_))
            {
                const int id = static_cast<int>(picker->GetPointId());
                if (id >= 0 && id < mesh_->points.size()) selectedNodes_.insert(id);
            }
        }
        else
        {
            auto picker = vtkSmartPointer<vtkCellPicker>::New();
            picker->SetTolerance(selectionMode_ == SelectionMode::Edge ? 0.04 : 0.02);
            picker->InitializePickList();
            if (selectionMode_ == SelectionMode::Edge) picker->AddPickList(edgePickActor_);
            else if (selectionMode_ == SelectionMode::Face) picker->AddPickList(facePickActor_);
            else if (selectionMode_ == SelectionMode::Element) picker->AddPickList(elementPickActor_);
            picker->PickFromListOn();
            if (picker->Pick(x, y, 0.0, renderer_))
            {
                const int id = static_cast<int>(picker->GetCellId());
                if (selectionMode_ == SelectionMode::Edge && id >= 0 && id < edges_.size())
                    selectedEdges_.insert(id);
                else if (selectionMode_ == SelectionMode::Face && id >= 0 && id < faces_.size())
                    selectedFaces_.insert(id);
                else if (selectionMode_ == SelectionMode::Element && id >= 0 && id < elements_.size())
                    selectedElements_.insert(id);
            }
        }
    }

    isBoxSelecting_ = false;
    if (selectionRectangleActor_)
        selectionRectangleActor_->SetVisibility(false);
    updateSelectionActor();
    render();
}

void MeshVisualization::rebuildTopology()
{
    edges_.clear();
    faces_.clear();
    elements_.clear();
    edgeLookup_.clear();
    faceLookup_.clear();
    boundaryFaceIds_.clear();

    if (!mesh_)
        return;

    for (int i = 0; i < mesh_->volumeElements.size(); ++i)
    {
        const SU2Element &e = mesh_->volumeElements[i];
        elements_.push_back({e.type, e.nodes});

        for (const auto &edge : localEdges(e.type, e.nodes))
        {
            const QString key = edgeKey(edge.first, edge.second);
            if (!edgeLookup_.contains(key))
            {
                int id = edges_.size();
                edgeLookup_[key] = id;
                edges_.push_back({std::min(edge.first, edge.second),
                                  std::max(edge.first, edge.second)});
            }
        }

        for (const QVector<int> &faceNodes : localFaces(e.type, e.nodes))
        {
            const QString key = faceKey(faceNodes);
            if (!faceLookup_.contains(key))
            {
                int id = faces_.size();
                faceLookup_[key] = id;
                faces_.push_back({faceNodes, i, -1, {}});
            }
        }
    }

    for (const SU2Boundary &boundary : mesh_->boundaries)
    {
        for (int bi = 0; bi < boundary.elements.size(); ++bi)
        {
            const SU2Element &be = boundary.elements[bi];
            QVector<int> nodes = be.nodes;
            const QString key = faceKey(nodes);

            if (mesh_->dimension == 2)
            {
                // In 2-D, boundary entries are edges and volume elements are faces.
                // Associate the marker with the matching edge; no synthetic face is needed.
                boundaryFaceIds_[boundary.name].append(-1);
                continue;
            }

            auto it = faceLookup_.find(key);
            if (it != faceLookup_.end())
            {
                int faceId = it.value();
                faces_[faceId].boundary = boundary.name;
                faces_[faceId].boundaryElement = bi;
                boundaryFaceIds_[boundary.name].append(faceId);
            }
        }
    }
}

void MeshVisualization::rebuildActors()
{
    if (!renderer_ || !mesh_)
        return;

    renderer_->RemoveActor(surfaceActor_);
    renderer_->RemoveActor(edgeActor_);
    renderer_->RemoveActor(wireframeActor_);
    renderer_->RemoveActor(pointActor_);
    renderer_->RemoveActor(selectionActor_);
    renderer_->RemoveActor(boundarySelectionActor_);
    renderer_->RemoveActor(edgePickActor_);
    renderer_->RemoveActor(facePickActor_);
    renderer_->RemoveActor(elementPickActor_);

    auto surface = buildSurfacePolyData();
    auto surfaceMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    surfaceMapper->SetInputData(surface);
    surfaceActor_->SetMapper(surfaceMapper);
    surfaceActor_->GetProperty()->SetColor(kMeshR, kMeshG, kMeshB);
    surfaceActor_->GetProperty()->SetOpacity(1.0);

    auto edge = buildEdgePolyData();
    auto edgeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    edgeMapper->SetInputData(edge);
    edgeActor_->SetMapper(edgeMapper);
    edgeActor_->GetProperty()->SetColor(0.12, 0.12, 0.12);
    edgeActor_->GetProperty()->SetLineWidth(1.0);

    wireframeActor_->SetMapper(surfaceMapper);
    wireframeActor_->GetProperty()->SetRepresentationToWireframe();
    wireframeActor_->GetProperty()->SetColor(kMeshR, kMeshG, kMeshB);
    wireframeActor_->GetProperty()->SetLineWidth(1.0);

    auto edgePickMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    edgePickMapper->SetInputData(edge);
    edgePickActor_->SetMapper(edgePickMapper);
    edgePickActor_->GetProperty()->SetOpacity(0.0);
    edgePickActor_->PickableOn();

    auto facePick = buildFacePolyData();
    auto facePickMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    facePickMapper->SetInputData(facePick);
    facePickActor_->SetMapper(facePickMapper);
    facePickActor_->GetProperty()->SetOpacity(0.0);
    facePickActor_->PickableOn();

    auto elementGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    auto elementPoints = vtkSmartPointer<vtkPoints>::New();
    for (const auto &p : mesh_->points)
        elementPoints->InsertNextPoint(p[0], p[1], mesh_->dimension == 3 ? p[2] : 0.0);
    elementGrid->SetPoints(elementPoints);
    for (const auto &e : elements_)
    {
        const int count = e.nodes.size();
        if (count <= 0) continue;
        int vtkType = -1;
        switch (e.type)
        {
        case 3: vtkType = VTK_LINE; break;
        case 5: vtkType = VTK_TRIANGLE; break;
        case 9: vtkType = VTK_QUAD; break;
        case 10: vtkType = VTK_TETRA; break;
        case 12: vtkType = VTK_HEXAHEDRON; break;
        case 13: vtkType = VTK_WEDGE; break;
        case 14: vtkType = VTK_PYRAMID; break;
        default: break;
        }
        if (vtkType < 0) continue;
        vtkSmartPointer<vtkIdList> ids = vtkSmartPointer<vtkIdList>::New();
        ids->SetNumberOfIds(count);
        for (int i = 0; i < count; ++i) ids->SetId(i, e.nodes[i]);
        elementGrid->InsertNextCell(vtkType, ids);
    }
    auto elementMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    elementMapper->SetInputData(elementGrid);
    elementPickActor_->SetMapper(elementMapper);
    elementPickActor_->GetProperty()->SetOpacity(0.0);
    elementPickActor_->PickableOn();

    auto points = vtkSmartPointer<vtkPolyData>::New();
    points->SetPoints(surface->GetPoints());
    auto pointMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    pointMapper->SetInputData(points);
    pointActor_->SetMapper(pointMapper);
    pointActor_->GetProperty()->SetRepresentationToPoints();
    pointActor_->GetProperty()->SetPointSize(5.0);
    pointActor_->GetProperty()->SetColor(kMeshR, kMeshG, kMeshB);

    renderer_->AddActor(surfaceActor_);
    renderer_->AddActor(edgeActor_);
    renderer_->AddActor(wireframeActor_);
    renderer_->AddActor(pointActor_);
    renderer_->AddActor(selectionActor_);
    renderer_->AddActor(boundarySelectionActor_);
    renderer_->AddActor(edgePickActor_);
    renderer_->AddActor(facePickActor_);
    renderer_->AddActor(elementPickActor_);
    if (selectionRectangleActor_)
        renderer_->AddActor2D(selectionRectangleActor_);

    updateDisplay();
}

void MeshVisualization::rebuildBoundaryActors()
{
    if (!renderer_ || !mesh_)
        return;

    for (auto it = boundaryActors_.begin(); it != boundaryActors_.end(); ++it)
        renderer_->RemoveActor(it.value());
    boundaryActors_.clear();

    for (const SU2Boundary &boundary : mesh_->boundaries)
    {
        auto poly = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();
        auto lines = vtkSmartPointer<vtkCellArray>::New();
        auto polys = vtkSmartPointer<vtkCellArray>::New();

        for (const SU2Element &e : boundary.elements)
        {
            QVector<vtkIdType> ids;
            for (int node : e.nodes)
            {
                if (node < 0 || node >= mesh_->points.size()) continue;
                const auto &p = mesh_->points[node];
                ids.append(points->InsertNextPoint(p[0], p[1], mesh_->dimension == 3 ? p[2] : 0.0));
            }
            if (ids.size() == 2)
                lines->InsertNextCell(2, ids.constData());
            else if (ids.size() >= 3)
                polys->InsertNextCell(ids.size(), ids.constData());
        }

        poly->SetPoints(points);
        if (lines->GetNumberOfCells()) poly->SetLines(lines);
        if (polys->GetNumberOfCells()) poly->SetPolys(polys);

        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(poly);
        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(0.95, 0.65, 0.10);
        actor->GetProperty()->SetLineWidth(3.0);
        actor->GetProperty()->SetOpacity(0.85);
        actor->PickableOn();

        boundaryActors_.insert(boundary.name, actor);
        renderer_->AddActor(actor);
    }
}

void MeshVisualization::updateDisplay()
{
    if (!renderer_) return;

    surfaceActor_->SetVisibility(displayMode_ == DisplayMode::Surface ||
                                 displayMode_ == DisplayMode::SurfaceWithEdges);
    edgeActor_->SetVisibility(displayMode_ == DisplayMode::SurfaceWithEdges);
    wireframeActor_->SetVisibility(displayMode_ == DisplayMode::Wireframe);
    pointActor_->SetVisibility(displayMode_ == DisplayMode::Points);

    // Checked boundary families stay visible; a selected boundary is highlighted
    // even if its checkbox is off.
    for (auto it = boundaryActors_.begin(); it != boundaryActors_.end(); ++it)
        it.value()->SetVisibility(visibleBoundaries_.contains(it.key()) ||
                                  it.key() == selectedBoundary_);

    edgePickActor_->SetVisibility(selectionMode_ == SelectionMode::Edge);
    facePickActor_->SetVisibility(selectionMode_ == SelectionMode::Face);
    elementPickActor_->SetVisibility(selectionMode_ == SelectionMode::Element);

    // Keep pick geometry effectively invisible while ensuring it remains in the
    // renderer's visible prop list for vtkCellPicker.
    edgePickActor_->GetProperty()->SetOpacity(0.001);
    facePickActor_->GetProperty()->SetOpacity(0.001);
    elementPickActor_->GetProperty()->SetOpacity(0.001);

    render();
}

void MeshVisualization::updateSelectionActor()
{
    if (!renderer_ || !mesh_)
        return;

    auto poly = buildSelectionPolyData();
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(poly);
    selectionActor_->SetMapper(mapper);

    selectionActor_->GetProperty()->SetColor(1.0, 0.90, 0.05);
    selectionActor_->GetProperty()->SetOpacity(1.0);
    selectionActor_->GetProperty()->SetLineWidth(3.0);
    selectionActor_->GetProperty()->SetPointSize(5.0);

    if (selectionMode_ == SelectionMode::Node)
    {
        // Render selected nodes as small point dots, not large ring glyphs.
        selectionActor_->GetProperty()->SetRepresentationToPoints();
        selectionActor_->GetProperty()->SetPointSize(5.0);
    }
    else if (selectionMode_ == SelectionMode::Edge)
    {
        selectionActor_->GetProperty()->SetRepresentationToWireframe();
    }
    else
    {
        selectionActor_->GetProperty()->SetRepresentationToSurface();
        selectionActor_->GetProperty()->SetOpacity(0.45);
        selectionActor_->GetProperty()->SetEdgeVisibility(true);
        selectionActor_->GetProperty()->SetEdgeColor(1.0, 0.85, 0.0);
    }

    selectionActor_->SetVisibility(poly->GetNumberOfPoints() > 0);
    render();
}

void MeshVisualization::updateBoundaryHighlight()
{
    if (!renderer_) return;

    for (auto it = boundaryActors_.begin(); it != boundaryActors_.end(); ++it)
    {
        const bool selected = (it.key() == selectedBoundary_);
        const bool visible = visibleBoundaries_.contains(it.key()) || selected;
        it.value()->SetVisibility(visible);
        it.value()->GetProperty()->SetColor(selected ? 1.0 : 0.95,
                                             selected ? 0.85 : 0.65,
                                             selected ? 0.05 : 0.10);
        it.value()->GetProperty()->SetLineWidth(selected ? 5.0 : 3.0);
    }
    render();
}

void MeshVisualization::render()
{
    if (renderWindow_)
        renderWindow_->Render();
}

int MeshVisualization::findEdgeNearPick(double x, double y) const
{
    if (!mesh_ || edges_.isEmpty()) return -1;

    double best = std::numeric_limits<double>::max();
    int bestId = -1;
    for (int i = 0; i < edges_.size(); ++i)
    {
        const auto &e = edges_[i];
        const auto &a = mesh_->points[e.a];
        const auto &b = mesh_->points[e.b];
        const double ax = a[0], ay = a[1];
        const double bx = b[0], by = b[1];
        const double dx = bx - ax, dy = by - ay;
        const double len2 = dx*dx + dy*dy;
        double t = len2 > 0 ? ((x-ax)*dx + (y-ay)*dy)/len2 : 0.0;
        t = std::max(0.0, std::min(1.0, t));
        const double px = ax + t*dx, py = ay + t*dy;
        const double d2 = (x-px)*(x-px) + (y-py)*(y-py);
        if (d2 < best) { best = d2; bestId = i; }
    }
    return bestId;
}

int MeshVisualization::findFaceNearPick(double x, double y) const
{
    if (!mesh_ || faces_.isEmpty()) return -1;

    // For a 2-D mesh use the containing/nearest element centroid. For 3-D,
    // the visible surface face is selected by the closest face centroid in
    // the current projected XY coordinates. The visual overlay then makes the
    // result explicit.
    double best = std::numeric_limits<double>::max();
    int bestId = -1;
    for (int i = 0; i < faces_.size(); ++i)
    {
        const auto &nodes = faces_[i].nodes;
        double cx = 0.0, cy = 0.0;
        int count = 0;
        for (int n : nodes)
        {
            if (n < 0 || n >= mesh_->points.size()) continue;
            cx += mesh_->points[n][0];
            cy += mesh_->points[n][1];
            ++count;
        }
        if (!count) continue;
        cx /= count; cy /= count;
        const double d2 = (x-cx)*(x-cx) + (y-cy)*(y-cy);
        if (d2 < best) { best = d2; bestId = i; }
    }
    return bestId;
}

vtkSmartPointer<vtkPolyData> MeshVisualization::buildSurfacePolyData() const
{
    auto poly = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto polys = vtkSmartPointer<vtkCellArray>::New();
    auto lines = vtkSmartPointer<vtkCellArray>::New();

    if (!mesh_) return poly;

    for (const auto &p : mesh_->points)
        points->InsertNextPoint(p[0], p[1], mesh_->dimension == 3 ? p[2] : 0.0);

    if (mesh_->dimension == 2)
    {
        for (const auto &e : mesh_->volumeElements)
        {
            if (e.type == 5 || e.type == 9)
                addPolygon(polys, e.nodes);
            else if (e.type == 3)
                addLine(lines, e.nodes);
        }
    }
    else
    {
        for (const auto &face : faces_)
        {
            if (face.boundary.isEmpty())
                continue;
            addPolygon(polys, face.nodes);
        }
    }

    poly->SetPoints(points);
    if (polys->GetNumberOfCells()) poly->SetPolys(polys);
    if (lines->GetNumberOfCells()) poly->SetLines(lines);
    return poly;
}

vtkSmartPointer<vtkPolyData> MeshVisualization::buildEdgePolyData() const
{
    auto poly = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto lines = vtkSmartPointer<vtkCellArray>::New();
    if (!mesh_) return poly;

    for (const auto &p : mesh_->points)
        points->InsertNextPoint(p[0], p[1], mesh_->dimension == 3 ? p[2] : 0.0);
    for (const auto &e : edges_)
    {
        vtkIdType ids[2] = {e.a, e.b};
        lines->InsertNextCell(2, ids);
    }
    poly->SetPoints(points);
    poly->SetLines(lines);
    return poly;
}

vtkSmartPointer<vtkPolyData> MeshVisualization::buildFacePolyData() const
{
    auto poly = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto polys = vtkSmartPointer<vtkCellArray>::New();
    if (!mesh_) return poly;
    for (const auto &p : mesh_->points)
        points->InsertNextPoint(p[0], p[1], mesh_->dimension == 3 ? p[2] : 0.0);
    for (const auto &f : faces_)
        addPolygon(polys, f.nodes);
    poly->SetPoints(points);
    poly->SetPolys(polys);
    return poly;
}

/*vtkSmartPointer<vtkPolyData> MeshVisualization::buildSelectionPolyData() const
{
    auto poly = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto lines = vtkSmartPointer<vtkCellArray>::New();
    auto polys = vtkSmartPointer<vtkCellArray>::New();

    if (!mesh_) return poly;

    for (const auto &p : mesh_->points)
        points->InsertNextPoint(p[0], p[1], mesh_->dimension == 3 ? p[2] : 0.0);

    if (selectionMode_ == SelectionMode::Node && selectedNode_ >= 0)
    {
        vtkIdType id = selectedNode_;
        auto verts = vtkSmartPointer<vtkCellArray>::New();
        verts->InsertNextCell(1, &id);
        poly->SetPoints(points);
        poly->SetVerts(verts);
        return poly;
    }

    if (selectionMode_ == SelectionMode::Edge && selectedEdge_ >= 0 && selectedEdge_ < edges_.size())
    {
        const auto &e = edges_[selectedEdge_];
        vtkIdType ids[2] = {e.a, e.b};
        lines->InsertNextCell(2, ids);
        poly->SetPoints(points);
        poly->SetLines(lines);
        return poly;
    }

    if (selectionMode_ == SelectionMode::Face && selectedFace_ >= 0 && selectedFace_ < faces_.size())
    {
        addPolygon(polys, faces_[selectedFace_].nodes);
        poly->SetPoints(points);
        poly->SetPolys(polys);
        return poly;
    }

    if (selectionMode_ == SelectionMode::Element && selectedElement_ >= 0 && selectedElement_ < elements_.size())
    {
        for (const QVector<int> &face : localFaces(elements_[selectedElement_].type,
                                                    elements_[selectedElement_].nodes))
            addPolygon(polys, face);
        if (polys->GetNumberOfCells() == 0)
            addLine(lines, elements_[selectedElement_].nodes);
        poly->SetPoints(points);
        if (polys->GetNumberOfCells()) poly->SetPolys(polys);
        if (lines->GetNumberOfCells()) poly->SetLines(lines);
    }
    return poly;
}*/

vtkSmartPointer<vtkPolyData>
MeshVisualization::buildSelectionPolyData() const
{
    auto poly = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto lines = vtkSmartPointer<vtkCellArray>::New();
    auto polys = vtkSmartPointer<vtkCellArray>::New();
    auto verts = vtkSmartPointer<vtkCellArray>::New();

    if (!mesh_)
        return poly;

    // Nodes: only insert the selected points.
    if (selectionMode_ == SelectionMode::Node)
    {
        for (int id : selectedNodes_)
        {
            if (id < 0 || id >= mesh_->points.size())
                continue;

            const auto &p = mesh_->points[id];

            vtkIdType pointId = points->InsertNextPoint(
                p[0], p[1],
                mesh_->dimension == 3 ? p[2] : 0.0);

            verts->InsertNextCell(1, &pointId);
        }

        poly->SetPoints(points);
        poly->SetVerts(verts);
        return poly;
    }

    // Other entity types use the original mesh point indexing.
    for (const auto &p : mesh_->points)
    {
        points->InsertNextPoint(
            p[0], p[1],
            mesh_->dimension == 3 ? p[2] : 0.0);
    }

    if (selectionMode_ == SelectionMode::Edge)
    {
        for (int id : selectedEdges_)
        {
            if (id < 0 || id >= edges_.size())
                continue;

            const auto &e = edges_[id];
            vtkIdType ids[2] = {e.a, e.b};
            lines->InsertNextCell(2, ids);
        }

        poly->SetPoints(points);
        poly->SetLines(lines);
    }
    else if (selectionMode_ == SelectionMode::Face)
    {
        for (int id : selectedFaces_)
        {
            if (id < 0 || id >= faces_.size())
                continue;

            addPolygon(polys, faces_[id].nodes);
        }

        poly->SetPoints(points);
        poly->SetPolys(polys);
    }
    else if (selectionMode_ == SelectionMode::Element)
    {
        for (int id : selectedElements_)
        {
            if (id < 0 || id >= elements_.size())
                continue;

            const auto &element = elements_[id];

            const auto elementFaces = localFaces(
                element.type, element.nodes);

            for (const QVector<int> &face : elementFaces)
                addPolygon(polys, face);

            // For elements without polygonal faces, show their edges.
            if (elementFaces.isEmpty())
                addLine(lines, element.nodes);
        }

        poly->SetPoints(points);

        if (polys->GetNumberOfCells() > 0)
            poly->SetPolys(polys);

        if (lines->GetNumberOfCells() > 0)
            poly->SetLines(lines);
    }

    return poly;
}

const QVector<int> *MeshVisualization::elementNodes(int id) const
{
    if (id < 0 || id >= elements_.size()) return nullptr;
    return &elements_[id].nodes;
}

QString MeshVisualization::nodeDescription(int id) const
{
    if (!mesh_ || id < 0 || id >= mesh_->points.size()) return {};
    const auto &p = mesh_->points[id];
    return QString("NODE\nID %1\nX %2\nY %3\nZ %4")
        .arg(id).arg(p[0], 0, 'f', 8).arg(p[1], 0, 'f', 8)
        .arg(mesh_->dimension == 3 ? p[2] : 0.0, 0, 'f', 8);
}

QString MeshVisualization::edgeDescription(int id) const
{
    if (!mesh_ || id < 0 || id >= edges_.size()) return {};
    const auto &e = edges_[id];
    const auto &a = mesh_->points[e.a];
    const auto &b = mesh_->points[e.b];
    const double dx=b[0]-a[0], dy=b[1]-a[1], dz=(mesh_->dimension==3 ? b[2]-a[2] : 0.0);
    const double length=std::sqrt(dx*dx+dy*dy+dz*dz);
    return QString("EDGE\nID %1\nNode 1 %2\nNode 2 %3\nLength %4")
        .arg(id).arg(e.a).arg(e.b).arg(length, 0, 'f', 8);
}

QString MeshVisualization::faceDescription(int id) const
{
    if (!mesh_ || id < 0 || id >= faces_.size()) return {};
    const auto &f = faces_[id];
    QStringList nodeList;
    for (int n : f.nodes) nodeList << QString::number(n);
    QString text = QString("FACE\nID %1\nNodes %2\nParent Element %3")
        .arg(id).arg(nodeList.join(", ")).arg(f.parentElement);
    text += QString("\nBoundary %1").arg(f.boundary.isEmpty() ? "Interior" : f.boundary);
    return text;
}

QString MeshVisualization::elementDescription(int id) const
{
    if (!mesh_ || id < 0 || id >= elements_.size()) return {};
    const auto &e = elements_[id];
    QStringList nodeList;
    for (int n : e.nodes) nodeList << QString::number(n);
    return QString("ELEMENT\nID %1\nType %2\nNodes %3")
        .arg(id).arg(SU2Mesh::elementTypeName(e.type)).arg(nodeList.join(", "));
}

QString MeshVisualization::edgeKey(int a, int b)
{
    if (a > b) std::swap(a, b);
    return QString::number(a) + ":" + QString::number(b);
}

QString MeshVisualization::faceKey(QVector<int> nodes)
{
    std::sort(nodes.begin(), nodes.end());
    QStringList values;
    for (int n : nodes) values << QString::number(n);
    return values.join(":");
}

QVector<QVector<int>> MeshVisualization::localFaces(int type, const QVector<int> &n)
{
    QVector<QVector<int>> f;
    switch (type)
    {
    case 5: if (n.size() >= 3) f << QVector<int>{n[0],n[1],n[2]}; break;
    case 9: if (n.size() >= 4) f << QVector<int>{n[0],n[1],n[2],n[3]}; break;
    case 10:
        if (n.size() >= 4) f << QVector<int>{n[0],n[1],n[2]}
                              << QVector<int>{n[0],n[1],n[3]}
                              << QVector<int>{n[1],n[2],n[3]}
                              << QVector<int>{n[2],n[0],n[3]};
        break;
    case 12:
        if (n.size() >= 8) f << QVector<int>{n[0],n[1],n[2],n[3]}
                              << QVector<int>{n[4],n[5],n[6],n[7]}
                              << QVector<int>{n[0],n[1],n[5],n[4]}
                              << QVector<int>{n[1],n[2],n[6],n[5]}
                              << QVector<int>{n[2],n[3],n[7],n[6]}
                              << QVector<int>{n[3],n[0],n[4],n[7]};
        break;
    case 13:
        if (n.size() >= 6) f << QVector<int>{n[0],n[1],n[2]}
                              << QVector<int>{n[3],n[5],n[4]}
                              << QVector<int>{n[0],n[1],n[4],n[3]}
                              << QVector<int>{n[1],n[2],n[5],n[4]}
                              << QVector<int>{n[2],n[0],n[3],n[5]};
        break;
    case 14:
        if (n.size() >= 5) f << QVector<int>{n[0],n[1],n[2],n[3]}
                              << QVector<int>{n[0],n[1],n[4]}
                              << QVector<int>{n[1],n[2],n[4]}
                              << QVector<int>{n[2],n[3],n[4]}
                              << QVector<int>{n[3],n[0],n[4]};
        break;
    }
    return f;
}

QVector<QPair<int,int>> MeshVisualization::localEdges(int type, const QVector<int> &n)
{
    QVector<QPair<int,int>> e;
    auto add = [&](int a, int b){ if (a < n.size() && b < n.size()) e.append({n[a],n[b]}); };
    switch (type)
    {
    case 3: add(0,1); break;
    case 5: add(0,1); add(1,2); add(2,0); break;
    case 9: add(0,1); add(1,2); add(2,3); add(3,0); break;
    case 10: add(0,1); add(1,2); add(2,0); add(0,3); add(1,3); add(2,3); break;
    case 12:
        add(0,1); add(1,2); add(2,3); add(3,0); add(4,5); add(5,6); add(6,7); add(7,4);
        add(0,4); add(1,5); add(2,6); add(3,7); break;
    case 13:
        add(0,1); add(1,2); add(2,0); add(3,4); add(4,5); add(5,3); add(0,3); add(1,4); add(2,5); break;
    case 14:
        add(0,1); add(1,2); add(2,3); add(3,0); add(0,4); add(1,4); add(2,4); add(3,4); break;
    }
    return e;
}
