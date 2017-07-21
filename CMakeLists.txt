cmake_minimum_required(VERSION 3.0.0)
project(atlas VERSION 0.0.0)

include(CTest)
enable_testing()

add_subdirectory("deps/Anvil")

include_directories("deps/Anvil/deps"
                    "deps/Anvil/include"
                    "deps/VulkanMemoryAllocator/src"
                    "deps/glm"
                    "include")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Enable vulkan layers
  target_compile_definitions(atlas ENABLE_VALIDATION=1)
  # Fix glm::perspective to work with Vulkan's coordinate system
  target_compile_definitions(atlas GLM_FORCE_DEPTH_ZERO_TO_ONE=1)
endif()

set(ATLAS_SRC_LIST  "include/atlas.h"
                    "include/err.h"
                    "include/mesh.h"
                    "include/camera.h"
                    "include/my_math.h"
                    
                    "src/atlas.cpp"
                    "src/mesh.cpp"
                    "src/camera.cpp"
                    "src/my_math.cpp")

add_library(atlas ${ATLAS_SRC_LIST})
add_dependencies(atlas Anvil)
if (WIN32)
    target_link_libraries(atlas Anvil)
else()
    target_link_libraries(atlas Anvil dl)
endif()

if (CMAKE_GENERATOR STREQUAL "Visual Studio 6")
elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 7 .NET 2003")
elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 8 2005")
elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 9 2008")
elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 10 2010")
elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 11 2012")
elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 12 2013")
    # temporary; will add QPC implementation in the future
    message(SEND_ERROR, "Insufficient <chrono> specification. Please use VS2015 and up.")
endif()

add_executable(call_lib demos/call_lib.cpp)
target_link_libraries(call_lib atlas vulkan)

add_executable(start_vulkan demos/stolen_anvil_demo.cpp)
add_dependencies(start_vulkan atlas)
target_link_libraries(start_vulkan atlas)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
