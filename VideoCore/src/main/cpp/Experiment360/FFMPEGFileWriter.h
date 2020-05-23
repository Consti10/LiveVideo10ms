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
    void lol(const std::string filename) {
        static AVFormatContext *ofmt_ctx=nullptr;
        avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename.c_str());
        if (!ofmt_ctx) {
            MLOGE<<"avformat_alloc_output_context2";
            return;
        }

        if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            MLOGD<<"Calling avio_open";
            auto ret = avio_open(&ofmt_ctx->pb, filename.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0) {
                MLOGE<<"Could not open output file";
                return;
            }
        }

        /* init muxer, write output file header */
        auto ret = avformat_write_header(ofmt_ctx, NULL);
        if (ret < 0) {
            MLOGE<<"Error occurred when calling avformat_write_header";
            return;
        }
        avio_closep(&ofmt_ctx->pb);
    }



}

#endif //FPV_VR_OS_FFMPEGFILEWRITER_H
