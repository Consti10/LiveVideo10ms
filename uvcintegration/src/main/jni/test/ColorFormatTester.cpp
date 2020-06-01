//
// Created by geier on 31/05/2020.
//
#include <jni.h>
#include <android/native_window_jni.h>
#include <android/surface_texture.h>
#include <android/surface_texture_jni.h>

#include <NDKThreadHelper.hpp>
#include <AndroidThreadPrioValues.hpp>
#include <NDKArrayHelper.hpp>
#include <GroundRecorderRAW.hpp>
#include <FileHelper.hpp>
#include <TimeHelper.hpp>
#include <SimpleTranscoder.h>
#include <GroundRecorderFPV.hpp>
#include <AColorFormats.hpp>
#include <APixelBuffers.hpp>

class ColorFormatTester{
private:
    const size_t WIDTH=640;
    const size_t HEIGHT=480;
    const int FORMAT=AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420;
public:
    ANativeWindow* aNativeWindow=nullptr;
    void setSurface(JNIEnv* env,jobject surface){
        if(surface==nullptr){
            ANativeWindow_release(aNativeWindow);
            aNativeWindow=nullptr;
        }else{
            aNativeWindow=ANativeWindow_fromSurface(env,surface);
            setBuffersGeometry();
        }
    }
    void setBuffersGeometry(){
        auto ret=ANativeWindow_setBuffersGeometry(aNativeWindow,WIDTH,HEIGHT,FORMAT);
        MLOGD<<"ANativeWindow_setBuffersGeometry returned "<<ret;
        const auto actualFormat=ANativeWindow_getFormat(aNativeWindow);
        if(actualFormat!=FORMAT){
            MLOGE<<"Actual format is "<<actualFormat;
        }else{
            MLOGD<<"Set format to "<<actualFormat;
        }
    }
    int frameIndex=0;
    void testUpdateSurface(){
        MEASURE_FUNCTION_EXECUTION_TIME
        ANativeWindow_Buffer buffer;
        //ARect bounds = {0, 0, WIDTH, HEIGHT};
        if(ANativeWindow_lock(aNativeWindow, &buffer, nullptr)==0){
            ANativeWindowBufferHelper::debugANativeWindowBuffer(buffer);
            if(FORMAT==AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM){
                auto framebuffer=APixelBuffers::RGB(buffer.bits,buffer.width,buffer.height,buffer.stride);
                framebuffer.drawPattern(frameIndex);
            }else if(FORMAT==AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420){
                auto framebuffer=APixelBuffers::YUV420SemiPlanar(buffer.bits, buffer.width, buffer.height);
                YUVFrameGenerator::generateFrame(framebuffer,frameIndex);
            }
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