cmake_minimum_required(VERSION 3.6)

set(SHARED_PATH ${CMAKE_CURRENT_LIST_DIR}/src/main/cpp)
#include_directories(${SHARED_PATH})
include_directories(${SHARED_PATH}/Helper)
include_directories(${SHARED_PATH}/NDKHelper)

add_library(Helper
        SHARED
        src/main/cpp/Helper/ZDummy.cpp
        )

add_library(NDKHelper
        SHARED
        src/main/cpp/NDKHelper/ZDummy.cpp
        )

target_link_libraries( Helper
        log
        android)

target_link_libraries( NDKHelper
        log
        android)