cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/version.cmake)

set(COMPONENTS main)

set(EXTRA_COMPONENT_DIRS "components")

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(evospark-v2)
