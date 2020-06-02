//
// Created by geier on 24/05/2020.
//

#include "SimpleTranscoder.h"
#include <AndroidLogger.hpp>
#include <chrono>
#include <asm/fcntl.h>
#include <fcntl.h>
#include <unistd.h>
#include <FileHelper.hpp>
#include <AColorFormats.hpp>
#include <APixelBuffers.hpp>
#include <NDKArrayHelper.hpp>
#include <utility>
#include <NDKThreadHelper.hpp>
#include <chrono>

SimpleTranscoder::SimpleTranscoder(std::string TEST_FILE_DIRECTORY1,std::string INPUT_FILE_PATH1) :
DEBUG_USE_PATTERN_INSTEAD(INPUT_FILE_PATH1.length()==0),
TEST_FILE_DIRECTORY(std::move(TEST_FILE_DIRECTORY1)),
INPUT_FILE_PATH(std::move(INPUT_FILE_PATH1)),
OUTPUT_FILE_PATH(DEBUG_USE_PATTERN_INSTEAD ? FileHelper::findUnusedFilename(TEST_FILE_DIRECTORY, ".mp4") : FileHelper::changeFileContainerFPVtoMP4(INPUT_FILE_PATH))
{
    MLOGD << INPUT_FILE_PATH << " " << OUTPUT_FILE_PATH << " " << TEST_FILE_DIRECTORY;
    MLOGD<<"DEBUG_USE_PATTERN_INSTEAD "<<DEBUG_USE_PATTERN_INSTEAD;
    MLOGD<<"DELETE_FILES_WHEN_DONE "<<DELETE_FILES_WHEN_DONE;
    using namespace MediaCodecInfo::CodecCapabilities;
    SELECTED_ENCODER_COLOR_FORMAT=COLOR_FormatYUV420SemiPlanar;
    mediaCodec=openMediaCodecEncoder(SELECTED_ENCODER_COLOR_FORMAT);
    if(mediaCodec== nullptr){
        SELECTED_ENCODER_COLOR_FORMAT=COLOR_FormatYUV420Planar;
        mediaCodec=openMediaCodecEncoder(SELECTED_ENCODER_COLOR_FORMAT);
        if(mediaCodec== nullptr){
            MLOGE<<"Cannot create encoder for YUV420XXX color format";
            return;
        }
    }
    if(!DEBUG_USE_PATTERN_INSTEAD){
        fileReaderMjpeg.open(INPUT_FILE_PATH);
    }
    const auto mediaStatus=AMediaCodec_start(mediaCodec);
    MLOGD<<"AMediaCodec_start returned "<<mediaStatus;
}

SimpleTranscoder::~SimpleTranscoder() {
    if(mediaMuxer!=nullptr){
        AMediaMuxer_stop(mediaMuxer);
        AMediaMuxer_delete(mediaMuxer);
        close(outputFileFD);
        MLOGD<<"Wrote file";
    }
    AMediaCodec_stop(mediaCodec);
    AMediaCodec_delete(mediaCodec);
    if(!DEBUG_USE_PATTERN_INSTEAD){
        fileReaderMjpeg.close();
    }
    if(DELETE_FILES_WHEN_DONE){
        if(successfullyTranscodedWholeFile){
            MLOGD << "Deleting input file (successfully transcoded whole file) " << INPUT_FILE_PATH;
            std::remove(INPUT_FILE_PATH.c_str());
        }else{
            MLOGD<<"Deleting output file (interrupted early) "<<OUTPUT_FILE_PATH;
            std::remove(OUTPUT_FILE_PATH.c_str());
        }
    }
}

AMediaCodec* SimpleTranscoder::openMediaCodecEncoder(const int wantedColorFormat) {
    AMediaFormat* format = AMediaFormat_new();
    auto ret = AMediaCodec_createEncoderByType("video/avc");
    //auto ret= AMediaCodec_createCodecByName("OMX.google.h264.encoder");

    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, "video/avc");
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, WIDTH);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, HEIGHT);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, BIT_RATE);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, FRAME_RATE);
    AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_I_FRAME_INTERVAL,30);
    AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_STRIDE,WIDTH);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT,wantedColorFormat);

    auto status=AMediaCodec_configure(ret,format, nullptr, nullptr, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);

    MLOGD<<"Media format:"<<AMediaFormat_toString(format);

    AMediaFormat_delete(format);
    if (AMEDIA_OK != status) {
        MLOGE<<"AMediaCodec_configure returned"<<status<<" Error with color format "<<wantedColorFormat;
        return nullptr;
    }
    MLOGD<<"Opened MediaCodec";
    return ret;
}

