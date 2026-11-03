#pragma once

#include <QString>
#include <QVector>
#include <QHash>
#include <QPair>
#include <QSet>

#include <vtkSmartPointer.h>
#include <vtkType.h>

class vtkActor;
class vtkActor2D;
class vtkGenericOpenGLRenderWindow;
class vtkPolyData;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkCallbackCommand;
class vtkUnstructuredGrid;
class vtkObject;

#include "../mesh/su2_mesh.hpp"

class MeshVisualization
{
public:
    enum class DisplayMode { Surface, SurfaceWithEdges, Wireframe, Points };
    enum class SelectionMode { Node, Edge, Face, Element };
    enum class InteractionMode { View, Select };

    MeshVisualization();
    ~MeshVisualization();

    void initialize(vtkRenderer *renderer,
                    vtkGenericOpenGLRenderWindow *renderWindow,
                    vtkRenderWindowInteractor *interactor);

    void setMeshData(const SU2Mesh *mesh);
    void clear();

    void setDisplayMode(DisplayMode mode);
    DisplayMode displayMode() const { return displayMode_; }

    void setSelectionMode(SelectionMode mode);
    SelectionMode selectionMode() const { return selectionMode_; }
    void setInteractionMode(InteractionMode mode);
    InteractionMode interactionMode() const {
        return interactionMode_;
    }

    void clearSelection();

    void selectBoundary(const QString &name);
    void clearBoundarySelection();
    void setBoundaryVisible(const QString &name, bool visible);
    void clearVisibleBoundaries();
    QStringList boundaryNames() const;
    int boundaryElementCount(const QString &name) const;
    QString selectedBoundary() const { return selectedBoundary_; }

    QString selectionDescription() const;

    // void setViewOrientation();
    void fitView();
    void viewXY();
    void viewYZ();
    void viewXZ();
    void fitXY();
    void fitYZ();
    void fitXZ();
    void resetView();
    void setViewOrientation(
        double dx, double dy, double dz,
        double upX, double upY, double upZ, bool fit);

    // Drag-selection state
    bool isBoxSelecting_ = false;

    int selectionStartX_ = 0;
    int selectionStartY_ = 0;
    int selectionCurrentX_ = 0;
    int selectionCurrentY_ = 0;

private:
    struct EdgeRecord
    {
        int a = -1;
        int b = -1;
    };

    struct FaceRecord
    {
        QVector<int> nodes;
        int parentElement = -1;
        int boundaryElement = -1;
        QString boundary;
    };

    struct ElementRecord
    {
        int type = 0;
        QVector<int> nodes;
    };

    static void onLeftButtonPress(vtkObject *caller,
                                  unsigned long eventId,
                                  void *clientData,
                                  void *callData);
    static void onLeftButtonRelease(vtkObject *caller,
                                    unsigned long eventId,
                                    void *clientData,
                                    void *callData);
    static void onMouseMove(vtkObject *caller,
                            unsigned long eventId,
                            void *clientData,
                            void *callData);

    void handleLeftButtonPress();
    void handleLeftButtonRelease();
    void handleMouseMove();
    void updateSelectionRectangle();
    void rebuildTopology();
    void rebuildActors();
    void rebuildBoundaryActors();
    void updateDisplay();
    void updateSelectionActor();
    void updateBoundaryHighlight();
    void render();

    int findEdgeNearPick(double x, double y) const;
    int findFaceNearPick(double x, double y) const;

    vtkSmartPointer<vtkPolyData> buildSurfacePolyData() const;
    vtkSmartPointer<vtkPolyData> buildEdgePolyData() const;
    vtkSmartPointer<vtkPolyData> buildFacePolyData() const;
    vtkSmartPointer<vtkPolyData> buildSelectionPolyData() const;

    const QVector<int> *elementNodes(int id) const;
    QString elementDescription(int id) const;
    QString nodeDescription(int id) const;
    QString edgeDescription(int id) const;
    QString faceDescription(int id) const;

    static QString edgeKey(int a, int b);
    static QString faceKey(QVector<int> nodes);
    static QVector<QVector<int>> localFaces(int type,
                                             const QVector<int> &nodes);
    static QVector<QPair<int,int>> localEdges(int type,
                                              const QVector<int> &nodes);

    vtkRenderer *renderer_ = nullptr;
    vtkGenericOpenGLRenderWindow *renderWindow_ = nullptr;
    vtkRenderWindowInteractor *interactor_ = nullptr;

    const SU2Mesh *mesh_ = nullptr;

    // DisplayMode displayMode_ = DisplayMode::SurfaceWithEdges;
    DisplayMode displayMode_ = DisplayMode::Surface;
    SelectionMode selectionMode_ = SelectionMode::Node;

/*    int selectedNode_ = -1;
    int selectedEdge_ = -1;
    int selectedFace_ = -1;
    int selectedElement_ = -1;*/

    QSet<int> selectedNodes_;
    QSet<int> selectedEdges_;
    QSet<int> selectedFaces_;
    QSet<int> selectedElements_;
    InteractionMode interactionMode_ = InteractionMode::View;
    QString selectedBoundary_;
    QSet<QString> visibleBoundaries_;

    QVector<EdgeRecord> edges_;
    QVector<FaceRecord> faces_;
    QVector<ElementRecord> elements_;
    QHash<QString, int> edgeLookup_;
    QHash<QString, int> faceLookup_;
    QHash<QString, QVector<int>> boundaryFaceIds_;

    QHash<QString, vtkSmartPointer<vtkActor>> boundaryActors_;

    vtkSmartPointer<vtkActor> surfaceActor_;
    vtkSmartPointer<vtkActor> edgeActor_;
    vtkSmartPointer<vtkActor> wireframeActor_;
    vtkSmartPointer<vtkActor> pointActor_;
    vtkSmartPointer<vtkActor> selectionActor_;
    vtkSmartPointer<vtkActor> boundarySelectionActor_;
    vtkSmartPointer<vtkCallbackCommand> leftButtonCallback_;
    vtkSmartPointer<vtkCallbackCommand> leftButtonReleaseCallback_;
    vtkSmartPointer<vtkCallbackCommand> mouseMoveCallback_;
    vtkSmartPointer<vtkActor2D> selectionRectangleActor_;
    vtkSmartPointer<vtkPolyData> selectionRectangleData_;
    vtkSmartPointer<vtkActor> edgePickActor_;
    vtkSmartPointer<vtkActor> facePickActor_;
    vtkSmartPointer<vtkActor> elementPickActor_;
};
