cmake_minimum_required(VERSION 3.6)

find_library( log-lib
              log )

set(DIR_Shared ${CMAKE_CURRENT_LIST_DIR}/../Shared/src/main/cpp)


include_directories(src/main/cpp)

include_directories(${DIR_Shared}/Helper)
include_directories(${DIR_Shared}/NDKHelper)

########## Experimental-add the XFEC library ##########
#######################################################
include(${CMAKE_SOURCE_DIR}/../VideoCore/src/main/cpp/XFEC/XFEC.cmake)
###############

add_library( UDPSender
        SHARED
       src/main/cpp/UDPSender.cpp
        )

target_link_libraries( UDPSender
        ${log-lib}
        log
        android
        XFEC_lib
        )