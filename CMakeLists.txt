cmake_minimum_required (VERSION 3.15)
project(libcommon VERSION 0.3.0 LANGUAGES CXX)

################################################################################
# options
################################################################################
# Set c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Used for autocompletion in vim
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

################################################################################
# dependencies
################################################################################
# log
add_subdirectory(dependencies/spdlog)
# json
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install    OFF CACHE INTERNAL "")
add_subdirectory(dependencies/json)

################################################################################
# build
################################################################################
add_library(common INTERFACE)

target_link_libraries(common INTERFACE
    spdlog_header_only
    nlohmann_json::nlohmann_json
    )

target_include_directories(common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

################################################################################
# Tests
################################################################################
include(CTest)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(COMMON_MASTER_PROJECT ON)
else()
    set(COMMON_MASTER_PROJECT OFF)
endif()

if (BUILD_TESTING AND COMMON_MASTER_PROJECT)
    enable_testing()
    add_subdirectory(test)
endif()

