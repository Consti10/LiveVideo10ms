cmake_minimum_required(VERSION 3.6)

find_library( log-lib
              log )


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
                       ${log-lib}
                       log
                       android
                       mediandk
                       OpenMAXAL )