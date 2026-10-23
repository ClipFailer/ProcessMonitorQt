# src/core/CMakeLists.txt

add_library(pm_core STATIC
    Types.hpp
    RingBuffer.hpp
    ISystemCollector.hpp
    MetaTypes.hpp
    MetaTypes.cpp
)

target_include_directories(pm_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pm_core PUBLIC
    Qt6::Core
)

set_target_compiler_warnings(pm_core)
