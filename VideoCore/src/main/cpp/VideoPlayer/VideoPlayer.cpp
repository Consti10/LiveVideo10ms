
#include "VideoPlayer.h"
#include <AndroidThreadPrioValues.hpp>
#include <NDKThreadHelper.hpp>
#include "../IDV.hpp"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>
#include <FileHelper.hpp>
#include <NDKHelper.hpp>

//TEST
//#include <NdkImage.h>
//#include <NdkImageReader.h>

VideoPlayer::VideoPlayer(JNIEnv* env, jobject context, const char* DIR) :
        mLowLagDecoder(env),
    mParser{std::bind(&VideoPlayer::onNewNALU, this, std::placeholders::_1)},
    mSettingsN(env,context,"pref_video",true),
    GROUND_RECORDING_DIRECTORY(DIR),
    mGroundRecorderFPV(GROUND_RECORDING_DIRECTORY),
    mFileReceiver(1024){
    env->GetJavaVM(&javaVm);
    //
    mLowLagDecoder.registerOnDecoderRatioChangedCallback([this](const VideoRatio ratio) {
        const bool changed=ratio!=this->latestVideoRatio;
        this->latestVideoRatio=ratio;
        latestVideoRatioChanged=changed;
    });
    mLowLagDecoder.registerOnDecodingInfoChangedCallback([this](const DecodingInfo info) {
        const bool changed=info!=this->latestDecodingInfo;
        this->latestDecodingInfo=info;
        latestDecodingInfoChanged=changed;
    });
}

//Not yet parsed bit stream (e.g. raw h264 or rtp data)
void VideoPlayer::onNewVideoData(const uint8_t* data, const std::size_t data_length,const VIDEO_DATA_TYPE videoDataType){
    //MLOGD("onNewVideoData %d",data_length);
    switch(videoDataType){
        case VIDEO_DATA_TYPE::RTP:
            mParser.parse_rtp_h264_stream(data,data_length);
            break;
        case VIDEO_DATA_TYPE::RAW:
            mParser.parse_raw_h264_stream(data,data_length);
            break;
        case VIDEO_DATA_TYPE::CUSTOM:
            mParser.parseCustom(data,data_length);
            break;
        case VIDEO_DATA_TYPE::CUSTOM2:
            mParser.parseCustomRTPinsideFEC(data, data_length);
            break;
        case VIDEO_DATA_TYPE::DJI:
            mParser.parseDjiLiveVideoData(data,data_length);
            break;
    }
}

void VideoPlayer::onNewNALU(const NALU& nalu){
    //MLOGD("VideoNative::onNewNALU %d %s",(int)nalu.data_length,nalu.get_nal_name().c_str());
    //nalu.debugX();
    //mTestEncodeDecodeRTP.testEncodeDecodeRTP(nalu);
    mLowLagDecoder.interpretNALU(nalu);
    mGroundRecorderFPV.writePacketIfStarted(nalu.getData(),nalu.getSize(),GroundRecorderFPV::PACKET_TYPE_VIDEO_H264);
}


void VideoPlayer::setVideoSurface(JNIEnv *env, jobject surface) {
    //reset the parser so the statistics start again from 0
    mParser.reset();
    //set the jni object for settings
    mSettingsN.replaceJNI(env);
    if(surface!=nullptr){
        mLowLagDecoder.setOutputSurface(env,surface,mSettingsN);
        MLOGD<<"Start with surface";
    }else{
        mLowLagDecoder.setOutputSurface(env, nullptr,mSettingsN);
        MLOGD<<"Set surface to null";
    }
}


