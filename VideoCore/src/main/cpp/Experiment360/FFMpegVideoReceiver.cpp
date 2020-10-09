
#include <iostream>
#include <jni.h>
#include <android/log.h>
#include <chrono>
#include <thread>

#include "FFMpegVideoReceiver.h"

#include <h264_stream.h>
#include <AndroidLogger.hpp>


//compilation without any changes (ok) but run time error on Pixel api 28
///lib/x86/libavcodec.so" has text relocations (https://android.googlesource.com/platform/bionic/+/master/android-changes-for-ndk-developers.md#Text-Relocations-Enforced-for-API-level-23)
//Pixel 3 API 25:
//same error

//Changing targetSdkVersion to 22 does not compile (android minimum error)
//Running 'Rebuild' in studio
//Still runtime error libavcodec.so: has text relocations
//ZTE Axon 7:
//java.lang.UnsatisfiedLinkError: dlopen failed: cannot locate symbol "iconv_close" referenced by "/data/app/com.constantin.wilson.FPV_VR-mxC0B6_-lmepeN9MZfItsw==/lib/arm64/libavcodec.so"...

static int g_shutdown_callback(void *d) {
    return reinterpret_cast<FFMpegVideoReceiver*>(d)->shutdown_callback();
}

FFMpegVideoReceiver::FFMpegVideoReceiver(std::string url, int cpu_priority,
                                         std::function<void(uint8_t[],int)> callback,
                                         NALU_DATA_CALLBACK callback2,uint32_t bufsize) :
        m_url(url), m_cpu_priority(cpu_priority), m_bufsize(bufsize), m_shutdown(false),
        raw_h264_data_callback(callback),onNewNALUCallback(callback2) {

    // Allocate the packet
    av_init_packet(&m_packet);

    av_register_all();

    // Register the network interface
    avformat_network_init();
}

void FFMpegVideoReceiver::start_playing() {
    m_shutdown = false;
    m_thread.reset(new std::thread([this] { this->run(); }));
}

void FFMpegVideoReceiver::stop_playing() {
    m_shutdown = true;
    if (m_thread) {
        m_thread->join();
        m_thread.reset();
    }
}

