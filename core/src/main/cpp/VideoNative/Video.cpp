/*************************************************************************
 * Glue for the Video Java class
 * ***********************************************************************/
#include "Video.h"
#include "../General/CPUPriority.hpp"
#include <android/native_window.h>
#include <android/native_window_jni.h>

#define TAG "Video"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

#define CPU_PRIORITY_UDPRECEIVER_VIDEO (-16)  //needs low latency and does not use the cpu that much
#define CPU_PRIORITY_DECODER_OUTPUT (-16)     //needs low latency and does not use the cpu that much



Video::Video(JNIEnv* env, jobject obj) :
    mParser{std::bind(&Video::onNewNALU, this, std::placeholders::_1)}{
    //We need a reference to the JavaVM to attach the callback thread to it
    env->GetJavaVM(&callToJava.javaVirtualMachine);
    //Get the class that implements the IVideoParamsChanged, so that we can
    //get the function pointers to it's 2 callback functions
    jclass jClassExtendsIVideoParamsChanged= env->GetObjectClass(obj);
    callToJava.onVideoRatioChangedJAVA = env->GetMethodID(jClassExtendsIVideoParamsChanged, "onVideoRatioChanged", "(II)V");
    callToJava.onDecoderFPSChangedJAVA = env->GetMethodID(jClassExtendsIVideoParamsChanged, "onDecodingInfoChanged", "(FFFFFII)V");
    //Store a global reference to it, so we can use it later
    //callToJava.globalJavaObj = env->NewGlobalRef(videoParamsChangedI); //also need a global javaObj
    callToJava.globalJavaObj = env->NewWeakGlobalRef(obj);
}

void Video::onNewNALU(const NALU& nalu){
    if(mLowLagDecoder!=nullptr){
        mLowLagDecoder->interpretNALU(nalu);
    }
    if(mGroundRecorder!= nullptr){
        mGroundRecorder->writeData(nalu.data,nalu.data_length);
    }
}

void Video::onDecoderRatioChangedCallback(const int videoW,const int videoH) {
    //When this callback is invoked, no Java VM is attached to the thread
    JNIEnv* jniENV;
    callToJava.javaVirtualMachine->AttachCurrentThread(&jniENV, nullptr);
    jniENV->CallVoidMethod(callToJava.globalJavaObj,callToJava.onVideoRatioChangedJAVA,(jint)videoW,(jint)videoH);
    callToJava.javaVirtualMachine->DetachCurrentThread();
}

void Video::onDecodingInfoChangedCallback(const LowLagDecoder::DecodingInfo& info) {
    //When this callback is invoked, no Java VM is attached to the thread
    JNIEnv* jniENV;
    callToJava.javaVirtualMachine->AttachCurrentThread(&jniENV, nullptr);
    jniENV->CallVoidMethod(callToJava.globalJavaObj,callToJava.onDecoderFPSChangedJAVA,(jfloat)info.currentFPS,(jfloat)info.currentKiloBitsPerSecond,
                           (jfloat)info.avgParsingTime_ms,(jfloat)info.avgWaitForInputBTime_ms,(jfloat)info.avgDecodingTime_ms,(jint)info.nNALU,(jint)info.nNALUSFeeded);
    callToJava.javaVirtualMachine->DetachCurrentThread();
}

void Video::onNewVideoData(const uint8_t* data,const int data_length,const bool isRTPData,const bool limitFPS=false){
    mParser.limitFPS=limitFPS;
    if(isRTPData){
        mParser.parse_rtp_h264_stream(data,data_length);
    }else{
        mParser.parse_raw_h264_stream(data,data_length);
    }
}

void Video::addConsumers(JNIEnv* env,jobject surface,jstring groundRecordingFileName) {
    if(surface!= nullptr){
        window=ANativeWindow_fromSurface(env,surface);
        mLowLagDecoder=new LowLagDecoder(window,CPU_PRIORITY_DECODER_OUTPUT);
        mLowLagDecoder->registerOnDecoderRatioChangedCallback([this](int w,int h) { this->onDecoderRatioChangedCallback(w,h); });
        mLowLagDecoder->registerOnDecodingInfoChangedCallback([this](LowLagDecoder::DecodingInfo& info) {
            this->onDecodingInfoChangedCallback(info);
        });
    }
    if(groundRecordingFileName!=nullptr){
         const char *str = env->GetStringUTFChars(groundRecordingFileName, 0);
         mGroundRecorder=new GroundRecorder(str);
          env->ReleaseStringUTFChars(groundRecordingFileName,str);
    }
    mParser.reset();
}

void Video::removeConsumers(){
    if(mLowLagDecoder!= nullptr){
        mLowLagDecoder->waitForShutdownAndDelete();
        delete(mLowLagDecoder);
        mLowLagDecoder= nullptr;
    }
    if(mGroundRecorder!= nullptr){
        mGroundRecorder->stop();
        delete(mGroundRecorder);
        mGroundRecorder=nullptr;
    }
    if(window!=nullptr){
        //Don't forget to release the window, does not matter if the decoder has been created or not
        ANativeWindow_release(window);
        window=nullptr;
    }
}


