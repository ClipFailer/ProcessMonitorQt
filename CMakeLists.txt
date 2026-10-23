cmake_minimum_required(VERSION 3.20)
project(ProcessMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Конфигурация компиляторов и предупреждений
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

# Подключение необходимых компонентов Qt6
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

# Модули проекта
add_subdirectory(src/core)
add_subdirectory(src/platform)
add_subdirectory(src/worker)
add_subdirectory(src/gui)

# Исполняемый файл приложения
qt_add_executable(ProcessMonitor
    WIN32 MACOSX_BUNDLE
    main.cpp
)

target_include_directories(ProcessMonitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ProcessMonitor PRIVATE
    pm_gui
    pm_worker
    pm_platform
    pm_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

set_target_compiler_warnings(ProcessMonitor)

# Тесты
enable_testing()
add_subdirectory(tests)