void FFMpegVideoReceiver::run() {
    MLOGD<<"FFMPeg playing: "<<m_url.c_str();

    // Add a callback that we can use to shutdown.
    m_format_ctx = avformat_alloc_context();
    m_format_ctx->interrupt_callback.callback = g_shutdown_callback;
    m_format_ctx->interrupt_callback.opaque = this;

    // Configure some options for the input stream.
    AVDictionary *opts = 0;
    //av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    // Timeout in usec.
    av_dict_set_int(&opts, "stimeout", 1000000, 0);
    av_dict_set_int(&opts, "rw_timeout", 1000000, 0);
    av_dict_set_int(&opts, "reorder_queue_size", 1, 0);
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);

    // Open the input stream.
    if (avformat_open_input(&m_format_ctx, m_url.c_str(), NULL, &opts) != 0) {
        LOG_TERMINATING_ERROR("Error connecting to: "+m_url);
        return;
    }

    // Start reading packets from stream
    if (avformat_find_stream_info(m_format_ctx, 0) < 0) {
        LOG_TERMINATING_ERROR("Can't find video stream info");
        return;
    }

    // Find the video stream.
    m_video_stream = av_find_best_stream(m_format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (m_video_stream < 0) {
        LOG_TERMINATING_ERROR("Can't find video stream in input file");
        return;
    }

    AVCodecParameters *origin_par = m_format_ctx->streams[m_video_stream]->codecpar;

    // Find the decoder
    if (!(m_codec = avcodec_find_decoder(origin_par->codec_id))) {
        LOG_TERMINATING_ERROR("Error finding the decoder");
        return;
    }

    // Allocate the context
    if (!(m_codec_ctx = avcodec_alloc_context3(m_codec))) {
        LOG_TERMINATING_ERROR("Error opening the decoding context");
        return;
    }

    if (avcodec_parameters_to_context(m_codec_ctx, origin_par)) {
        LOG_TERMINATING_ERROR("Error copying the decoder context");
        return;
    }

    // Open the codec
    if (avcodec_open2(m_codec_ctx, m_codec, 0) < 0) {
        LOG_TERMINATING_ERROR("Error opening the decoding codec");
        return;
    }

    m_pCodecPaser = av_parser_init(AV_CODEC_ID_H264);
    if(!m_pCodecPaser){
        LOG_TERMINATING_ERROR("Cannot find parser");
    }
    m_frame=av_frame_alloc();
    if(!m_frame){
        LOG_TERMINATING_ERROR("Could not allocate video frame");
    }

    MLOGD<<"Started receiving";

    int ret;
    uint8_t *data;
    size_t   data_size;
    AVPacket* pkt=av_packet_alloc();

    int naluC=0;

    AVFrame *frame = av_frame_alloc();

    bool alreadySentSPS_PPS=false;

    while (av_read_frame(m_format_ctx, &m_packet) >= 0) {
        if (m_shutdown) {
            break;
        }
        if (m_packet.stream_index == m_video_stream) {
            currentlyReceivedVideoData+=m_packet.size;
            //LOGV("Read packet: %d", m_packet.size);
            //NALU nalu(m_packet.data, m_packet.size);
            raw_h264_data_callback(m_packet.data,m_packet.size);

            /*while(m_packet.size>0){
                if(m_codec_ctx->extradata_size>0 && !alreadySentSPS_PPS){
                    const NALU extradataNALU(m_codec_ctx->extradata,m_codec_ctx->extradata_size);
                    LOGV("Extradata %d is %s",m_codec_ctx->extradata_size,extradataNALU.get_nal_name().c_str());
                    //The extradata from ffmpeg needs to be split into sps and pps
                    //raw_h264_data_callback(m_codec_ctx->extradata,m_codec_ctx->extradata_size);
                    //alreadySentSPS_PPS=true;

                    h264_stream_t* h = h264_new();
                    read_debug_nal_unit(h,&m_codec_ctx->extradata[4],m_codec_ctx->extradata_size-4);
                    h264_free(h);

                }
                ret = av_parser_parse2(m_pCodecPaser, m_codec_ctx, &pkt->data,&pkt->size,
                                       m_packet.data,m_packet.size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
                if (ret < 0) {
                    LOGV("Error while parsing");
                    break;
                }
                m_packet.data+=ret;
                m_packet.size-=ret;
                if(pkt->size){
                    //TODO: Investigate: Looks like the av_parser_parse2 function does not split sps/pps
                    //Submits them together or something ?
                    if(naluC<2){
                        //raw_h264_data_callback(pkt->data,pkt->size);
                    }else{
                        //onNewNALUCallback(NALU(pkt->data,pkt->size));
                    }
                    naluC++;
                    //if(lala>10){
                        //onNewNALUCallback(pkt->data,pkt->size);
                    //}else{
                        //raw_h264_data_callback(pkt->data,pkt->size);
                    //}
                    //onNewNALUCallback(pkt->data,pkt->size);
                    //LOGV("key frame %d w %d h %d",m_pCodecPaser->key_frame,m_pCodecPaser->width,m_pCodecPaser->height);
                }
                /*avcodec_send_packet(m_codec_ctx,&m_packet);
                int response = avcodec_receive_frame(m_codec_ctx, frame);
                LOGV("Repsonse %d",response);*/
            //}
            //--
        }
    }

    // Close the codec context
    if (avcodec_close(m_codec_ctx) != 0) {
        MLOGD<<"Error closing the AV codec context";
        return;
    }
    av_free(m_codec_ctx);

    // Close the format context
    avformat_close_input(&m_format_ctx);

    av_parser_close(m_pCodecPaser);
}

int FFMpegVideoReceiver::shutdown_callback() {
    if (m_shutdown) {
        MLOGD<<"Shutting down";
        return 1;
    }
    return 0;
}

void FFMpegVideoReceiver::LOG_TERMINATING_ERROR(const std::string message) {
    MLOGD<<message;
    currentErrorMessage=message;
}
