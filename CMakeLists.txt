cmake_minimum_required(VERSION 3.0.0)
project(atlas VERSION 0.0.0)

include(CTest)
enable_testing()

add_subdirectory("Anvil")

include_directories("deps/Anvil/deps"
                    "deps/Anvil/include")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

add_library(atlas src/atlas.cpp)

add_executable(call_lib demos/call_lib.cpp)
target_link_libraries(call_lib atlas vulkan)

add_executable(start_vulkan demos/stolen_anvil_demo.cpp)
add_dependencies(start_vulkan Anvil)
if (WIN32)
    target_link_libraries(start_vulkan Anvil)
else()
    target_link_libraries(start_vulkan Anvil dl)
endif()


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
