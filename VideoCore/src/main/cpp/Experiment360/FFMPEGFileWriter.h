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

namespace CustomFFMPEGLog{
    void custom_log(void *ptr, int level, const char* fmt, va_list vl){
        __android_log_print(ANDROID_LOG_DEBUG,"FFMPEG",fmt,vl);
    }
    void set(){
        av_log_set_callback(CustomFFMPEGLog::custom_log);
    }
}

class TestX{
private:
    AVFormatContext *ifmt_ctx;
    AVFormatContext *ofmt_ctx;
    typedef struct StreamContext {
        AVCodecContext *dec_ctx;
        AVCodecContext *enc_ctx;
    } StreamContext;
    StreamContext *stream_ctx;
    //AVCodecContext* xDec_ctx;

    int open_input_file(const char *filename)
    {
        int ret;

        ifmt_ctx = NULL;
        if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
            return ret;
        }

        if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
            return ret;
        }

        // only 1 stream (e.g. no audio)
        MLOGD<<"N streams "<<ifmt_ctx->nb_streams;

        stream_ctx =(StreamContext*) av_mallocz_array(1, sizeof(*stream_ctx));
        if (!stream_ctx)
            return AVERROR(ENOMEM);

        AVStream *stream = ifmt_ctx->streams[0];
        AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *codec_ctx;
        if (!dec) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", 0);
            return AVERROR_DECODER_NOT_FOUND;
        }
        codec_ctx = avcodec_alloc_context3(dec);
        if (!codec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", 0);
            return AVERROR(ENOMEM);
        }
        ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                                       "for stream #%u\n", 0);
            return ret;
        }
        /* Reencode video & audio and remux subtitles etc. */
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
            || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
                codec_ctx->framerate = av_guess_frame_rate(ifmt_ctx, stream, NULL);
            /* Open decoder */
            ret = avcodec_open2(codec_ctx, dec, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", 0);
                return ret;
            }
        }
        stream_ctx[0].dec_ctx = codec_ctx;

        av_dump_format(ifmt_ctx, 0, filename, 0);
        return 0;
    }
    int open_output_file(const char *filename){
        AVStream *out_stream;
        AVStream *in_stream;
        AVCodecContext *dec_ctx, *enc_ctx;
        AVCodec *encoder;
        int ret;


        ofmt_ctx = NULL;
        avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
        if (!ofmt_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
            return AVERROR_UNKNOWN;
        }


        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
            return AVERROR_UNKNOWN;
        }

        in_stream = ifmt_ctx->streams[0];
        dec_ctx = stream_ctx[0].dec_ctx;

        if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            /* in this example, we choose transcoding to same codec */
            MLOGD<<"dec_ctx->codec_id"<<dec_ctx->codec_id;
            if(dec_ctx->codec_id==AV_CODEC_ID_MJPEG){
                MLOGD<<"Is =AV_CODEC_ID_MJPEG";
            }

            encoder = avcodec_find_encoder(dec_ctx->codec_id);
            if (!encoder) {
                av_log(NULL, AV_LOG_FATAL, "Necessary encoder not found\n");
                return AVERROR_INVALIDDATA;
            }
            enc_ctx = avcodec_alloc_context3(encoder);
            if (!enc_ctx) {
                av_log(NULL, AV_LOG_FATAL, "Failed to allocate the encoder context\n");
                return AVERROR(ENOMEM);
            }

            /* In this example, we transcode to same properties (picture size,
             * sample rate etc.). These properties can be changed for output
             * streams easily using filters */
            if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                enc_ctx->height = dec_ctx->height;
                enc_ctx->width = dec_ctx->width;
                enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
                /* take first format from list of supported formats */
                if (encoder->pix_fmts){
                    enc_ctx->pix_fmt = encoder->pix_fmts[0];
                }else{
                    enc_ctx->pix_fmt = dec_ctx->pix_fmt;
                }
                MLOGD<<"encoder->pix_fmts"<<encoder->pix_fmts[0];
                if(encoder->pix_fmts[0]==AV_PIX_FMT_YUVJ420P){
                    MLOGD<<"AV_PIX_FMT_YUVJ420P";
                }
                /* video time_base can be set to whatever is handy and supported by encoder */
                enc_ctx->time_base = av_inv_q(dec_ctx->framerate);
            }

            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER){
                MLOGD<<"AVFMT_GLOBALHEADER";
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }

            /* Third parameter can be used to pass settings to encoder */
            ret = avcodec_open2(enc_ctx, encoder, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", 0);
                return ret;
            }
            ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream #%u\n", 0);
                return ret;
            }

            out_stream->time_base = enc_ctx->time_base;
            stream_ctx[0].enc_ctx = enc_ctx;
        }



        av_dump_format(ofmt_ctx, 0, filename, 1);

        if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            MLOGD<<"Opening file";
            ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
                return ret;
            }
        }
        MLOGD<<"Writing header";
        /* init muxer, write output file header */
        ret = avformat_write_header(ofmt_ctx, NULL);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
            return ret;
        }

        return 0;
    }

public:
    void test(const std::string filenameIn,const std::string filenameOut){
        CustomFFMPEGLog::set();
        int ret;
        if ((ret = open_input_file(filenameIn.c_str())) < 0)
           MLOGE<<"open input file";
        if ((ret = open_output_file(filenameOut.c_str())) < 0)
            MLOGE<<"open output file";

        AVPacket packet = { .data = NULL, .size = 0 };
        unsigned int stream_index;
        enum AVMediaType type;
        while (1) {
            if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0)
                break;
            MLOGD<<"Packet duration"<<packet.duration<<" size"<<packet.size;
            stream_index = packet.stream_index;
            MLOGD<<"packet.stream_index"<<packet.stream_index;
            type = ifmt_ctx->streams[packet.stream_index]->codecpar->codec_type;
            av_log(NULL, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n",
                   stream_index);
            av_packet_rescale_ts(&packet,
                                 ifmt_ctx->streams[stream_index]->time_base,
                                 ofmt_ctx->streams[stream_index]->time_base);

            ret = av_interleaved_write_frame(ofmt_ctx, &packet);
        }
    }
};

