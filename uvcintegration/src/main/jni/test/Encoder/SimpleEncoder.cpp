//
// Created by geier on 24/05/2020.
//

#include "SimpleEncoder.h"
#include <AndroidLogger.hpp>
#include <chrono>
#include <asm/fcntl.h>
#include <fcntl.h>
#include <unistd.h>
#include <FileHelper.hpp>
#include "AndroidColorFormats.hpp"

void SimpleEncoder::start() {
    running=true;
    mEncoderThread=new std::thread(&SimpleEncoder::loopEncoder, this);
}

void SimpleEncoder::stop() {
    running=false;
    if(mEncoderThread->joinable()){
        mEncoderThread->join();
    }
    delete(mEncoderThread);
    mEncoderThread= nullptr;
}

bool SimpleEncoder::openMediaCodecEncoder(const int wantedColorFormat) {
    AMediaFormat* format = AMediaFormat_new();
    mediaCodec = AMediaCodec_createEncoderByType("video/avc");
    //mediaCodec= AMediaCodec_createCodecByName("OMX.google.h264.encoder");

    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, "video/avc");
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, HEIGHT);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, WIDTH);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, BIT_RATE);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, FRAME_RATE);
    AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_I_FRAME_INTERVAL,30);

    AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_STRIDE,640);
    //AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_STRIDE,640);
    //AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_COLOR_FORMAT,)
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT,wantedColorFormat);

    auto status=AMediaCodec_configure(mediaCodec,format, nullptr, nullptr, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);

    MLOGD<<"Media format:"<<AMediaFormat_toString(format);

    AMediaFormat_delete(format);
    if (AMEDIA_OK != status) {
        MLOGE<<"AMediaCodec_configure returned"<<status;
        return false;
    }
    MLOGD<<"Opened MediaCodec";
    return true;
}


void SimpleEncoder::loopEncoder() {
    using namespace MediaCodecInfo::CodecCapabilities;
    constexpr auto ENCODER_COLOR_FORMAT=COLOR_FormatYUV420SemiPlanar;
    openMediaCodecEncoder(ENCODER_COLOR_FORMAT);

    fileReaderMjpeg.open(INPUT_FILE);

    AMediaCodec_start(mediaCodec);
    int frameTimeUs=0;
    int frameIndex=0;
    while(true){
        // Get input buffer if possible
        {
            const auto index=AMediaCodec_dequeueInputBuffer(mediaCodec,5*1000);
            if(index>0){
                size_t inputBufferSize;
                void* buf = AMediaCodec_getInputBuffer(mediaCodec,(size_t)index,&inputBufferSize);
                MLOGD<<"Got input buffer "<<inputBufferSize;
                if(!running){
                    AMediaCodec_queueInputBuffer(mediaCodec,index,0,0,frameTimeUs,AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                    break;
                }else{
                    auto framebuffer=MyColorSpaces::YUV420SemiPlanar(buf,WIDTH,HEIGHT);
                    YUVFrameGenerator::generateFrame(framebuffer,frameIndex);
                    AMediaCodec_queueInputBuffer(mediaCodec,index,0,inputBufferSize,frameTimeUs,0);
                    frameIndex++;
                    frameTimeUs+=8*1000;
                }
            }
            /*const auto index=AMediaCodec_dequeueInputBuffer(mediaCodec,5*1000);
            if(index>0){
                size_t inputBufferSize;
                void* buf = AMediaCodec_getInputBuffer(mediaCodec,(size_t)index,&inputBufferSize);
                MLOGD<<"Got input buffer "<<inputBufferSize;
                int mjpegFrameIndex;
                auto mjpegData= fileReaderMjpeg.getNextMJPEGPacket(mjpegFrameIndex);
                if(mjpegData==std::nullopt){
                    AMediaCodec_queueInputBuffer(mediaCodec,index,0,0,frameTimeUs,AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                    break;
                }else{
                    MLOGD<<"Got mjpeg"<<mjpegData->size()<<" idx"<<mjpegFrameIndex;
                    auto decodeBuffer= MyColorSpaces::YUV422Planar(640,480);
                    mjpegDecodeAndroid.decodeToYUV422(mjpegData->data(),mjpegData->size(),decodeBuffer);
                    auto encoderBuffer=MyColorSpaces::YUV420SemiPlanar(buf,640,480);
                    MyColorSpaces::copyTo(decodeBuffer,encoderBuffer);
                    frameTimeUs+=8*1000;
                }
                AMediaCodec_queueInputBuffer(mediaCodec,index,0,inputBufferSize,frameTimeUs,0);
                frameTimeUs+=8*1000;
            }*/
        }
        {
            AMediaCodecBufferInfo info;
            const auto index=AMediaCodec_dequeueOutputBuffer(mediaCodec,&info,5*1000);
            AMediaCodec_getOutputFormat(mediaCodec);
            if(index==AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED){
                MLOGD<<"AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED";
                const std::string fn=FileHelper::findUnusedFilename(GROUND_RECORDING_DIRECTORY,"mp4");
                outputFileFD = open(fn.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
                mediaMuxer=AMediaMuxer_new(outputFileFD, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
                AMediaFormat* format=AMediaCodec_getOutputFormat(mediaCodec);
                MLOGD<<"Output format:"<<AMediaFormat_toString(format);
                videoTrackIndex=AMediaMuxer_addTrack(mediaMuxer,format);
                const auto status=AMediaMuxer_start(mediaMuxer);
                MLOGD<<"Media Muxer status "<<status;
                AMediaFormat_delete(format);
            }else if(index==AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED){
                MLOGD<<"AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED";
            }
            if(index>0){
                MLOGD<<"Got output buffer "<<index;
                size_t outputBufferSize;
                const auto* data = (const uint8_t*)AMediaCodec_getOutputBuffer(mediaCodec,(size_t)index,&outputBufferSize);

                AMediaMuxer_writeSampleData(mediaMuxer,videoTrackIndex,data,&info);

                AMediaCodec_releaseOutputBuffer(mediaCodec,index,false);

                if((info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0){
                    MLOGD<<" Got EOS in output";
                    break;
                }

            }
        }
        MLOGD<<"Hi from worker";
    }
    if(mediaMuxer!=nullptr){
        AMediaMuxer_stop(mediaMuxer);
        AMediaMuxer_delete(mediaMuxer);
        close(outputFileFD);
    }
    AMediaCodec_stop(mediaCodec);

    AMediaCodec_delete(mediaCodec);

    fileReaderMjpeg.close();
}