void Video::startNativeUDPReceiver(const int port,const bool useRTP) {
    mVideoReceiver=new UDPReceiver(port,"VideoPlayer VideoReceiver",CPU_PRIORITY_UDPRECEIVER_VIDEO,1024*8,[this,useRTP](uint8_t* data,int data_length) {
        if(useRTP){
            mParser.parse_rtp_h264_stream(data,data_length);
        }else{
            mParser.parse_raw_h264_stream(data,data_length);
        }
    });
    mVideoReceiver->startReceiving();
}

void Video::stopNativeUDPReceiver() {
    mVideoReceiver->stopReceiving();
    delete(mVideoReceiver);
    mVideoReceiver= nullptr;
}

std::string Video::getInfoString(){
    std::ostringstream ostringstream1;
    ostringstream1 << "Listening for video on port " << mVideoReceiver->getPort();
    ostringstream1 << "\nReceived: " << mVideoReceiver->getNReceivedBytes() << "B"
                   << " | parsed frames: "
                   << mParser.nParsedNALUs<<" | key frames: "<<mParser.nParsedKeyFrames;
    return ostringstream1.str();
}


//----------------------------------------------------JAVA bindings---------------------------------------------------------------
#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_lowlagvideo_VideoNative_Video_##method_name

inline jlong jptr(Video *videoPlayerN) {
    return reinterpret_cast<intptr_t>(videoPlayerN);
}
inline Video *native(jlong ptr) {
    return reinterpret_cast<Video *>(ptr);
}


extern "C"{

JNI_METHOD(jlong, initialize)
(JNIEnv * env,jclass jclass1,jobject obj) {
    return jptr(new Video(env,obj));
}

JNI_METHOD(void, finalize)
(JNIEnv * env, jclass jclass1,jlong videoPlayerN) {
    Video* p=native(videoPlayerN);
    delete (p);
}

JNI_METHOD(void, nativeAddConsumers)
(JNIEnv * env, jclass unused,jlong videoPlayerN,jobject surface,jstring groundRecordingFileName) {
    native(videoPlayerN)->addConsumers(env,surface,groundRecordingFileName);
}

JNI_METHOD(void, nativeRemoveConsumers)
(JNIEnv *env,jclass jclass1, jlong videoPlayerN){
    native(videoPlayerN)->removeConsumers();
}

//This function is only called when the data for the video is coming from a file.
//In this case, the receiving is done via JAVA, and the received bytes are transfered from JAVA to NDK
JNI_METHOD(void, nativePassNALUData)
(JNIEnv *env,jclass jclass1,jlong videoPlayerN,jbyteArray b,jint offset,jint length){
    jbyte *arrayP=env->GetByteArrayElements(b, nullptr);
    auto * p=(uint8_t*)arrayP;
    native(videoPlayerN)->onNewVideoData(&p[(int) offset], length,false,true);
    env->ReleaseByteArrayElements(b,arrayP,0);
}

JNI_METHOD(void, nativeStartUDPReceiver)
(JNIEnv * env, jclass jclass1,jlong videoPlayerN,jint port,jboolean jUseRTP){
    native(videoPlayerN)->startNativeUDPReceiver((int)port,(bool) jUseRTP);
}

JNI_METHOD(void, nativeStopUDPReceiver)
(JNIEnv * env,jclass jclass1,jlong videoPlayerN){
    native(videoPlayerN)->stopNativeUDPReceiver();
}

JNI_METHOD(jstring , getVideoInfoString)
(JNIEnv *env,jclass jclass1,jlong testReceiverN) {
    Video* p=native(testReceiverN);
    jstring ret = env->NewStringUTF(p->getInfoString().c_str());
    return ret;
}

JNI_METHOD(jboolean , anyVideoDataReceived)
(JNIEnv *env,jclass jclass1,jlong testReceiverN) {
    Video* p=native(testReceiverN);
    bool ret = (p->mVideoReceiver->getNReceivedBytes()  > 0);
    return (jboolean) ret;
}

JNI_METHOD(jboolean , receivingVideoButCannotParse)
(JNIEnv *env,jclass jclass1,jlong testReceiverN) {
    Video* p=native(testReceiverN);
    return (jboolean) (p->mVideoReceiver->getNReceivedBytes() > 1024 * 1024 && p->mParser.nParsedNALUs == 0);
}


JNI_METHOD(jboolean , anyVideoBytesParsedSinceLastCall)
(JNIEnv *env,jclass jclass1,jlong testReceiverN) {
    Video* p=native(testReceiverN);
    long nalusSinceLast = p->mParser.nParsedNALUs - p->nNALUsAtLastCall;
    p->nNALUsAtLastCall += nalusSinceLast;
    return (jboolean) (nalusSinceLast > 0);
}

}
