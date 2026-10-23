# src/platform/CMakeLists.txt

set(PLATFORM_SOURCES
    SystemCollectorFactory.cpp
)

if(WIN32)
    list(APPEND PLATFORM_SOURCES
        windows/WinUtils.hpp
        windows/WinSystemCollector.hpp
        windows/WinSystemCollector.cpp
    )
elseif(UNIX AND NOT APPLE)
    list(APPEND PLATFORM_SOURCES
        linux/LinuxSystemCollector.hpp
        linux/LinuxSystemCollector.cpp
    )
endif()

add_library(pm_platform STATIC
    ${PLATFORM_SOURCES}
)

target_include_directories(pm_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(pm_platform PUBLIC
    pm_core
)

if(WIN32)
    target_link_libraries(pm_platform PRIVATE
        psapi
        iphlpapi
    )
endif()

set_target_compiler_warnings(pm_platform)
