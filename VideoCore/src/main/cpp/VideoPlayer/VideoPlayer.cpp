
#include "VideoPlayer.h"
#include <AndroidThreadPrioValues.hpp>
#include <NDKThreadHelper.hpp>
#include "../IDV.hpp"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>
#include <FileHelper.hpp>



VideoPlayer::VideoPlayer(JNIEnv* env, jobject context, const char* DIR) :
    mParser{std::bind(&VideoPlayer::onNewNALU, this, std::placeholders::_1)},
    mSettingsN(env,context,"pref_video",true),
    GROUND_RECORDING_DIRECTORY(DIR),
    mGroundRecorderFPV(GROUND_RECORDING_DIRECTORY),
    mFileReceiver(1024){
    env->GetJavaVM(&javaVm);

    const auto filename=FileHelper::findUnusedFilename(GROUND_RECORDING_DIRECTORY,"mp4");
    FFMPEGFileWriter::lol(filename);
}

//Not yet parsed bit stream (e.g. raw h264 or rtp data)
void VideoPlayer::onNewVideoData(const uint8_t* data, const std::size_t data_length,const VIDEO_DATA_TYPE videoDataType){
    //MLOGD("onNewVideoData %d",data_length);
    switch(videoDataType){
        case VIDEO_DATA_TYPE::RAW:
            mParser.parse_raw_h264_stream(data,data_length);
            break;
        case VIDEO_DATA_TYPE::RTP:
            mParser.parse_rtp_h264_stream(data,data_length);
            break;
        case VIDEO_DATA_TYPE::DJI:
            mParser.parseDjiLiveVideoData(data,data_length);
            break;
    }
}

void VideoPlayer::onNewNALU(const NALU& nalu){
    //MLOGD("VideoNative::onNewNALU %d %s",(int)nalu.data_length,nalu.get_nal_name().c_str());
    //nalu.debugX();
    if(mLowLagDecoder){
        mLowLagDecoder->interpretNALU(nalu);
    }
    mGroundRecorderFPV.writePacketIfStarted(nalu.getData(),nalu.getSize(),GroundRecorderFPV::PACKET_TYPE_VIDEO_H264);
}


void VideoPlayer::addConsumers(JNIEnv* env, jobject surface) {
    //reset the parser so the statistics start again from 0
    mParser.reset();
    //set the jni object for settings
    mSettingsN.replaceJNI(env);
    //add decoder if surface!=nullptr
    const bool VS_USE_SW_DECODER=mSettingsN.getBoolean(IDV::VS_USE_SW_DECODER);
    if(surface!= nullptr){
        window=ANativeWindow_fromSurface(env,surface);
        mLowLagDecoder=std::make_unique<LowLagDecoder>(javaVm,window,VS_USE_SW_DECODER);
        mLowLagDecoder->registerOnDecoderRatioChangedCallback([this](const VideoRatio ratio) {
            const bool changed=!(ratio==this->latestVideoRatio);
            this->latestVideoRatio=ratio;
            latestVideoRatioChanged=changed;
        });
        mLowLagDecoder->registerOnDecodingInfoChangedCallback([this](const DecodingInfo info) {
            const bool changed=!(info==this->latestDecodingInfo);
            this->latestDecodingInfo=info;
            latestDecodingInfoChanged=changed;
        });
    }
    //Add Ground recorder if enabled
    const bool VS_GroundRecording=mSettingsN.getBoolean(IDV::VS_GROUND_RECORDING);
    const auto VS_SOURCE= static_cast<SOURCE_TYPE_OPTIONS>(mSettingsN.getInt(IDV::VS_SOURCE));
    if(VS_GroundRecording && VS_SOURCE!=FILE && VS_SOURCE != ASSETS){
    //if(true){
         mGroundRecorderFPV.start();
    }
}

void VideoPlayer::removeConsumers(){
    if(mLowLagDecoder){
        mLowLagDecoder->waitForShutdownAndDelete();
        mLowLagDecoder.reset();
    }
    mGroundRecorderFPV.stop();
    if(window!=nullptr){
        //Don't forget to release the window, does not matter if the decoder has been created or not
        ANativeWindow_release(window);
        window=nullptr;
    }
}

