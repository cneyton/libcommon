cmake_minimum_required (VERSION 3.8)
project(libcommon)

# Set the project version number.
set(VERSION_MAJOR   0   CACHE STRING "Project major version number.")
set(VERSION_MINOR   0   CACHE STRING "Project minor version number.")
set(VERSION_PATCH   1   CACHE STRING "Project patch version number.")
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH)

# Set c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Used for autocompletion in vim
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Allow static library to be included in another lib (used for python wrapping)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(dependencies)

add_library(common INTERFACE)

target_link_libraries(common
    INTERFACE
    spdlog_header_only
    GSL
    nlohmann_json::nlohmann_json
    )

target_include_directories(common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

# ---
# Tests
# ---
include(CTest)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(COMMON_MASTER_PROJECT ON)
else()
    set(SPDLOG_MASTER_PROJECT OFF)
endif()

if (BUILD_TESTING AND COMMON_MASTER_PROJECT)
    enable_testing()
    add_subdirectory(test)
endif()

