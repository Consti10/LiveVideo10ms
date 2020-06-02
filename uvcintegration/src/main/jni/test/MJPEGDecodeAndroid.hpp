//
// Created by geier on 07/05/2020.
//

#ifndef UVCCAMERA_MJPEGDECODEANDROID_HPP
#define UVCCAMERA_MJPEGDECODEANDROID_HPP

//#include "HuffTables.hpp"
#include <jpeglib.h>
#include <jni.h>
#include <android/native_window_jni.h>
#include <setjmp.h>
#include <AndroidLogger.hpp>
#include <TimeHelper.hpp>
#include <vector>
#include <APixelBuffers.hpp>
#include <AColorFormats.hpp>

// Since I only need to support android it is cleaner to write my own conversion function.
// inspired by the uvc_mjpeg_to_rgbx .. functions
// Including this file adds dependency on Android and libjpeg-turbo
// now class since then I can reuse the jpeg_decompress_struct dinfo member (and do not need to re-allocate & init)
class MJPEGDecodeAndroid{
private:
    //  error handling (must be set !)
    struct error_mgr {
        struct jpeg_error_mgr super;
        jmp_buf jmp;
    };
    static void _error_exit(j_common_ptr dinfo) {
        struct error_mgr *myerr = (struct error_mgr *) dinfo->err;
        char err_msg[1024];
        (*dinfo->err->format_message)(dinfo, err_msg);
        err_msg[1023] = 0;
        MLOGD<<"LIBJPEG ERROR %s"<<err_msg;
        longjmp(myerr->jmp, 1);
    }
    // We need to set the error manager every time else it will crash (I have no idea why )
    // https://stackoverflow.com/questions/11613040/why-does-jpeg-decompress-create-crash-without-error-message
    void setErrorManager(){
        struct error_mgr jerr;
        dinfo.err = jpeg_std_error(&jerr.super);
        jerr.super.error_exit = _error_exit;
    }
public:
    MJPEGDecodeAndroid(){
        struct error_mgr jerr;
        dinfo.err = jpeg_std_error(&jerr.super);
        jerr.super.error_exit = _error_exit;
        setErrorManager();
        jpeg_create_decompress(&dinfo);
    }
    ~MJPEGDecodeAndroid(){
        jpeg_destroy_decompress(&dinfo);
    }
private:
   struct jpeg_decompress_struct dinfo;
    // 'Create array with pointers to an array'
    static std::vector<uint8_t*> convertToPointers(uint8_t* array1d, size_t heightInPx, size_t scanline_width){
        std::vector<uint8_t*> ret(heightInPx);
        for(int i=0; i < heightInPx; i++){
            ret[i]=array1d+i*scanline_width;
        }
        return ret;
    }
    // call this after read_header
    void debugCurrentInfo(){
        //MLOGD<<"Input color space is "<<dinfo.jpeg_color_space<<" num components "<<dinfo.num_components<<" data precision "<<dinfo.data_precision;
        //MLOGD<<"h samp factor"<<dinfo.comp_info[0].h_samp_factor<<"v samp factor "<<dinfo.comp_info[0].v_samp_factor;
        //MLOGD<<"min recommended height of scanline buffer. "<<dinfo.rec_outbuf_height;
        //For decompression, the JPEG file's color space is given in jpeg_color_space,
        //and this is transformed to the output color space out_color_space.
        // See jdcolor.c
        //MLOGD<<"jpeg color space is "<<dinfo.jpeg_color_space<<" libjpeg guessed following output colorspace "<<dinfo.out_color_space;
        // unsigned int scale_num, scale_denom : We do not need scaling since the HW composer does this job for us
        //MLOGD<<"boolean do_fancy_upsampling (default true) "<<dinfo.do_fancy_upsampling<<" boolean do_block_smoothing (default true) "<<dinfo.do_block_smoothing;
        //If your interest is merely in bypassing color conversion, we recommend
        //that you use the standard interface and simply set jpeg_color_space =
        //in_color_space (or jpeg_color_space = out_color_space for decompression).
    }
    static void printStartEnd(uint8_t* data,size_t dataSize){
        MLOGD<<" Is "<<(int)data[0]<<" "<<(int)data[1];
        MLOGD<<" Is "<<(int)data[dataSize-2]<<" "<<(int)data[dataSize-1];
    }
public:
    // Supports the most common ANativeWindow_Buffer image formats
    // No unnecessary memcpy's & correctly handle stride of ANativeWindow_Buffer
    void DecodeMJPEGtoANativeWindowBuffer(const void* jpegData, size_t jpegDataSize, const ANativeWindow_Buffer& nativeWindowBuffer){
        ANativeWindowBufferHelper::debugANativeWindowBuffer(nativeWindowBuffer);
        //printStartEnd((uint8_t*)jpegData,jpegDataSize);
        MEASURE_FUNCTION_EXECUTION_TIME
        unsigned int BYTES_PER_PIXEL;
        J_COLOR_SPACE wantedOutputColorspace;
        if(nativeWindowBuffer.format==AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM || nativeWindowBuffer.format==AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM){
            wantedOutputColorspace = JCS_EXT_RGBA;
            BYTES_PER_PIXEL=4;
        }else if(nativeWindowBuffer.format==AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM){
            wantedOutputColorspace = JCS_EXT_RGB;
            BYTES_PER_PIXEL=3;
        }else if(nativeWindowBuffer.format==AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM){
            wantedOutputColorspace = JCS_RGB565;
            BYTES_PER_PIXEL=2;
        //}else if(nativeWindowBuffer.format==AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420){
        //    wantedOutputColorspace = JCS_YCbCr;
        //    BYTES_PER_PIXEL=2;
        }else{
            MLOGD<<"Unsupported image format";
            return;
        }
        setErrorManager();
        jpeg_mem_src(&dinfo,(const unsigned char*) jpegData,jpegDataSize);
        jpeg_read_header(&dinfo, TRUE);
        dinfo.out_color_space=wantedOutputColorspace;
        dinfo.dct_method = JDCT_IFAST;
        jpeg_start_decompress(&dinfo);

        // create the array of pointers that takes stride of nativeWindowBuffer into account
        // Especially when using RGB (24 bit) stride != image height
        const unsigned int SCANLINE_LEN = ((unsigned int)nativeWindowBuffer.stride) * BYTES_PER_PIXEL;
        const auto tmp=convertToPointers((uint8_t*)nativeWindowBuffer.bits, dinfo.output_height, SCANLINE_LEN);
        unsigned int scanline_count = 0;
        while (dinfo.output_scanline < dinfo.output_height){
            JSAMPROW row2= (JSAMPROW)tmp[scanline_count];
            auto lines_read=jpeg_read_scanlines(&dinfo,&row2, 8);
            // unfortunately reads only one line at a time CLOGD("Lines read %d",lines_read);
            scanline_count+=lines_read;
        }
        jpeg_finish_decompress(&dinfo);
    }

