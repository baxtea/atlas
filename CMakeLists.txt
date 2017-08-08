cmake_minimum_required(VERSION 3.7.0)
project(atlas)

include(CTest)
enable_testing()

find_package(Vulkan)
if(Vulkan_FOUND)
    include_directories(${Vulkan_INCLUDE_DIR})
    message("Using Vulkan at " ${Vulkan_INCLUDE_DIR} ", " ${Vulkan_LIBRARY})
else()
    message(FATAL_ERROR "Couldn't find Vulkan")
endif()

include_directories(${Vulkan_INCLUDE_DIR}
                    "deps/glm"
                    "include"
                    "deps/VulkanMemoryAllocator/src")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

set(ATLAS_SRC_LIST  "include/mesh.h"
                    "include/camera.h"
                    "include/my_math.h"
                    "include/window.h"
                    "include/backend.h"
                    "include/shader.h"
                    
                    "src/mesh.cpp"
                    "src/camera.cpp"
                    "src/my_math.cpp"
                    "src/window.cpp"
                    "src/backend.cpp"
                    "src/shader.cpp")

add_library(atlas ${ATLAS_SRC_LIST})

target_compile_definitions(atlas PUBLIC
    ATLAS_VERSION_MAJOR=0
    ATLAS_VERSION_MINOR=0
    ATLAS_VERSION_PATCH=1
    # Fix glm::perspective to work with Vulkan's coordinate system
    GLM_FORCE_DEPTH_ZERO_TO_ONE=1)

if (WIN32)
    target_link_libraries(atlas ${Vulkan_LIBRARY})
else()
    target_link_libraries(atlas ${Vulkan_LIBRARY} dl xcb)
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

#add_executable(call_lib demos/call_lib.cpp)
#target_link_libraries(call_lib atlas)

add_executable(breakout demos/breakout.cpp)
target_link_libraries(breakout atlas)

#add_executable(start_vulkan demos/stolen_anvil_demo.cpp)
#add_dependencies(start_vulkan atlas)
#target_link_libraries(start_vulkan atlas)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
