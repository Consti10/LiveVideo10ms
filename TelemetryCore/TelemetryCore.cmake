##########################################################################################################
# include this to build the native part
##########################################################################################################

#cmake_minimum_required(VERSION 3.6)

project(TelemetryCoreProject VERSION 1.0.0 LANGUAGES CXX)

#find_library( log-lib
#              log )

set(DIR_VideoTelemetryShared ${CMAKE_CURRENT_LIST_DIR}/../Shared/src/main/cpp)

# Add IO and Helper from the video/telemetry shared folder
set(IO_PATH ${DIR_VideoTelemetryShared}/InputOutput)
set(HELPER_PATH ${DIR_VideoTelemetryShared}/Helper)
include_directories(${HELPER_PATH})
include_directories(${IO_PATH})

set(T_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/main/cpp/)
include_directories( ${T_SOURCE_DIR}/SharedCppC)
####################
#C-Files
####################
add_library( parser
        SHARED
        ${T_SOURCE_DIR}/parser_c/ltm.c
        ${T_SOURCE_DIR}/parser_c/frsky.c
        ${T_SOURCE_DIR}/parser_c/mavlink2.c
        ${T_SOURCE_DIR}/parser_c/smartport.c
        )
target_link_libraries(parser
        android
        log)
#######################################################
include_directories( ${T_SOURCE_DIR}/TelemetryReceiver)
include_directories( ${T_SOURCE_DIR}/WFBTelemetryData)
add_library( TelemetryReceiver
        SHARED
        ${T_SOURCE_DIR}/TelemetryReceiver/TelemetryReceiver.cpp
        # these 2 files are included in VideoCore
        ${IO_PATH}/FileReader.cpp
        ${IO_PATH}/UDPReceiver.cpp
        )
target_link_libraries(TelemetryReceiver
        android
        log
        mediandk
        parser)


