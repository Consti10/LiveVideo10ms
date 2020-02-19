
#include "UDPReceiver.h"
#include "CPUPriority.hpp"
#include <arpa/inet.h>
#include <vector>
#include <sstream>

#define TAG "UDPReceiver"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

UDPReceiver::UDPReceiver(int port,const std::string& name,int CPUPriority,const DATA_CALLBACK& onDataReceivedCallback,size_t buffsize):
        mPort(port),mName(name),RECV_BUFF_SIZE(buffsize),mCPUPriority(CPUPriority),onDataReceivedCallback(onDataReceivedCallback){
}

void UDPReceiver::registerOnSourceIPFound(const SOURCE_IP_CALLBACK& onSourceIP) {
    this->onSourceIP=onSourceIP;
}

long UDPReceiver::getNReceivedBytes()const {
    return nReceivedBytes;
}

std::string UDPReceiver::getSourceIPAddress()const {
    return senderIP;
}

void UDPReceiver::startReceiving() {
    receiving=true;
    mUDPReceiverThread=std::make_unique<std::thread>([this]{this->receiveFromUDPLoop();} );
}

void UDPReceiver::stopReceiving() {
    if(receiving== false){
        LOGD("UDP Receiver %s already stopped",mName.c_str());
        return;
    }
    receiving=false;
    shutdown(mSocket,SHUT_RD);
    if(mUDPReceiverThread->joinable()){
        mUDPReceiverThread->join();
    }
    mUDPReceiverThread.reset();
}

void UDPReceiver::receiveFromUDPLoop() {
    mSocket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (mSocket == -1) {
        LOGD("Error creating socket");
        return;
    }
    int enable = 1;
    if (setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        LOGD("Error setting reuse");
    }
    //
    //setMaxSocketBuffer();
    // 30Mbps 50ms buffer
    int val=1;
    socklen_t len=sizeof(int);
    getsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, &val,&len);
    LOGD("Default socket recv buffer is %d bytes", val);

    /*val = 30 * 1000 * 500 / 8;
    setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (char *) &val, sizeof(val));
    len = sizeof(val);
    getsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (char *) &val, &len);
    LOGD("Current socket recv buffer is %d bytes", val);*/

    //


    setCPUPriority(mCPUPriority,mName);
    struct sockaddr_in myaddr;
    memset((uint8_t *) &myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(mPort);
    if (bind(mSocket, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        LOGD("Error binding Port; %d", mPort);
        return;
    }
    std::vector<uint8_t> buff(RECV_BUFF_SIZE);
    sockaddr_in source;
    socklen_t sourceLen= sizeof(sockaddr_in);

    std::vector<uint8_t> sequenceNumbers;


    while (receiving) {
        //TODO investigate: does a big buffer size create latency with MSG_WAITALL ?
        //Hypothesis: it should on linux, but does not on android ? what ?
        const ssize_t message_length = recvfrom(mSocket,buff.data(),RECV_BUFF_SIZE, MSG_WAITALL,(sockaddr*)&source,&sourceLen);
        //ssize_t message_length = recv(mSocket, buff, (size_t) mBuffsize, MSG_WAITALL);
        if (message_length > 0) { //else -1 was returned;timeout/No data received
            //LOGD("Data size %d",(int)message_length);

            //Custom protocoll where first byte of udp packet is sequence number
            if(false){
                uint8_t nr;
                memcpy(&nr,buff.data(),1);
                sequenceNumbers.push_back(nr);
                if(sequenceNumbers.size()>32){
                    std::stringstream ss;
                    for(const auto seqnr:sequenceNumbers){
                        ss<<((int)seqnr)<<" ";
                    }
                    bool allInOrder=true;
                    bool allInAscendingOrder=true;
                    for(size_t i=0;i<sequenceNumbers.size()-1;i++){
                        if((sequenceNumbers.at(i)+1) != sequenceNumbers.at(i+1)){
                            allInOrder=false;
                        }
                        if((sequenceNumbers.at(i)) >= sequenceNumbers.at(i+1)){
                            allInAscendingOrder=false;
                        }
                    }
                    LOGD("Seq numbers. In order %d  In ascending order %d values : %s",(int)allInOrder,(int)allInAscendingOrder,ss.str().c_str());
                    sequenceNumbers.resize(0);
                }
                onDataReceivedCallback(&buff.data()[1], (size_t)message_length);
            }else{
                onDataReceivedCallback(buff.data(), (size_t)message_length);
            }

            nReceivedBytes+=message_length;
            //The source ip stuff
            const char* p=inet_ntoa(source.sin_addr);
            std::string s1=std::string(p);
            if(senderIP!=s1){
                senderIP=s1;
            }
            if(onSourceIP!=nullptr){
                onSourceIP(p);
            }
        }else{
            if(errno != EWOULDBLOCK) {
                LOGD("Error on recvfrom. errno=%d %s", errno, strerror(errno));
            }
        }
    }
    close(mSocket);
}

int UDPReceiver::getPort() const {
    return mPort;
}