    // Decode a jpeg whose color format is YUV422 into the appropriate buffer
    // No colorspace conversion(s) probably means best performance
    void decodeRawYUV422(void* jpegData, size_t jpegDataSize, APixelBuffers::YUV422Planar& out_buff){
        assert(out_buff.WIDTH==640 && out_buff.HEIGHT==480);
        MEASURE_FUNCTION_EXECUTION_TIME
        setErrorManager();
        jpeg_mem_src(&dinfo,(const unsigned char*) jpegData, jpegDataSize);
        jpeg_read_header(&dinfo, TRUE);
        // The jpeg color space is YUV422 planar if all these requirements are fulfilled
        const bool IS_YUV422=
                dinfo.jpeg_color_space==JCS_YCbCr &&
                dinfo.comp_info[0].v_samp_factor==1 &&
                dinfo.comp_info[1].v_samp_factor==1 &&
                dinfo.comp_info[2].v_samp_factor==1 &&
                dinfo.comp_info[0].h_samp_factor==2 &&
                dinfo.comp_info[1].h_samp_factor==1 &&
                dinfo.comp_info[2].h_samp_factor==1;
        if(!IS_YUV422){
            MLOGE<<"Is not YUV422";
            return;
        }
        dinfo.out_color_space = JCS_YCbCr;
        dinfo.dct_method = JDCT_FASTEST;
        dinfo.raw_data_out = TRUE;
        jpeg_start_decompress (&dinfo);

        decodeDirect(out_buff);

        jpeg_finish_decompress(&dinfo);
    }

    void decodeDirect(APixelBuffers::YUV422Planar& out_buf){
        unsigned char **yuv[3];

        auto y2=convertToPointers(&out_buf.Y(0,0),480,640);
        auto u2=convertToPointers(&out_buf.U(0,0),480,320);
        auto v2=convertToPointers(&out_buf.V(0,0),480,320);

        //jpeg_read_raw_data() returns one MCU row per call, and thus you must pass a
        //buffer of at least max_v_samp_factor*DCTSIZE scanlines
        auto max_v_samp_factor=dinfo.comp_info[0].v_samp_factor;
        size_t scanline_count=0;
        for (int i = 0; i < (int) dinfo.image_height; i += max_v_samp_factor * DCTSIZE){
            const auto SOME_SIZE=max_v_samp_factor * DCTSIZE;
            yuv[0] = &y2[scanline_count];
            yuv[1] = &u2[scanline_count];
            yuv[2] = &v2[scanline_count];
            auto lines_read = jpeg_read_raw_data (&dinfo, (JSAMPIMAGE) yuv, SOME_SIZE);
            // lines read is always 8
            //MLOGD<<"lines read "<<lines_read;
            scanline_count+=lines_read;
        }
    }
};

#endif //UVCCAMERA_MJPEGDECODEANDROID_HPP
