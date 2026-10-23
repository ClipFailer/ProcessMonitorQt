# src/gui/CMakeLists.txt

add_library(pm_gui STATIC
    theme/Theme.hpp
    models/ProcessTreeItem.hpp
    models/ProcessTreeItem.cpp
    models/ProcessTreeModel.hpp
    models/ProcessTreeModel.cpp
    models/ProcessSortFilterProxyModel.hpp
    models/ProcessSortFilterProxyModel.cpp
    delegates/ProcessItemDelegate.hpp
    delegates/ProcessItemDelegate.cpp
    widgets/SparklineWidget.hpp
    widgets/SparklineWidget.cpp
    widgets/SystemSummaryBar.hpp
    widgets/SystemSummaryBar.cpp
    dialogs/ProcessInspectDialog.hpp
    dialogs/ProcessInspectDialog.cpp
    MainWindow.hpp
    MainWindow.cpp
)

target_include_directories(pm_gui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(pm_gui PUBLIC
    pm_core
    pm_worker
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)

set_target_compiler_warnings(pm_gui)
