cmake_minimum_required(VERSION 3.6)

find_library( log-lib
              log )

#########################################################
########## Import ffmpeg as apre-built library ##########
#########################################################
set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/libs/ffmpeg)
add_library( avcodec-lib
        SHARED
        IMPORTED )
add_library( avformat-lib
        SHARED
        IMPORTED )
add_library( avutil-lib
        SHARED
        IMPORTED )
add_library( swresample-lib
        SHARED
        IMPORTED )
set_target_properties( # Specifies the target library.
        avcodec-lib
        # Specifies the parameter you want to define.
        PROPERTIES IMPORTED_LOCATION
        # Provides the path to the library you want to import.
        ${FFMPEG_DIR}/lib/${ANDROID_ABI}/libavcodec.so)
set_target_properties( # Specifies the target library.
        avformat-lib
        # Specifies the parameter you want to define.
        PROPERTIES IMPORTED_LOCATION
        # Provides the path to the library you want to import.
        ${FFMPEG_DIR}/lib/${ANDROID_ABI}/libavformat.so)
set_target_properties( # Specifies the target library.
        avutil-lib
        # Specifies the parameter you want to define.
        PROPERTIES IMPORTED_LOCATION
        # Provides the path to the library you want to import.
        ${FFMPEG_DIR}/lib/${ANDROID_ABI}/libavutil.so)
# fix this ?
set_target_properties( # Specifies the target library.
        swresample-lib
        # Specifies the parameter you want to define.
        PROPERTIES IMPORTED_LOCATION
        # Provides the path to the library you want to import.
        ${FFMPEG_DIR}/lib/${ANDROID_ABI}/libswresample.so)


include_directories(${FFMPEG_DIR}/include/${ANDROID_ABI}/)

# add the h264bitstream library
set(H264_BITSTREAM_DIR ${CMAKE_SOURCE_DIR}/libs/h264bitstream)
add_library(h264bitstream
        SHARED
        ${H264_BITSTREAM_DIR}/h264_stream.c
        ${H264_BITSTREAM_DIR}/h264_sei.c
        ${H264_BITSTREAM_DIR}/h264_nal.c
        )
target_link_libraries(h264bitstream
        ${log-lib}
        log
        android)
include_directories(${H264_BITSTREAM_DIR})
# add the h265nal library
set(H265NAL_DIR ${CMAKE_SOURCE_DIR}/libs/h265nal)
set(H265NAL_SRC_DIR ${H265NAL_DIR}/src)
#add_library(webrtc_lib
#        ${H265NAL_DIR}/webrtc/rtc_base/bit_buffer.cc
#        )
#include_directories(${H265NAL_DIR}/absl)
add_subdirectory(${H265NAL_DIR}/webrtc)
add_subdirectory(${H265NAL_DIR}/src)
#add_library(h265nal
#        ${H265NAL_SRC_DIR}/h265_common.h ${H265NAL_SRC_DIR}/h265_common.cc
        #h265_profile_tier_level_parser.h h265_profile_tier_level_parser.cc
        #h265_vps_parser.h h265_vps_parser.cc
        #h265_vui_parameters_parser.h h265_vui_parameters_parser.cc
        #h265_st_ref_pic_set_parser.h h265_st_ref_pic_set_parser.cc
        #h265_sps_parser.h h265_sps_parser.cc
        #h265_pps_parser.h h265_pps_parser.cc
        #h265_aud_parser.h h265_aud_parser.cc
        #h265_rtp_ap_parser.h h265_rtp_ap_parser.cc
        #h265_rtp_fu_parser.h h265_rtp_fu_parser.cc
        #h265_rtp_single_parser.h h265_rtp_single_parser.cc
        #h265_rtp_parser.h h265_rtp_parser.cc
        #h265_slice_parser.h h265_slice_parser.cc
        #h265_bitstream_parser_state.h
        #h265_bitstream_parser.h h265_bitstream_parser.cc
        #h265_nal_unit_parser.h h265_nal_unit_parser.cc
#        )



#######################################################
########## Build the VideoCore native libary ##########
#######################################################

set(DIR_VideoTelemetryShared ${CMAKE_CURRENT_LIST_DIR}/../Shared/src/main/cpp)
# Add the helper directories from the video/telemetry shared folder
set(IO_PATH ${DIR_VideoTelemetryShared}/InputOutput)
set(HELPER_PATH ${DIR_VideoTelemetryShared}/Helper)
set(NDKHELPER_PATH ${DIR_VideoTelemetryShared}/NDKHelper)
include_directories(${HELPER_PATH})
include_directories(${IO_PATH})
include_directories(${NDKHELPER_PATH})

#######################################################
########## Experimental-add the XFEC library ##########
#######################################################
include(${CMAKE_SOURCE_DIR}/src/main/cpp/XFEC/XFEC.cmake)
###############

#Webbn 360 stuff
add_library( FFMpegVideoReceiver
        SHARED
        src/main/cpp/Experiment360/FFMpegVideoReceiver.cpp)
target_link_libraries( FFMpegVideoReceiver
        ${log-lib}
        log
        android
        mediandk
        OpenMAXAL
        avcodec-lib
        avformat-lib
        avutil-lib
        #
        swresample-lib
        h264bitstream
        )

set(VIDEO_PATH src/main/cpp)
add_library( VideoNative
             SHARED
        ${IO_PATH}/UDPReceiver.cpp
        ${IO_PATH}/UDPSender.cpp
        ${IO_PATH}/FileReader.cpp

        ${VIDEO_PATH}/Parser/H264Parser.cpp
        ${VIDEO_PATH}/Parser/ParseRAW.cpp
        ${VIDEO_PATH}/Parser/ParseRTP.cpp
        ${VIDEO_PATH}/Decoder/LowLagDecoder.cpp
        ${VIDEO_PATH}/VideoPlayer/VideoPlayer.cpp
        )

target_link_libraries( VideoNative
        FFMpegVideoReceiver
        ${log-lib}
        log
        android
        mediandk
        OpenMAXAL
        h264bitstream
        XFEC_lib
        # XX
        avcodec-lib
        avformat-lib
        avutil-lib
        swresample-lib
        # XX
        )

# the transmitter stuff
add_library( VideoTransmitter
        SHARED
        ${DIR_VideoTelemetryShared}/InputOutput/UDPSender.cpp
        ${VIDEO_PATH}/Parser/ParseRTP.cpp
        src/main/cpp/VideoTransmitter/VideoTransmitter.cpp
        )

target_link_libraries( VideoTransmitter
        ${log-lib}
        log
        android
        XFEC_lib
        h264bitstream
        )