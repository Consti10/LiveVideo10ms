

#include <jni.h>
#include <android/native_window_jni.h>

#include <libusb.h>
#include <libuvc.h>
#include <cstring>
#include <thread>
#include <atomic>

#include <NDKThreadHelper.hpp>
#include <AndroidThreadPrioValues.hpp>
#include <NDKArrayHelper.hpp>
#include <GroundRecorderRAW.hpp>
#include <FileHelper.hpp>
#include <TimeHelper.hpp>
#include <SimpleTranscoder.h>
#include <GroundRecorderFPV.hpp>
#include <AColorFormats.hpp>
#include "MJPEGDecodeAndroid.hpp"

static constexpr const auto TAG="UVCReceiverDecoder";

class UVCReceiverDecoder{
private:
    // Window that holds the buffer(s) into which uvc frames will be decoded
    ANativeWindow* aNativeWindow=nullptr;
    // Setting / updating the window happens from 2 different threads.
    // Not 100% sure if needed, but can't hurt
    std::mutex mMutexNativeWindow;
    // Need a static function that calls class instance for the c-style uvc lib
    static void callbackProcessFrame(uvc_frame_t* frame, void* self){
        ((UVCReceiverDecoder *) self)->processFrame(frame);
    }
    uvc_context_t *ctx=nullptr;
    uvc_device_t *dev=nullptr;
    uvc_device_handle_t *devh=nullptr;
    bool isStreaming=false;
    static constexpr unsigned int VIDEO_STREAM_WIDTH=640;
    static constexpr unsigned int VIDEO_STREAM_HEIGHT=480;
    static constexpr unsigned int VIDEO_STREAM_FPS=30;
    int lastUvcFrameSequenceNr=0;
    bool processFramePrioritySet=false;
    JavaVM* javaVm;
    const std::string GROUND_RECORDING_DIRECTORY;
    GroundRecorderFPV groundRecorderFPV;
    MJPEGDecodeAndroid mMJPEGDecodeAndroid;
public:
    UVCReceiverDecoder(JNIEnv* env,std::string GROUND_RECORDING_DIRECTORY2):GROUND_RECORDING_DIRECTORY(std::move(GROUND_RECORDING_DIRECTORY2)),
    groundRecorderFPV(GROUND_RECORDING_DIRECTORY){
        javaVm=nullptr;
        env->GetJavaVM(&javaVm);
    }
    // nullptr: clean up and remove
    // valid surface: acquire the ANativeWindow
    void setSurface(JNIEnv* env,jobject surface){
        std::lock_guard<std::mutex> lock(mMutexNativeWindow);
        if(surface==nullptr){
            ANativeWindow_release(aNativeWindow);
            aNativeWindow=nullptr;
        }else{
            aNativeWindow=ANativeWindow_fromSurface(env,surface);
            const auto WANTED_FORMAT=AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM;
            ANativeWindow_setBuffersGeometry(aNativeWindow,VIDEO_STREAM_WIDTH,VIDEO_STREAM_HEIGHT,WANTED_FORMAT);
            const auto ACTUAL_FORMAT=ANativeWindow_getFormat(aNativeWindow);
            if(ACTUAL_FORMAT!=WANTED_FORMAT){
                MLOGE<<"Actual format is "<<ACTUAL_FORMAT;
            }else{
                MLOGD<<"Set format to "<<ACTUAL_FORMAT;
            }
        }
    }
    // Investigate: Even tough the documentation warns about dropping frames if processing takes too long
    // I cannot experience dropped frames - ?
    // Using less threads (no extra thread for decoding) reduces latency but also negatively affects troughput
    void processFrame(uvc_frame_t* frame_mjpeg){
        MEASURE_FUNCTION_EXECUTION_TIME
        if(!processFramePrioritySet){
            NDKThreadHelper::setProcessThreadPriorityAttachDetach(javaVm,FPV_VR_PRIORITY::CPU_PRIORITY_UVC_FRAME_CALLBACK,TAG);
            processFramePrioritySet=true;
        }
        std::lock_guard<std::mutex> lock(mMutexNativeWindow);
        //CLOGD("Got uvc_frame_t %d  ms: %f",frame_mjpeg->sequence,(frame_mjpeg->capture_time.tv_usec/1000)/1000.0f);
        int deltaFrameSequence=(int)frame_mjpeg->sequence-lastUvcFrameSequenceNr;
        lastUvcFrameSequenceNr=frame_mjpeg->sequence;
        if(deltaFrameSequence!=1){
            MLOGD<<"Probably dropped frame "<<deltaFrameSequence;
        }
        if(aNativeWindow==nullptr){
            MLOGD<<"No surface";
            return;
        }
        ANativeWindow_Buffer buffer;
        if(ANativeWindow_lock(aNativeWindow, &buffer, nullptr)==0){
            //ANativeWindowBufferHelper::debugANativeWindowBuffer(buffer);
            //auto framebuffer=APixelBuffers::YUV420SemiPlanar(buffer.bits, buffer.width, buffer.height);
            //YUVFrameGenerator::generateFrame(framebuffer,0);
            mMJPEGDecodeAndroid.DecodeMJPEGtoANativeWindowBuffer(frame_mjpeg->data, frame_mjpeg->actual_bytes,buffer);
            ANativeWindow_unlockAndPost(aNativeWindow);
        }else{
            MLOGD<<"Cannot lock window";
        }
        groundRecorderFPV.writePacketIfStarted((uint8_t*)frame_mjpeg->data,frame_mjpeg->actual_bytes,GroundRecorderFPV::PACKET_TYPE_MJPEG_ROTG02,frame_mjpeg->sequence);
    }
    // Connect via android java first (workaround ?!)
    // 0 on success, -1 otherwise
    int startReceiving(int vid, int pid, int fd,
                        int busnum,int devAddr,
                        const std::string usbfs){
        uvc_stream_ctrl_t ctrl;
        uvc_error_t res;
        /* Initialize a UVC service context. Libuvc will set up its own libusb
         * context. Replace NULL with a libusb_context pointer to run libuvc
         * from an existing libusb context. */
        res = uvc_init2(&ctx,nullptr,usbfs.c_str());
        if (res < 0) {
            MLOGE<<"Error uvc_init "<<res;
        }
        MLOGD<<"UVC initialized";
        /* Locates the first attached UVC device, stores in dev */
        //res = uvc_find_device(
        //        ctx, &dev,
        //        0, 0, NULL); /* filter devices: vendor_id, product_id, "serial_num" */
        //res = uvc_get_device_with_fd(ctx, &dev, pid, vid, NULL, fd, NULL, NULL);
        res = uvc_get_device_with_fd(ctx, &dev, vid, pid, nullptr, fd, busnum, devAddr);
        if (res < 0) {
            uvc_perror(res, "uvc_find_device"); /* no devices found */
        } else {
            MLOGD<<"Device found";
            /* Try to open the device: requires exclusive access */
            res = uvc_open(dev, &devh);
            if (res < 0) {
                MLOGE<<"Error uvc_open"; /* unable to open device */
            } else {
                MLOGD<<"Device opened";
                //X MJPEG only
                res = uvc_get_stream_ctrl_format_size(
                        devh, &ctrl,
                        UVC_FRAME_FORMAT_MJPEG,
                        VIDEO_STREAM_WIDTH, VIDEO_STREAM_HEIGHT, VIDEO_STREAM_FPS
                );
                /* Print out the result */
                //uvc_print_stream_ctrl(&ctrl, stderr);
                if (res < 0) {
                    uvc_perror(res, "get_mode"); /* device doesn't provide a matching stream */
                } else {
                    processFramePrioritySet=false;
                    res = uvc_start_streaming(devh, &ctrl, UVCReceiverDecoder::callbackProcessFrame, this, 0);
                    if (res < 0) {
                        MLOGE<<"Error start_streaming "<<res; /* unable to start stream */
                    } else {
                        MLOGD<<"Streaming...";
                        //uvc_set_ae_mode(devh, 1); /* e.g., turn on auto exposure */
                        isStreaming=true;
                        groundRecorderFPV.start();
                        return 0;
                    }
                }
                /* Release our handle on the device */
                uvc_close(devh);
            }
            /* Release the device descriptor */
            uvc_unref_device(dev);
        }
        /* Close the UVC context. This closes and cleans up any existing device handles,
         * and it closes the libusb context if one was not provided. */
        uvc_exit(ctx);
        return -1;
    }
    // return the filename of the written file on success
    std::optional<std::string> stopReceiving(JNIEnv* env,jobject androidContext){
        if(isStreaming){
            uvc_stop_streaming(devh);
            uvc_close(devh);
            uvc_unref_device(dev);
            uvc_exit(ctx);
            isStreaming=false;
            return groundRecorderFPV.stop(env,androidContext);
        }
        return std::nullopt;
    }
};