void SimpleTranscoder::loopEncoder(JNIEnv* env) {
    const bool PLANAR= (SELECTED_ENCODER_COLOR_FORMAT==MediaCodecInfo::CodecCapabilities::COLOR_FormatYUV420Planar);
    while(true){
        // Dequeue input buffer
        if(DEBUG_USE_PATTERN_INSTEAD){
            const auto index=AMediaCodec_dequeueInputBuffer(mediaCodec,TIMEOUT_US);
            if(index>0){
                size_t inputBufferSize;
                void* buf = AMediaCodec_getInputBuffer(mediaCodec,(size_t)index,&inputBufferSize);
                MLOGD<<"Got input buffer "<<inputBufferSize;
                if(JThread::isInterrupted(env)){
                    MLOGD<<"Feeding EOS because interrupted";
                    AMediaCodec_queueInputBuffer(mediaCodec,index,0,0,frameTimeUs,AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                    break;
                }else{
                    if(PLANAR){
                        auto framebuffer=APixelBuffers::YUV420Planar(buf,WIDTH, HEIGHT);
                        assert(inputBufferSize>=framebuffer.SIZE_BYTES);
                        YUVFrameGenerator::generateFrame(framebuffer,frameIndex);
                        AMediaCodec_queueInputBuffer(mediaCodec,index,0,framebuffer.SIZE_BYTES,frameTimeUs,0);
                    }else{
                        auto framebuffer=APixelBuffers::YUV420SemiPlanar(buf,WIDTH, HEIGHT);
                        assert(inputBufferSize>=framebuffer.SIZE_BYTES);
                        YUVFrameGenerator::generateFrame(framebuffer,frameIndex);
                        AMediaCodec_queueInputBuffer(mediaCodec,index,0,framebuffer.SIZE_BYTES,frameTimeUs,0);
                    }
                    frameIndex++;
                    frameTimeUs+=8*1000;
                }
            }
        }else{
            const auto index=AMediaCodec_dequeueInputBuffer(mediaCodec,5*1000);
            if(index>0) {
                size_t inputBufferSize;
                void *buf = AMediaCodec_getInputBuffer(mediaCodec, (size_t) index,
                                                       &inputBufferSize);
                MLOGD << "Got input buffer " << inputBufferSize;
                int mjpegFrameIndex;
                auto mjpegData = fileReaderMjpeg.getNextMJPEGPacket(mjpegFrameIndex);
                if (JThread::isInterrupted(env)) {
                    MLOGD << "Transcoding was interrupted. Should delete file";
                    mjpegData = std::nullopt;
                }
                if (mjpegData == std::nullopt) {
                    AMediaCodec_queueInputBuffer(mediaCodec, index, 0, 0, frameTimeUs,AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                    successfullyTranscodedWholeFile = true;
                    break;
                } else {
                    MLOGD << "Got mjpeg" << mjpegData->size() << " idx" << mjpegFrameIndex;
                    auto decodeBuffer = APixelBuffers::YUV422Planar(640, 480);
                    mjpegDecodeAndroid.decodeRawYUV422(mjpegData->data(), mjpegData->size(),decodeBuffer);
                    auto encoderBuffer = APixelBuffers::YUV420SemiPlanar(buf, 640, 480);
                    APixelBuffers::copyTo(decodeBuffer, encoderBuffer);
                    frameTimeUs += 8 * 1000;
                }
                AMediaCodec_queueInputBuffer(mediaCodec, index, 0, inputBufferSize, frameTimeUs, 0);
                frameTimeUs += 8 * 1000;
            }
        }
        // Dequeue output buffer
        AMediaCodecBufferInfo info;
        const auto index=AMediaCodec_dequeueOutputBuffer(mediaCodec,&info,TIMEOUT_US);
        AMediaCodec_getOutputFormat(mediaCodec);
        if(index==AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED){
            MLOGD<<"AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED";
            outputFileFD = open(OUTPUT_FILE_PATH.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
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
                MLOGD<<" Got EOS in output buffer";
                break;
            }
        }
        MLOGD<<"Hi from worker";
    }
}

// ------------------------------------- Native Bindings -------------------------------------
#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_test_SimpleTranscoder_##method_name
extern "C" {

JNI_METHOD(jlong, nativeCreate)
(JNIEnv *env, jclass jclass1,jstring INPUT_FILE_PATH,jstring TEST_FILE_DIRECTORY) {
    auto* simpleEncoder=new SimpleTranscoder(NDKArrayHelper::DynamicSizeString(env, TEST_FILE_DIRECTORY),NDKArrayHelper::DynamicSizeString(env, INPUT_FILE_PATH));
    return reinterpret_cast<intptr_t>(simpleEncoder);
}

JNI_METHOD(void, nativeDelete)
(JNIEnv *env, jclass jclass1,jlong simpleEncoder) {
    auto* simpleEncoder1=reinterpret_cast<SimpleTranscoder*>(simpleEncoder);
    delete simpleEncoder1;
}

JNI_METHOD(void, nativeLoop)
(JNIEnv *env, jclass jclass1,jlong simpleEncoder) {
    auto* simpleEncoder1=reinterpret_cast<SimpleTranscoder*>(simpleEncoder);
    simpleEncoder1->loopEncoder(env);
}

}