cmake_minimum_required(VERSION 3.6)

find_library( log-lib
              log )

set(DIR_Shared ${CMAKE_CURRENT_LIST_DIR}/../Shared/src/main/cpp)

include_directories(src/main/cpp)

include_directories(${DIR_Shared}/Helper)
include_directories(${DIR_Shared}/NDKHelper)
include_directories(${DIR_Shared}/InputOutput)

set(DIR_VideoCore ${CMAKE_CURRENT_LIST_DIR}/../VideoCore/src/main/cpp)

########## Experimental-add the XFEC library ##########
#######################################################
include(${DIR_VideoCore}/XFEC/XFEC.cmake)
###############

##
set(H264_BITSTREAM_DIR ${DIR_VideoCore}/../../../libs/h264bitstream)
# use '2' to avoid duplicate error
add_library(h264bitstream2
        SHARED
        ${H264_BITSTREAM_DIR}/h264_stream.c
        ${H264_BITSTREAM_DIR}/h264_sei.c
        ${H264_BITSTREAM_DIR}/h264_nal.c
        )
target_link_libraries(h264bitstream2
        ${log-lib}
        log
        android)
include_directories(${H264_BITSTREAM_DIR})
#############

add_library( VideoTransmitter
        SHARED
        ${DIR_Shared}/InputOutput/UDPSender.cpp
        ${DIR_VideoCore}/Parser/ParseRTP.cpp
        src/main/cpp/VideoTransmitter.cpp
        )

target_link_libraries( VideoTransmitter
        ${log-lib}
        log
        android
        XFEC_lib
        h264bitstream2
        )