// ------------------------------------- Native Bindings -------------------------------------
#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_uvcintegration_UVCReceiverDecoder_##method_name
extern "C" {

inline jlong jptr(UVCReceiverDecoder *p) {
    return reinterpret_cast<intptr_t>(p);
}
inline UVCReceiverDecoder *native(jlong ptr) {
    return reinterpret_cast<UVCReceiverDecoder*>(ptr);
}

JNI_METHOD(jlong, nativeConstruct)
(JNIEnv *env, jclass jclass1,jstring groundRecordingDir) {
    auto* tmp=new UVCReceiverDecoder(env,NDKArrayHelper::DynamicSizeString(env,groundRecordingDir));
    return jptr(tmp);
}
JNI_METHOD(void, nativeDelete)
(JNIEnv *env, jclass jclass1, jlong p) {
    delete native(p);
}

JNI_METHOD(jint, nativeStartReceiving)
(JNIEnv *env, jclass jclass1,jlong nativeInstance,
 jint vid, jint pid, jint fd,
 jint busnum,jint devAddr,
 jstring usbfs_str
) {
    const std::string usbfs=NDKArrayHelper::DynamicSizeString(env,usbfs_str);
    return native(nativeInstance)->startReceiving(vid,pid,fd,busnum,devAddr,usbfs);
}
JNI_METHOD(jstring , nativeStopReceiving)
(JNIEnv *env, jclass jclass1, jlong p,jobject androidContext) {
   const auto groundRecordingFilenamePath=native(p)->stopReceiving(env,androidContext);
   if(groundRecordingFilenamePath==std::nullopt){
       return nullptr;
   }
   return env->NewStringUTF(groundRecordingFilenamePath->c_str());
}

JNI_METHOD(void, nativeSetSurface)
(JNIEnv *env, jclass jclass1, jlong javaP,jobject surface) {
   native(javaP)->setSurface(env,surface);
}

}
