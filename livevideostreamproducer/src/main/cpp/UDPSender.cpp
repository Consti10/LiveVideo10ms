//
// Created by geier on 28/02/2020.
//

#include "UDPSender.h"
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

//#include <wifibroadcast/fec.hh>
#include "../../../../VideoCore/src/main/cpp/XFEC/include/wifibroadcast/fec.hh"

UDPSender::UDPSender(const std::string &IP,const int Port) {
    //create the socket
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if (sockfd < 0) {
        MLOGD<<"Cannot create socket";
    }
    //Create the address
    address.sin_family = AF_INET;
    address.sin_port = htons(Port);
    inet_pton(AF_INET,IP.c_str(), &address.sin_addr);
}

//Split data into smaller packets when exceeding UDP max packet size
void UDPSender::splitAndSend(const uint8_t *data, ssize_t data_length,bool addSeqNr) {
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
            mySendTo(&data[offset],remaining,addSeqNr);
            break;
        }
        mySendTo(&data[offset],MAX_VIDEO_DATA_PACKET_SIZE,addSeqNr);
        offset+=MAX_VIDEO_DATA_PACKET_SIZE;
    }
    //if(data_length>MAX_VIDEO_DATA_PACKET_SIZE){
    //    mySendTo(data,MAX_VIDEO_DATA_PACKET_SIZE);
    //    splitAndSend(&data[MAX_VIDEO_DATA_PACKET_SIZE], data_length - MAX_VIDEO_DATA_PACKET_SIZE);
    //}else{
    //    mySendTo(data,data_length);
    //}
}

void UDPSender::mySendTo(const uint8_t* data, ssize_t data_length,bool addSeqNr) {
    timeSpentSending.start();
    if(addSeqNr) {
        std::memcpy(workingBuffer.data(),&sequenceNumber,sizeof(uint32_t));
        std::memcpy(&workingBuffer.data()[sizeof(uint32_t)],data,data_length);
        sequenceNumber++;
        // Send the packet N times
        for(int i=0;i<1;i++){
            const auto result = sendto(sockfd,workingBuffer.data(), data_length+sizeof(uint32_t), 0, (struct sockaddr *) &(address),
                                       sizeof(struct sockaddr_in));
            if (result < 0) {
                MLOGE << "Cannot send data " << data_length << " " << strerror(errno)<<" ret:"<<result;
            } else {
                //MLOGD << "Sent " << data_length;
            }
        }
    }else{
        const auto result=sendto(sockfd,data,data_length, 0, (struct sockaddr *)&(address), sizeof(struct sockaddr_in));
        if(result<0){
            MLOGE<<"Cannot send data "<<data_length<<" "<<strerror(errno);
        }else{
            //MLOGD<<"Sent "<<data_length;
        }
    }
    timeSpentSending.stop();
    if(timeSpentSending.getNSamples()>100){
        MLOGD<<"TimeSS "<<timeSpentSending.getAvgReadable();
        timeSpentSending.reset();
    }
}

void UDPSender::FECSend(const uint8_t *data, ssize_t data_length) {
    std::vector<std::shared_ptr<FECBlock> > blks = enc.encode_buffer(data,data_length);
    if(blks.size()==0){
        MLOGE<<"Cannot encode";
    }
    for (auto blk : blks) {
        mySendTo(blk->data(),blk->data_length(), false);
    }
}



//----------------------------------------------------JAVA bindings---------------------------------------------------------------

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_livevideostreamproducer_UDPSender_##method_name

inline jlong jptr(UDPSender *p) {
    return reinterpret_cast<intptr_t>(p);
}
inline UDPSender *native(jlong ptr) {
    return reinterpret_cast<UDPSender*>(ptr);
}

extern "C" {

JNI_METHOD(jlong, nativeConstruct)
(JNIEnv *env, jobject obj, jstring ip,jint port) {
    return jptr(new UDPSender(NDKArrayHelper::DynamicSizeString(env,ip),(int)port));
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
        native(p)->splitAndSend((uint8_t *) data, (ssize_t) size, false);
    }else if(streamMode==1){
        native(p)->splitAndSend((uint8_t *) data, (ssize_t) size, true);
    }else{
        native(p)->FECSend((uint8_t *) data, (ssize_t) size);
    }
}

}
