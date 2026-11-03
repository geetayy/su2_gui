cmake_minimum_required(VERSION 3.16)

project(SU2GUI
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    OpenGLWidgets
)

find_package(VTK 9.1 REQUIRED COMPONENTS
    CommonCore
    CommonDataModel
    FiltersCore
    FiltersGeneral
    RenderingCore
    RenderingOpenGL2
    RenderingUI
    RenderingAnnotation
    InteractionStyle
    InteractionWidgets
    GUISupportQt
    IOXML
)

add_executable(su2_gui
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.hpp
    src/mesh/su2_mesh.cpp
    src/mesh/su2_mesh.hpp
    src/vtk/mesh_visualization.cpp
    src/vtk/mesh_visualization.hpp
    src/ui/mainwindow_layout.cpp
    src/case/case_manager.cpp
    src/solver/solver_runner.cpp
    src/vtk/vtk_view.cpp
    src/widgets/convergence_widget.cpp
    src/widgets/convergence_widget.hpp
)

target_link_libraries(su2_gui
    PRIVATE
        Qt6::Widgets
        Qt6::OpenGLWidgets
        ${VTK_LIBRARIES}
        VTK::IOXML
        VTK::InteractionWidgets
)

target_include_directories(su2_gui
    PRIVATE
        ${VTK_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

vtk_module_autoinit(
    TARGETS su2_gui
    MODULES ${VTK_LIBRARIES}
)
