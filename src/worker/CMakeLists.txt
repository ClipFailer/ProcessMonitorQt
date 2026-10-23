# src/worker/CMakeLists.txt

add_library(pm_worker STATIC
    CollectorWorker.hpp
    CollectorWorker.cpp
)

target_include_directories(pm_worker PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(pm_worker PUBLIC
    pm_core
    pm_platform
    Qt6::Core
)

set_target_compiler_warnings(pm_worker)
