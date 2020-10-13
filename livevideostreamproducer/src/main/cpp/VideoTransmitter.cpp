//
// Created by geier on 13/10/2020.
//

#include <string>
#include <arpa/inet.h>
#include <array>

#include <TimeHelper.hpp>

#include "../../../../VideoCore/src/main/cpp/XFEC/include/wifibroadcast/fec.hh"

#include "../../../../Shared/src/main/cpp/InputOutput/UDPSender.h"
#include "../../../../VideoCore/src/main/cpp/Parser/ParseRTP.h"
#include <NDKArrayHelper.hpp>
#include <AndroidLogger.hpp>

#include <jni.h>
#include <cstdlib>
#include <pthread.h>
#include <cerrno>
#include <sys/ioctl.h>
#include <endian.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <StringHelper.hpp>


class VideoTransmitter{
public:
    VideoTransmitter(const std::string& IP,const int Port):
    mUDPSender(IP,Port),
    mEncodeRTP(std::bind(&VideoTransmitter::newRTPPacket, this, std::placeholders::_1)){}
    /**
     * send data to the ip and port set previously. Logs error on failure.
     * If data length exceeds the max UDP packet size, the method splits data into smaller packets
     */
    void splitAndSend(const uint8_t* data, ssize_t data_length,bool addSeqNr);
    //
    void sendPacket(const uint8_t* data, ssize_t data_length,bool addSeqNr);
    //
    void FECSend(const uint8_t* data, ssize_t data_length);
    //
    void RTPSend(const uint8_t* data, ssize_t data_length);
private:
    UDPSender mUDPSender;
    //static constexpr const size_t MAX_VIDEO_DATA_PACKET_SIZE=UDP_PACKET_MAX_SIZE--sizeof(uint32_t);
    static constexpr const size_t MAX_VIDEO_DATA_PACKET_SIZE=1024-sizeof(uint32_t);
    int32_t sequenceNumber=0;
    std::array<uint8_t,UDPSender::UDP_PACKET_MAX_SIZE> workingBuffer;
    AvgCalculator avgDeltaBetweenVideoPackets;
    std::chrono::steady_clock::time_point lastForwardedPacket{};
    //
    FECBufferEncoder enc{1024,0.5f};
    AvgCalculator avgNALUSize;
    //
    EncodeRTP mEncodeRTP;
    void newRTPPacket(const EncodeRTP::RTPPacket packet);
};

//Split data into smaller packets when exceeding UDP max packet size
void VideoTransmitter::splitAndSend(const uint8_t *data, ssize_t data_length,bool addSeqNr) {
    avgNALUSize.add(std::chrono::nanoseconds(data_length));
    if(avgNALUSize.getNSamples() > 100){
        MLOGD<<"NALUSize"
             <<" min:"<<StringHelper::memorySizeReadable(avgNALUSize.getMin().count())
             <<" max:"<<StringHelper::memorySizeReadable(avgNALUSize.getMax().count())
             <<" avg:"<<StringHelper::memorySizeReadable(avgNALUSize.getAvg().count());
        avgNALUSize.reset();
    }
    if(lastForwardedPacket==std::chrono::steady_clock::time_point{}){
        lastForwardedPacket=std::chrono::steady_clock::now();
    }else{
        const auto now=std::chrono::steady_clock::now();
        const auto delta=now-lastForwardedPacket;
        lastForwardedPacket=now;
        avgDeltaBetweenVideoPackets.add(delta);
        if(delta>std::chrono::milliseconds(150)){
            MLOGD<<"Dafuq why so high";
        }
        if( avgDeltaBetweenVideoPackets.getNSamples()>120){
            MLOGD<<""<<avgDeltaBetweenVideoPackets.getAvgReadable();
            avgDeltaBetweenVideoPackets.reset();
        }
    }
    if(data_length<=0)return;
    // Recursion is more pretty but dang the stack function pointer exception
    std::size_t offset=0;
    while (true){
        std::size_t remaining=data_length-offset;
        if(remaining<=MAX_VIDEO_DATA_PACKET_SIZE){
            sendPacket(&data[offset],remaining,addSeqNr);
            break;
        }
        sendPacket(&data[offset],MAX_VIDEO_DATA_PACKET_SIZE,addSeqNr);
        offset+=MAX_VIDEO_DATA_PACKET_SIZE;
    }
    //if(data_length>MAX_VIDEO_DATA_PACKET_SIZE){
    //    mySendTo(data,MAX_VIDEO_DATA_PACKET_SIZE);
    //    splitAndSend(&data[MAX_VIDEO_DATA_PACKET_SIZE], data_length - MAX_VIDEO_DATA_PACKET_SIZE);
    //}else{
    //    mySendTo(data,data_length);
    //}
}

void VideoTransmitter::sendPacket(const uint8_t *data, ssize_t data_length, bool addSeqNr) {
    if(addSeqNr){
        std::memcpy(workingBuffer.data(),&sequenceNumber,sizeof(uint32_t));
        std::memcpy(&workingBuffer.data()[sizeof(uint32_t)],data,data_length);
        sequenceNumber++;
        for(int i=0;i<1;i++){
            mUDPSender.mySendTo(workingBuffer.data(), data_length + sizeof(uint32_t));
        }
    } else{
        mUDPSender.mySendTo(data, data_length);
    }
}

void VideoTransmitter::FECSend(const uint8_t *data, ssize_t data_length) {
    std::vector<std::shared_ptr<FECBlock> > blks = enc.encode_buffer(data,data_length);
    if(blks.size()==0){
        MLOGE<<"Cannot encode";
    }
    for (auto blk : blks) {
        mUDPSender.mySendTo(blk->data(), blk->data_length());
    }
}

void VideoTransmitter::RTPSend(const uint8_t *data, ssize_t data_length) {
    mEncodeRTP.parseNALtoRTP(30,data,data_length);
}

void VideoTransmitter::newRTPPacket(const EncodeRTP::RTPPacket packet) {
    mUDPSender.mySendTo(packet.data, packet.data_len);
}

//----------------------------------------------------JAVA bindings---------------------------------------------------------------

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_livevideostreamproducer_VideoTransmitter_##method_name

inline jlong jptr(VideoTransmitter *p) {
    return reinterpret_cast<intptr_t>(p);
}
inline VideoTransmitter *native(jlong ptr) {
    return reinterpret_cast<VideoTransmitter*>(ptr);
}

extern "C" {

JNI_METHOD(jlong, nativeConstruct)
(JNIEnv *env, jobject obj, jstring ip,jint port) {
    return jptr(new VideoTransmitter(NDKArrayHelper::DynamicSizeString(env,ip),(int)port));
}
JNI_METHOD(void, nativeDelete)
(JNIEnv *env, jobject obj, jlong p) {
    delete native(p);
}

JNI_METHOD(void, nativeSend)
(JNIEnv *env, jobject obj, jlong p,jobject buf,jint size,jint streamMode) {
    //jlong size=env->GetDirectBufferCapacity(buf);
    auto *data = (jbyte*)env->GetDirectBufferAddress(buf);
    if(data== nullptr){
        MLOGE<<"Something wrong with the byte buffer (is it direct ?)";
    }
    //LOGD("size %d",size);
    if(streamMode==0){
        //native(p)->splitAndSend((uint8_t *) data, (ssize_t) size, false);
        native(p)->RTPSend((uint8_t*)data,(ssize_t)size);
    }else if(streamMode==1){
        native(p)->splitAndSend((uint8_t *) data, (ssize_t) size, true);
    }else{
        native(p)->FECSend((uint8_t *) data, (ssize_t) size);
    }
}

}