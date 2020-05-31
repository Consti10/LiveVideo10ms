//
// Created by geier on 31/05/2020.
//
#include <jni.h>
#include <android/native_window_jni.h>

#include <NDKThreadHelper.hpp>
#include <AndroidThreadPrioValues.hpp>
#include <NDKArrayHelper.hpp>
#include <GroundRecorderRAW.hpp>
#include <FileHelper.hpp>
#include <TimeHelper.hpp>
#include <SimpleEncoder.h>
#include <GroundRecorderFPV.hpp>
#include <AndroidColorFormats.hpp>

class ColorFormatTester{
private:
    static constexpr size_t WIDTH=640;
    static constexpr size_t HEIGHT=480;
public:
    ANativeWindow* aNativeWindow=nullptr;
    void setSurface(JNIEnv* env,jobject surface){
        if(surface==nullptr){
            ANativeWindow_release(aNativeWindow);
            aNativeWindow=nullptr;
        }else{
            aNativeWindow=ANativeWindow_fromSurface(env,surface);
            const auto WANTED_FORMAT=ImageFormat::YUV_420_888;
            ANativeWindow_setBuffersGeometry(aNativeWindow,WIDTH,HEIGHT,WANTED_FORMAT);
            const auto ACTUAL_FORMAT=ANativeWindow_getFormat(aNativeWindow);
            if(ACTUAL_FORMAT!=WANTED_FORMAT){
                MLOGE<<"Actual format is "<<ACTUAL_FORMAT;
            }else{
                MLOGD<<"Set format to "<<ACTUAL_FORMAT;
            }
        }
    }
    int frameIndex=0;
    void testUpdateSurface(){
        MEASURE_FUNCTION_EXECUTION_TIME
        ANativeWindow_Buffer buffer;
        if(ANativeWindow_lock(aNativeWindow, &buffer, nullptr)==0){
            MJPEGDecodeAndroid::debugANativeWindowBuffer(buffer);
            auto framebuffer=MyColorSpaces::YUV420SemiPlanar(buffer.bits,buffer.width,buffer.height);
            //framebuffer.clear(120,160,200);
            YUVFrameGenerator::generateFrame(framebuffer,frameIndex);
            //YUVFrameGenerator::generateFrame(frameIndex,MediaCodecInfo::CodecCapabilities::COLOR_FormatYUV420Planar,(uint8_t*)buffer.bits,buffer.width*buffer.height*12 / 8);

            //auto framebuffer=MyColorSpaces::RGB(buffer.bits,buffer.width,buffer.height,buffer.stride);
            //framebuffer.drawPattern(frameIndex);
            ANativeWindow_unlockAndPost(aNativeWindow);
            frameIndex++;
        }else{
            MLOGE<<"Cannot lock window";
        }
    }
};
static ColorFormatTester tester;

// ------------------------------------- Native Bindings -------------------------------------
#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_test_ColorFormatTester_##method_name
extern "C" {

JNI_METHOD(void, nativeSetSurface)
(JNIEnv *env, jclass jclass1, jobject surface) {
    tester.setSurface(env,surface);
}

JNI_METHOD(void, nativeTestUpdateSurface)
(JNIEnv *env, jclass jclass1) {
    tester.testUpdateSurface();
}


}