//
// Created by geier on 20/05/2020.
//

#ifndef FPV_VR_OS_FFMPEGFILEWRITER_H
#define FPV_VR_OS_FFMPEGFILEWRITER_H

#include <iostream>
#include <jni.h>
#include <android/log.h>
#include <chrono>
#include <thread>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}

// https://stackoverflow.com/questions/19679833/muxing-avpackets-into-mp4-file

namespace FFMPEGFileWriter{
    void lol(std::string DIR) {
        avio_open()
    }
}

#endif //FPV_VR_OS_FFMPEGFILEWRITER_H
