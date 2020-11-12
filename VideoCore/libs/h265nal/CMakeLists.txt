# CMakeLists files in this project can
# refer to the root source directory of the project as ${MAIN_SOURCE_DIR} and
# to the root binary directory of the project as ${MAIN_BINARY_DIR}.
cmake_minimum_required(VERSION 3.0)

# https://cmake.org/cmake/help/v3.0/command/project.html
project(h265nal VERSION 0.3)

# Abseil requires C++11
set(CMAKE_CXX_STANDARD 11)

enable_testing()

# Recurse into source code subdirectories.
add_subdirectory(src)
add_subdirectory(webrtc)
add_subdirectory(test)
