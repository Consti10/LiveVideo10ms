//
// Created by geier on 13/10/2020.
//

#include <string>
#include <arpa/inet.h>
#include <array>

#include <TimeHelper.hpp>

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
#include <UDPSender.h>
#include "../Parser/ParseRTP.h"
#include "../Parser/EncodeRtpTest.h"
#include <ATraceCompbat.hpp>


class VideoTransmitter{
public:
    VideoTransmitter(const std::string& IP,const int Port):
    mUDPSender(IP,Port,UDPSender::EXAMPLE_MEDIUM_SNDBUFF_SIZE),
    mEncodeRTP(std::bind(&VideoTransmitter::newRTPPacket, this, std::placeholders::_1),MY_RTP_PACKET_MAX_SIZE){}
    /**
     * send data to the ip and port set previously. Logs error on failure.
     * If data length exceeds the max UDP packet size, the method splits data into smaller packets
     */
    void splitAndSend(const uint8_t* data, ssize_t data_length);
    //
    void sendPacket(const uint8_t* data, ssize_t data_length);
    //
    //
    void RTPSend(const uint8_t* data, ssize_t data_length);
    AvgCalculatorSize avgNALUSize;
    // Do FEC over the RTP packets
    bool DO_FEC_WRAPPING=false;
    // Prepend each udp packets with 4 bytes of sequence numbers (for raw)
    bool ADD_SEQUENCE_NR=false;
    int SEND_EACH_RTP_PACKET_MULTIPLE_TIMES=0;
private:
    // RTP parser splits into packets of this maximum size
    static constexpr const size_t MY_RTP_PACKET_MAX_SIZE=65507;//TODO remove
    UDPSender mUDPSender;
    static constexpr const size_t MAX_VIDEO_DATA_PACKET_SIZE=1024-sizeof(uint32_t);
    int32_t sequenceNumber=0;
    std::array<uint8_t,UDPSender::UDP_PACKET_MAX_SIZE> workingBuffer;
    AvgCalculator avgTimeBetweenVideoNALUS;
    std::chrono::steady_clock::time_point lastForwardedPacket{};
    //
    //FECBufferEncoder enc{1500,0.5f};
    //
    RTPEncoder mEncodeRTP;
    void newRTPPacket(const RTPEncoder::RTPPacket& packet);
    //FECDecoder mFECDecoder;
};

//Split data into smaller packets when exceeding UDP max packet size
void VideoTransmitter::splitAndSend(const uint8_t *data, ssize_t data_length) {
    avgNALUSize.add(data_length);
    if(avgNALUSize.getNSamples() > 100){
        MLOGD<<"NALUSize "<<avgNALUSize.getAvgReadable();
        avgNALUSize.reset();
    }
    if(lastForwardedPacket==std::chrono::steady_clock::time_point{}){
        lastForwardedPacket=std::chrono::steady_clock::now();
    }else{
        const auto now=std::chrono::steady_clock::now();
        const auto delta=now-lastForwardedPacket;
        lastForwardedPacket=now;
        avgTimeBetweenVideoNALUS.add(delta);
        if(delta>std::chrono::milliseconds(150)){
            MLOGD<<"Dafuq why so high";
        }
        if(avgTimeBetweenVideoNALUS.getNSamples() > 120){
            MLOGD << "" << avgTimeBetweenVideoNALUS.getAvgReadable();
            avgTimeBetweenVideoNALUS.reset();
        }
    }
    if(data_length<=0)return;
    // Recursion is more pretty but dang the stack function pointer exception
    std::size_t offset=0;
    while (true){
        std::size_t remaining=data_length-offset;
        if(remaining<=MAX_VIDEO_DATA_PACKET_SIZE){
            sendPacket(&data[offset],remaining);
            break;
        }
        sendPacket(&data[offset],MAX_VIDEO_DATA_PACKET_SIZE);
        offset+=MAX_VIDEO_DATA_PACKET_SIZE;
    }
    //if(data_length>MAX_VIDEO_DATA_PACKET_SIZE){
    //    mySendTo(data,MAX_VIDEO_DATA_PACKET_SIZE);
    //    splitAndSend(&data[MAX_VIDEO_DATA_PACKET_SIZE], data_length - MAX_VIDEO_DATA_PACKET_SIZE);
    //}else{
    //    mySendTo(data,data_length);
    //}
}

void VideoTransmitter::sendPacket(const uint8_t *data, ssize_t data_length) {
    if(ADD_SEQUENCE_NR){
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

void VideoTransmitter::RTPSend(const uint8_t *data, ssize_t data_length) {
    ATrace_beginSection("VideoTransmitter::RTPSend");
    mEncodeRTP.parseNALtoRTP(30,data,data_length);
    ATrace_endSection();
}

void VideoTransmitter::newRTPPacket(const RTPEncoder::RTPPacket& packet) {
    /*std::vector<uint8_t> tmp;
    tmp.reserve(1024);
    for(int i=0;i<1024;i++){
        tmp.push_back((uint8_t)i);
    }*/
    if(DO_FEC_WRAPPING){
       /* ATrace_beginSection("VideoTransmitter::FECWrapping");
        MLOGD<<"Wrapping rtp packet into FEC";
        assert(packet.data_len<=1024);
        std::vector<std::shared_ptr<FECBlock> > blks = enc.encode_buffer(packet.data,packet.data_len);
        // With a ratio of 0.5 we should get exactly 2 blocks
        assert(blks.size()==2);
        ATrace_endSection();
        for (auto blk : blks) {
            ATrace_beginSection("UDP::sendto");
            mUDPSender.mySendTo(blk->pkt_data(), blk->pkt_length());
            ATrace_endSection();
        }*/
        //
    }else{
        // To emulate a higher bitstream rate (the receiver has to drop duplicates though)
        // Only enabled in 'CUSTOM' mode
        if(SEND_EACH_RTP_PACKET_MULTIPLE_TIMES>0){
            for(int i=0;i<SEND_EACH_RTP_PACKET_MULTIPLE_TIMES;i++){
                mUDPSender.mySendTo(packet.data, packet.data_len);
            }
        }else{
            mUDPSender.mySendTo(packet.data, packet.data_len);
        }
    }
}


//----------------------------------------------------JAVA bindings---------------------------------------------------------------

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_video_transmitter_VideoTransmitter_##method_name

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
    native(p)->DO_FEC_WRAPPING=false;
    native(p)->ADD_SEQUENCE_NR=false;
    native(p)->SEND_EACH_RTP_PACKET_MULTIPLE_TIMES=0;
    //LOGD("size %d",size);
    if(streamMode==0){
        // RTP
        native(p)->RTPSend((uint8_t*)data,(ssize_t)size);
    }else if(streamMode==1){
        // RAW
        native(p)->splitAndSend((uint8_t *) data, (ssize_t) size);
    }else if(streamMode==2){
        // Send each rtp packet multiple times
        native(p)->SEND_EACH_RTP_PACKET_MULTIPLE_TIMES=5;
        native(p)->RTPSend((uint8_t*)data,(ssize_t)size);
    }else{
        // RTP inside FEC over UDP
        native(p)->DO_FEC_WRAPPING=true;
        native(p)->RTPSend((uint8_t *) data, (ssize_t) size);
    }
}

}