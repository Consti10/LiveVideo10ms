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
include_directories(${FFMPEG_DIR}/include)

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
        )

#######################################################
########## Build the VideoCore native libary ##########
#######################################################
include_directories(src/main/cpp/Video)
include_directories(src/main/cpp/General)
set(VIDEO_PATH src/main/cpp)
add_library( VideoNative
             SHARED
        ${VIDEO_PATH}/General/UDPReceiver.cpp

        ${VIDEO_PATH}/Parser/H264Parser.cpp
        ${VIDEO_PATH}/Parser/ParseRAW.cpp
        ${VIDEO_PATH}/Parser/ParseRTP.cpp

        ${VIDEO_PATH}/Decoder/LowLagDecoder.cpp
        ${VIDEO_PATH}/Decoder/SPSHelper.cpp
        ${VIDEO_PATH}/VideoNative/VideoNative.cpp)

target_link_libraries( VideoNative
        FFMpegVideoReceiver
        ${log-lib}
        log
        android
        mediandk
        OpenMAXAL )