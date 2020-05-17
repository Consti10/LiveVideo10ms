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
include_directories(${FFMPEG_DIR}/include/${ANDROID_ABI}/)

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

#Webbn 360 stuff
add_library( FFMpegVideoReceiver
        SHARED
        src/main/cpp/Experiment360/FFMpegVideoReceiver.cpp
        )
target_link_libraries( FFMpegVideoReceiver
        ${log-lib}
        log
        android
        mediandk
        OpenMAXAL
        avcodec-lib
        avformat-lib
        avutil-lib
        h264bitstream
        )

#######################################################
########## Build the VideoCore native libary ##########
#######################################################

set(V_CORE_DIR ${CMAKE_SOURCE_DIR})
set(IO_PATH ${V_CORE_DIR}/../VideoTelemetryShared/InputOutput)
set(HELPER_PATH ${V_CORE_DIR}/../VideoTelemetryShared/Helper)
include_directories(${HELPER_PATH})
include_directories(${IO_PATH})

# Without that dummy .cpp file android studio does not properly show .hpp only source directories
add_library(MyDummyLib SHARED ${HELPER_PATH}/ZDummy.cpp)

set(VIDEO_PATH src/main/cpp)
add_library( VideoNative
             SHARED
        ${IO_PATH}/UDPReceiver.cpp
        ${IO_PATH}/FileReader.cpp

        ${VIDEO_PATH}/Parser/H264Parser.cpp
        ${VIDEO_PATH}/Parser/ParseRAW.cpp
        ${VIDEO_PATH}/Parser/ParseRTP.cpp
        ${VIDEO_PATH}/Decoder/LowLagDecoder.cpp
        ${VIDEO_PATH}/VideoPlayer/VideoPlayer.cpp)

target_link_libraries( VideoNative
        FFMpegVideoReceiver
        ${log-lib}
        log
        android
        mediandk
        OpenMAXAL
        h264bitstream)