void VideoPlayer::start(JNIEnv *env,jobject androidContext) {
    AAssetManager *assetManager=NDKHelper::getAssetManagerFromContext2(env,androidContext);
    mSettingsN.replaceJNI(env);
    mParser.setLimitFPS(-1); //Default: Real time !
    const auto VS_SOURCE= static_cast<SOURCE_TYPE_OPTIONS>(mSettingsN.getInt(IDV::VS_SOURCE));
    const int VS_FILE_ONLY_LIMIT_FPS=mSettingsN.getInt(IDV::VS_FILE_ONLY_LIMIT_FPS,60);
    const bool VS_GroundRecording=mSettingsN.getBoolean(IDV::VS_GROUND_RECORDING);

    //Add Ground recorder if enabled and needed
    if(VS_GroundRecording && VS_SOURCE!=FILE && VS_SOURCE != ASSETS){
    //if(true){
        mGroundRecorderFPV.start();
    }

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
            const int VS_PROTOCOL= mSettingsN.getInt(IDV::VS_PROTOCOL);
            const auto videoDataType=static_cast<VIDEO_DATA_TYPE>(VS_PROTOCOL);
            mUDPReceiver=std::make_unique<UDPReceiver>(javaVm,VS_PORT, "V_UDP_R", FPV_VR_PRIORITY::CPU_PRIORITY_UDPRECEIVER_VIDEO, [this,videoDataType](const uint8_t* data, size_t data_length) {
                onNewVideoData(data,data_length,videoDataType);
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
            MLOGD<<"Started with SOURCE=RTSP(FFMPEG)";
            const std::string url=mSettingsN.getString(IDV::VS_FFMPEG_URL);
            //const std::string url="file:/storage/emulated/0/DCIM/FPV_VR/capture1.h264";
            //const std::string url="file:/storage/emulated/0/DCIM/FPV_VR/360_test.h264";
            //const std::string url="file:/storage/emulated/0/DCIM/FPV_VR/360.h264";
            //const std::string url="file:/storage/emulated/0/DCIM/FPV_VR/test.mp4";
            //const std::string url="https://www.radiantmediaplayer.com/media/big-buck-bunny-360p.mp4";
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

void VideoPlayer::stop(JNIEnv *env,jobject androidContext) {
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
    mGroundRecorderFPV.stop(env,androidContext);
}

std::string VideoPlayer::getInfoString()const{
    std::stringstream ss;
    if(mUDPReceiver){
        ss << "Listening for video on port " << mUDPReceiver->getPort();
        ss << "\nReceived: " << mUDPReceiver->getNReceivedBytes() << "B"
           << " | parsed frames: "
           << mParser.nParsedNALUs << " | key frames: " << mParser.nParsedKeyFrames;
    }else if(mFFMpegVideoReceiver){
        ss << "Connecting to "<<mFFMpegVideoReceiver->m_url;
        ss << "\n"<<mFFMpegVideoReceiver->currentErrorMessage;
        ss <<"\nReceived: "<<mFFMpegVideoReceiver->currentlyReceivedVideoData<<" B"
                << " | parsed frames: "
                << mParser.nParsedNALUs << " | key frames: " << mParser.nParsedKeyFrames;
    }else{
        ss << "Not receiving udp raw / rtp / rtsp";
    }
    return ss.str();
}

static void fillBufferWithRandomData(std::vector<uint8_t>& data){
    const std::size_t size=data.size();
    for(std::size_t i=0;i<size;i++){
        data[i] = rand() % 255;
    }
}

// Create a buffer filled with random data of size sizeByes
std::vector<uint8_t> createRandomDataBuffer(const ssize_t sizeBytes){
  std::vector<uint8_t> buf(sizeBytes);
  for (uint32_t j = 0; j < sizeBytes; j++) {
    buf[j] = rand() % 255;
  }
  return buf;
}

struct PacketInfoData{
    uint32_t seqNr;
    std::chrono::steady_clock::time_point timestamp;
} __attribute__ ((packed));
static_assert(sizeof(PacketInfoData)==4+8);

uint32_t currentSequenceNumber=0;

void writeSequenceNumberAndTimestamp(std::vector<uint8_t>& data){
    assert(data.size()>=sizeof(PacketInfoData));
    PacketInfoData* packetInfoData=(PacketInfoData*)data.data();
    packetInfoData->seqNr=currentSequenceNumber;
    packetInfoData->timestamp= std::chrono::steady_clock::now();
}

PacketInfoData getSequenceNumberAndTimestamp(const std::vector<uint8_t>& data){
    assert(data.size()>=sizeof(PacketInfoData));
    PacketInfoData packetInfoData;
    memcpy(&packetInfoData,data.data(),sizeof(PacketInfoData));
    return packetInfoData;
}

AvgCalculator avgUDPProcessingTime;

static void validateReceivedData(const uint8_t* dataP,size_t data_length){
    auto data=std::vector<uint8_t>(dataP,dataP+data_length);
    const auto info=getSequenceNumberAndTimestamp(data);
    const auto latency=std::chrono::steady_clock::now()-info.timestamp;
    MLOGD<<"XGot data"<<data_length<<" "<<info.seqNr<<" "<<MyTimeHelper::R(latency);
    avgUDPProcessingTime.add(latency);
}

static void generateDataPackets(std::function<void(std::vector<uint8_t>&)> cb,const int N_PACKETS,const int PACKET_SIZE,const int PACKETS_PER_SECOND){
    for(int i=0;i<N_PACKETS;i++){
        auto packet=createRandomDataBuffer(PACKET_SIZE);
        cb(packet);
    }
}

#include <UDPSender.h>
static void test_latency(){
    // For a packet size of 1024 bytes, 1024 packets per second equals 1 MB/s or 8 MBit/s
    // 8 MBit/s is a just enough for encoded 720p video
    const int PACKET_SIZE=1024;
    const int WANTED_PACKETS_PER_SECOND=1*1024;
    const std::chrono::nanoseconds TIME_BETWEEN_PACKETS=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1))/WANTED_PACKETS_PER_SECOND;
    const int N_PACKETS=60;

    // start the receiver in its own thread
    UDPReceiver udpReceiver{nullptr,6001,"LTUdpRec",0,validateReceivedData,0};
    udpReceiver.startReceiving();
    // Wait a bit such that the OS can start the receiver before we start sending data
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    UDPSender udpSender{"127.0.0.1",6001};
    currentSequenceNumber=0;
    avgUDPProcessingTime.reset();

    const std::chrono::steady_clock::time_point testBegin=std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point firstPacketTimePoint=std::chrono::steady_clock::now();
    std:size_t writtenBytes=0;
    std::size_t writtenPackets=0;
    for(int i=0;i<N_PACKETS;i++){
        auto buff=createRandomDataBuffer(PACKET_SIZE);
        writeSequenceNumberAndTimestamp(buff);
        udpSender.mySendTo(buff.data(),buff.size());
        writtenBytes+=PACKET_SIZE;
        writtenPackets+=1;
        currentSequenceNumber++;
        // wait until as much time is elapsed such that we hit the target packets per seconds
        const auto timePointReadyToSendNextPacket=firstPacketTimePoint+i*TIME_BETWEEN_PACKETS;
        while(std::chrono::steady_clock::now()<timePointReadyToSendNextPacket){
            //busy wait
        }
    }
    const auto testEnd=std::chrono::steady_clock::now();
    const double testTimeSeconds=(testEnd-testBegin).count()/1000.0f/1000.0f/1000.0f;
    const double actualPacketsPerSecond=(double)N_PACKETS/testTimeSeconds;
    const double actualMBytesPerSecond=(double)writtenBytes/testTimeSeconds/1024.0f/1024;

   // Wait for any packet that might be still in transit
   std::this_thread::sleep_for(std::chrono::seconds(1));
   udpReceiver.stopReceiving();


   MLOGD<<"WANTED_PACKETS_PER_SECOND "<<WANTED_PACKETS_PER_SECOND<<" Got "<<actualPacketsPerSecond<<" TTS "<<testTimeSeconds<<" MB/s "<<actualMBytesPerSecond;
   MLOGD<<"Avg UDP processing time "<<avgUDPProcessingTime.getAvgReadable();

}




//----------------------------------------------------JAVA bindings---------------------------------------------------------------
#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_video_core_player_VideoPlayer_##method_name

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
(JNIEnv * env, jclass jclass1,jlong videoPlayerN,jobject androidContext){
    native(videoPlayerN)->start(env,androidContext);
}

JNI_METHOD(void, nativeStop)
(JNIEnv * env,jclass jclass1,jlong videoPlayerN,jobject androidContext){
    native(videoPlayerN)->stop(env,androidContext);
}

JNI_METHOD(void, nativeSetVideoSurface)
(JNIEnv * env, jclass jclass1,jlong videoPlayerN,jobject surface){
    native(videoPlayerN)->setVideoSurface(env,surface);
}

//This function is only called when the data for the video is coming from the dji receiver.
//In this case, the receiving is done via JAVA, and the received bytes are transfered from JAVA to NDK
JNI_METHOD(void, nativePassNALUData)
(JNIEnv *env,jclass jclass1,jlong videoPlayerN,jbyteArray b,jint offset,jint length){
    jbyte *arrayP=env->GetByteArrayElements(b, nullptr);
    auto * p=(uint8_t*)arrayP;
    native(videoPlayerN)->onNewVideoData(&p[(size_t) offset], (size_t)length,VideoPlayer::VIDEO_DATA_TYPE::DJI);
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
    if(p->mUDPReceiver== nullptr){
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
            jclass jcDecodingInfo = env->FindClass("constantin/video/core/player/DecodingInfo");
            assert(jcDecodingInfo!=nullptr);
            jmethodID jcDecodingInfoConstructor = env->GetMethodID(jcDecodingInfo, "<init>", "(FFFFFII)V");
            assert(jcDecodingInfoConstructor!= nullptr);
            const auto info=p->latestDecodingInfo;
            auto decodingInfo=env->NewObject(jcDecodingInfo,jcDecodingInfoConstructor,(jfloat)info.currentFPS,(jfloat)info.currentKiloBitsPerSecond,
                           (jfloat)info.avgParsingTime_ms,(jfloat)info.avgWaitForInputBTime_ms,(jfloat)info.avgDecodingTime_ms,(jint)info.nNALU,(jint)info.nNALUSFeeded);
            assert(decodingInfo!=nullptr);
            jmethodID onDecodingInfoChangedJAVA = env->GetMethodID(jClassExtendsIVideoParamsChanged, "onDecodingInfoChanged", "(Lconstantin/video/core/player/DecodingInfo;)V");
            assert(onDecodingInfoChangedJAVA!=nullptr);
            env->CallVoidMethod(videoParamsChangedI,onDecodingInfoChangedJAVA,decodingInfo);
            p->latestDecodingInfoChanged=false;
        }
    }
}

JNI_METHOD(void , testLatency)
(JNIEnv *env,jclass jclass1) {
    test_latency();
}

}