void VideoPlayer::startReceiver(JNIEnv *env, AAssetManager *assetManager) {
    mSettingsN.replaceJNI(env);
    mParser.setLimitFPS(-1); //Default: Real time !
    const auto VS_SOURCE= static_cast<SOURCE_TYPE_OPTIONS>(mSettingsN.getInt(IDV::VS_SOURCE));
    const int VS_FILE_ONLY_LIMIT_FPS=mSettingsN.getInt(IDV::VS_FILE_ONLY_LIMIT_FPS,60);

    //TODO use url instead of all these settings
    //If url starts with udp: raw h264 over udp
    //If url starts with rtp: rtp h264 over udp
    //If url starts with asset: play file from android assets folder
    //If url starts with file: play file from android local storage
    //If url starts with rtsp: play rtsp via ffmpeg

    //MLOGD("VS_SOURCE: %d",VS_SOURCE);
    switch (VS_SOURCE){
        case UDP:{
            const int VS_PORT=mSettingsN.getInt(IDV::VS_PORT);
            const bool useRTP= mSettingsN.getInt(IDV::VS_PROTOCOL) ==0;
            mUDPReceiver=std::make_unique<UDPReceiver>(javaVm,VS_PORT, "VideoPlayer VideoReceiver", FPV_VR_PRIORITY::CPU_PRIORITY_UDPRECEIVER_VIDEO, [this,useRTP](const uint8_t* data, size_t data_length) {
                onNewVideoData(data,data_length,useRTP ? VIDEO_DATA_TYPE::RTP : VIDEO_DATA_TYPE::RAW);
            }, WANTED_UDP_RCVBUF_SIZE);
            mUDPReceiver->startReceiving();
        }break;
        case FILE:
        case ASSETS: {
            const bool useAsset=VS_SOURCE==ASSETS;
            const std::string filename = useAsset ?  mSettingsN.getString(IDV::VS_ASSETS_FILENAME_TEST_ONLY,"testVideo.h264") :
                    mSettingsN.getString(IDV::VS_PLAYBACK_FILENAME);
            if(!FileHelper::endsWith(filename, ".fpv")){
                mParser.setLimitFPS(VS_FILE_ONLY_LIMIT_FPS);
            }
            const auto cb=[this](const uint8_t *data, size_t data_length,GroundRecorderFPV::PACKET_TYPE packetType) {
                if (packetType == GroundRecorderFPV::PACKET_TYPE_VIDEO_H264) {
                    onNewVideoData(data, data_length,VIDEO_DATA_TYPE::RAW);
                }
            };
            mFileReceiver.setCallBack(0,cb);
            mFileReceiver.startReading(useAsset ? assetManager : nullptr,filename);
        }
        break;
        case VIA_FFMPEG_URL:{
            MLOGD<<"Started with SOURCE=TEST360";
            const std::string url=mSettingsN.getString(IDV::VS_FFMPEG_URL);
            //const std::string url="file:/storage/emulated/0/DCIM/FPV_VR/capture1.h264";
            //const std::string url="file:/storage/emulated/0/DCIM/FPV_VR/360_test.h264";
            //const std::string url="file:/storage/emulated/0/DCIM/FPV_VR/360.h264";
            //const std::string url="file:/storage/emulated/0/DCIM/FPV_VR/test.mp4";
            //MLOGD("url:%s",url.c_str());
            mFFMpegVideoReceiver=std::make_unique<FFMpegVideoReceiver>(url,0,[this](uint8_t* data,int data_length) {
                onNewVideoData(data,(size_t)data_length,VIDEO_DATA_TYPE::RAW);
            },[this](const NALU& nalu) {
                onNewNALU(nalu);
            });
            mFFMpegVideoReceiver->start_playing();
        }break;
        case EXTERNAL:{
            //Data is being received somewhere else and passed trough-init nothing.
            MLOGD<<"Started with SOURCE=EXTERNAL";
        }break;
        default:
            MLOGD<<"Error unknown VS_SOURCE";
            break;
    }
}

void VideoPlayer::stopReceiver() {
    if(mUDPReceiver){
        mUDPReceiver->stopReceiving();
        mUDPReceiver.reset();
    }
    mFileReceiver.stopReadingIfStarted();
    if(mFFMpegVideoReceiver){
        mFFMpegVideoReceiver->shutdown_callback();
        mFFMpegVideoReceiver->stop_playing();
        mFFMpegVideoReceiver.reset();
    }
}

std::string VideoPlayer::getInfoString(){
    std::ostringstream ostringstream1;
    if(mUDPReceiver){
        ostringstream1 << "Listening for video on port " << mUDPReceiver->getPort();
        ostringstream1 << "\nReceived: " << mUDPReceiver->getNReceivedBytes() << "B"
                       << " | parsed frames: "
                       << mParser.nParsedNALUs << " | key frames: " << mParser.nParsedKeyFrames;
    }else{
        ostringstream1<<"Not receiving from udp";
    }
    return ostringstream1.str();
}

//----------------------------------------------------JAVA bindings---------------------------------------------------------------
#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_video_core_video_1player_VideoPlayer_##method_name

inline jlong jptr(VideoPlayer *videoPlayerN) {
    return reinterpret_cast<intptr_t>(videoPlayerN);
}
inline VideoPlayer *native(jlong ptr) {
    return reinterpret_cast<VideoPlayer *>(ptr);
}


