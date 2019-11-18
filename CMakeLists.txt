cmake_minimum_required (VERSION 3.6)
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

add_subdirectory(src)
