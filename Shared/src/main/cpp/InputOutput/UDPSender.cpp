//
// Created by geier on 28/02/2020.
//

#include "UDPSender.h"
#include <jni.h>
#include <cstdlib>
#include <pthread.h>
#include <cerrno>
#include <sys/ioctl.h>
#include <endian.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <AndroidLogger.hpp>
#include <StringHelper.hpp>


UDPSender::UDPSender(const std::string &IP,const int Port,const int WANTED_SNDBUFF_SIZE):
        WANTED_SNDBUFF_SIZE(WANTED_SNDBUFF_SIZE)
{
    //create the socket
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if (sockfd < 0) {
        MLOGD<<"Cannot create socket";
    }
    //Create the address
    address.sin_family = AF_INET;
    address.sin_port = htons(Port);
    inet_pton(AF_INET,IP.c_str(), &address.sin_addr);
    //
    int sendBufferSize=0;
    socklen_t len=sizeof(sendBufferSize);
    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendBufferSize, &len);
    MLOGD<<"Default socket send buffer is "<<StringHelper::memorySizeReadable(sendBufferSize);
    if(WANTED_SNDBUFF_SIZE!=0){
        if(setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &WANTED_SNDBUFF_SIZE,len)) {
            MLOGD<<"Cannot increase buffer size to "<<StringHelper::memorySizeReadable(WANTED_SNDBUFF_SIZE);
        }
        sendBufferSize=0;
        getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendBufferSize, &len);
        MLOGD<<"Wanted "<<StringHelper::memorySizeReadable(WANTED_SNDBUFF_SIZE)<<" Set "<<StringHelper::memorySizeReadable(sendBufferSize);
    }
}

void UDPSender::mySendTo(const uint8_t* data, ssize_t data_length) {
    if(data_length>UDP_PACKET_MAX_SIZE){
        MLOGE<<"Data size exceeds UDP packet size";
        return;
    }
    nSentBytes+=data_length;
    // Measure the time this call takes (is there some funkiness ? )
    timeSpentSending.start();
    const auto result= sendto(sockfd, data, data_length, 0, (struct sockaddr *) &(address),
                                sizeof(struct sockaddr_in));
    if(result<0){
        MLOGE<<"Cannot send data "<<data_length<<" "<<strerror(errno);
    }else{
        //MLOGD<<"Sent "<<data_length;
    }
    timeSpentSending.stop();
    if(timeSpentSending.getNSamples()>100){
        MLOGD<<"TimeSS "<<timeSpentSending.getAvgReadable();
        timeSpentSending.reset();
    }
}

UDPSender::~UDPSender() {
    //TODO
}