extern "C"{

JNI_METHOD(jlong, nativeInitialize)
(JNIEnv * env,jclass jclass1,jobject context,jstring groundRecordingDirectory) {
    const char *str = env->GetStringUTFChars(groundRecordingDirectory, nullptr);
    auto* p=new VideoPlayer(env, context, str);
    env->ReleaseStringUTFChars(groundRecordingDirectory,str);
    return jptr(p);
}

JNI_METHOD(void, nativeFinalize)
(JNIEnv * env, jclass jclass1,jlong videoPlayerN) {
    VideoPlayer* p=native(videoPlayerN);
    delete (p);
}

JNI_METHOD(void, nativeStart)
(JNIEnv * env, jclass jclass1,jlong videoPlayerN,jobject surface,jobject assetManager){
    AAssetManager* mgr=AAssetManager_fromJava(env,assetManager);
    native(videoPlayerN)->addConsumers(env,surface);
    native(videoPlayerN)->startReceiver(env, mgr);
}

JNI_METHOD(void, nativeStop)
(JNIEnv * env,jclass jclass1,jlong videoPlayerN){
    native(videoPlayerN)->removeConsumers();
    native(videoPlayerN)->stopReceiver();
}

//This function is only called when the data for the video is coming from a file.
//In this case, the receiving is done via JAVA, and the received bytes are transfered from JAVA to NDK
JNI_METHOD(void, nativePassNALUData)
(JNIEnv *env,jclass jclass1,jlong videoPlayerN,jbyteArray b,jint offset,jint length){
    jbyte *arrayP=env->GetByteArrayElements(b, nullptr);
    auto * p=(uint8_t*)arrayP;
    native(videoPlayerN)->onNewVideoData(&p[(int) offset], (size_t)length,VideoPlayer::VIDEO_DATA_TYPE::DJI);
    env->ReleaseByteArrayElements(b,arrayP,0);
}

JNI_METHOD(jstring , getVideoInfoString)
(JNIEnv *env,jclass jclass1,jlong testReceiverN) {
    VideoPlayer* p=native(testReceiverN);
    jstring ret = env->NewStringUTF(p->getInfoString().c_str());
    return ret;
}

JNI_METHOD(jboolean , anyVideoDataReceived)
(JNIEnv *env,jclass jclass1,jlong testReceiverN) {
    VideoPlayer* p=native(testReceiverN);
    if(p->mUDPReceiver){
        return (jboolean) false;
    }
    bool ret = (p->mUDPReceiver->getNReceivedBytes() > 0);
    return (jboolean) ret;
}

JNI_METHOD(jboolean , receivingVideoButCannotParse)
(JNIEnv *env,jclass jclass1,jlong testReceiverN) {
    VideoPlayer* p=native(testReceiverN);
    if(p->mUDPReceiver){
        return (jboolean) (p->mUDPReceiver->getNReceivedBytes() > 1024 * 1024 && p->mParser.nParsedNALUs == 0);
    }
    return (jboolean) false;
}

JNI_METHOD(jboolean , anyVideoBytesParsedSinceLastCall)
(JNIEnv *env,jclass jclass1,jlong testReceiverN) {
    VideoPlayer* p=native(testReceiverN);
    long nalusSinceLast = p->mParser.nParsedNALUs - p->nNALUsAtLastCall;
    p->nNALUsAtLastCall += nalusSinceLast;
    return (jboolean) (nalusSinceLast > 0);
}

JNI_METHOD(jlong , nativeGetExternalGroundRecorder)
(JNIEnv *env,jclass jclass1,jlong instance) {
    VideoPlayer* p=native(instance);
    return (jlong) &p->mGroundRecorderFPV;
}
JNI_METHOD(jlong , nativeGetExternalFileReader)
(JNIEnv *env,jclass jclass1,jlong instance) {
    VideoPlayer* p=native(instance);
    return (jlong) &p->mFileReceiver;
}

JNI_METHOD(void,nativeCallBack)
(JNIEnv *env,jclass jclass1,jobject videoParamsChangedI,jlong testReceiverN){
    VideoPlayer* p=native(testReceiverN);
    //Update all java stuff
    if(p->latestDecodingInfoChanged || p->latestVideoRatioChanged){
        jclass jClassExtendsIVideoParamsChanged= env->GetObjectClass(videoParamsChangedI);
        if(p->latestVideoRatioChanged){
            jmethodID onVideoRatioChangedJAVA = env->GetMethodID(jClassExtendsIVideoParamsChanged, "onVideoRatioChanged", "(II)V");
            env->CallVoidMethod(videoParamsChangedI,onVideoRatioChangedJAVA,(jint)p->latestVideoRatio.width,(jint)p->latestVideoRatio.height);
            p->latestVideoRatioChanged=false;
        }
        if(p->latestDecodingInfoChanged){
            jmethodID onDecodingInfoChangedJAVA = env->GetMethodID(jClassExtendsIVideoParamsChanged, "onDecodingInfoChanged", "(FFFFFII)V");
            const auto info=p->latestDecodingInfo;
            env->CallVoidMethod(videoParamsChangedI,onDecodingInfoChangedJAVA,(jfloat)info.currentFPS,(jfloat)info.currentKiloBitsPerSecond,
                                (jfloat)info.avgParsingTime_ms,(jfloat)info.avgWaitForInputBTime_ms,(jfloat)info.avgDecodingTime_ms,(jint)info.nNALU,(jint)info.nNALUSFeeded);
            p->latestDecodingInfoChanged=false;
        }
    }
}

}
