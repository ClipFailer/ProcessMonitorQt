# tests/CMakeLists.txt

add_executable(test_ring_buffer
    TestRingBuffer.cpp
)

target_include_directories(test_ring_buffer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_ring_buffer PRIVATE
    pm_core
)

set_target_compiler_warnings(test_ring_buffer)

add_test(NAME TestRingBuffer COMMAND test_ring_buffer)