// https://stackoverflow.com/questions/19679833/muxing-avpackets-into-mp4-file
namespace FFMPEGFileWriter{
    static int open_codec_context(int *stream_idx,AVFormatContext *fmt_ctx, enum AVMediaType type){
        int ret;
        AVStream *st;
        AVCodecContext *dec_ctx = NULL;
        AVCodec *dec = NULL;
        ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
        if (ret < 0) {
            MLOGD<<"Could not find "<<av_get_media_type_string(type)<<" stream";
            return ret;
        } else {
            *stream_idx = ret;
            st = fmt_ctx->streams[*stream_idx];
            /* find decoder for the stream */
            dec_ctx = st->codec;
            dec = avcodec_find_decoder(dec_ctx->codec_id);
            if (!dec) {
                MLOGD<<"Failed to find "<<av_get_media_type_string(type)<<" codec";
                return ret;
            }
            if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
                MLOGE<<"Failed to open"<<av_get_media_type_string(type)<<"codec";
                return ret;
            }
        }
        return 0;
    }


    void input(const std::string src_filename){
        CustomFFMPEGLog::set();

        AVFormatContext* avFormatContext=avformat_alloc_context();

        auto avInformat = av_find_input_format("mjpeg");
        MLOGD<<"Found "<<avInformat;

        /* open input file, and allocate format context */
        if (avformat_open_input(&avFormatContext, src_filename.c_str(), NULL, NULL) < 0) {
            MLOGE<<"Could not open source file "<<src_filename;
            return;
        }
        /* retrieve stream information */
        if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
            MLOGE<<"Could not find stream information";
            return;
        }
        MLOGD<<"opened stream";
        int video_stream_idx=-1;
        if (open_codec_context(&video_stream_idx, avFormatContext, AVMEDIA_TYPE_VIDEO) >= 0) {
           MLOGD<<"opened codec context";
        }
        av_dump_format(avFormatContext, 0,src_filename.c_str(), 0);
        auto  frame =av_frame_alloc();


        avformat_close_input(&avFormatContext);


    }


    void try1(const std::string filename) {
        CustomFFMPEGLog::set();

        //AVOutputFormat * outFmt = av_guess_format("mp4", NULL, NULL);
        AVOutputFormat * avOutputFormat = av_guess_format(NULL, "mp4", "video/mp4");
        if(avOutputFormat == nullptr){
            MLOGE<<"av_guess_format";
            return;
        }
        MLOGD<<"Success av_guess_format";
        AVFormatContext *avFormatContext = nullptr;
        avformat_alloc_output_context2(&avFormatContext, avOutputFormat, NULL, NULL);
        if(avFormatContext == nullptr){
            return;
        }
        MLOGD<<"Success avformat_alloc_output_context2";
        AVStream * avStream = avformat_new_stream(avFormatContext, 0);
        if(avStream == nullptr){
            return;
        }
        MLOGD<<"Success avformat_new_stream";

        // this writes into outFmtCtx->pb AVIOContext
        int ret=avio_open(&avFormatContext->pb, filename.c_str(), AVIO_FLAG_WRITE);
        if(ret<0){
            MLOGE<<"avio_open"<<av_err2str(ret);
        }

        //
        //auto encoder = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
        //if (!encoder) {
        //    av_log(NULL, AV_LOG_FATAL, "Cannot find MJPEG codec");
        //}
        //auto encoderContext=avcodec_alloc_context3(encoder);
        //

        //AVCodec * codec = nullptr;
        //avcodec_get_context_defaults3(outStrm->codec, codec);
        //outStrm->codec->coder_type = AVMEDIA_TYPE_VIDEO;
        //AVCodecContext * avCodecContext=avcodec_alloc_context3(nullptr);
        //outStrm->codecpar=AVMEDIA_TYPE_VIDEO;
        avStream->codecpar->codec_id=AV_CODEC_ID_MJPEG;
        avStream->codecpar->codec_type=AVMEDIA_TYPE_VIDEO;
        avStream->codecpar->format=AV_PIX_FMT_YUV420P;
        avStream->codecpar->width=1280;
        avStream->codecpar->height=720;
        avStream->time_base=AVRational{1,1};
        avStream->sample_aspect_ratio = AVRational{1,1};
        /* take first format from list of supported formats */


        MLOGD<<"Tag is "<<av_codec_get_tag(avOutputFormat->codec_tag,AV_CODEC_ID_MJPEG);

        //MLOGD<<"X "<<avOutputFormat->query_codec(AV_CODEC_ID_MJPEG,FF_COMPLIANCE_EXPERIMENTAL);

        MLOGD<<"Supported codec ?:"<<avformat_query_codec(avOutputFormat,AV_CODEC_ID_MJPEG,FF_COMPLIANCE_EXPERIMENTAL);

        ret=avformat_write_header(avFormatContext, NULL);
        MLOGD<<"Write header returned"<<av_err2str(ret);

        AVPacket avPacket{0};
        avPacket.data=new uint8_t[1024];
        avPacket.size=1024;
        ret=av_write_frame(avFormatContext, &avPacket);
        MLOGD<<"write frame "<<ret;

        avio_close(avFormatContext->pb);

        avformat_free_context(avFormatContext);
    }

    void lol1(const std::string filename) {
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
