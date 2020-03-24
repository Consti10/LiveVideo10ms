
#include "UDPReceiver.h"
#include <CPUPriority.hpp>
#include <arpa/inet.h>
#include <vector>
#include <sstream>
#include <array>

#define TAG "UDPReceiver"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

UDPReceiver::UDPReceiver(int port,const std::string& name,int CPUPriority,const DATA_CALLBACK& onDataReceivedCallback,size_t WANTED_RCVBUF_SIZE):
        mPort(port),mName(name),WANTED_RCVBUF_SIZE(WANTED_RCVBUF_SIZE),mCPUPriority(CPUPriority),onDataReceivedCallback(onDataReceivedCallback){
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
    receiving=false;
    //this stops the recvfrom even if in blocking mode
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
    int recvBufferSize=0;
    socklen_t len=sizeof(recvBufferSize);
    getsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, &recvBufferSize, &len);
    LOGD("Default socket recv buffer is %d bytes", recvBufferSize);

    if(WANTED_RCVBUF_SIZE>recvBufferSize){
        recvBufferSize=WANTED_RCVBUF_SIZE;
        if(setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, &WANTED_RCVBUF_SIZE,len)) {
            LOGD("Cannot increase buffer size to %d bytes", WANTED_RCVBUF_SIZE);
        }
        getsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, &recvBufferSize, &len);
        LOGD("Wanted %d Set %d", WANTED_RCVBUF_SIZE,recvBufferSize);
    }
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
    //wrap into unique pointer to avoid running out of stack
    const auto buff=std::make_unique<std::array<uint8_t,UDP_PACKET_MAX_SIZE>>();

    sockaddr_in source;
    socklen_t sourceLen= sizeof(sockaddr_in);
    std::vector<uint8_t> sequenceNumbers;

    while (receiving) {
        //TODO investigate: does a big buffer size create latency with MSG_WAITALL ?
        const ssize_t message_length = recvfrom(mSocket,buff->data(),UDP_PACKET_MAX_SIZE, MSG_WAITALL,(sockaddr*)&source,&sourceLen);
        //ssize_t message_length = recv(mSocket, buff, (size_t) mBuffsize, MSG_WAITALL);
        if (message_length > 0) { //else -1 was returned;timeout/No data received
            //LOGD("Data size %d",(int)message_length);

            //Custom protocol where first byte of udp packet is sequence number
            if(false){
                uint8_t nr;
                memcpy(&nr,buff->data(),1);
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
                onDataReceivedCallback(&buff->data()[1], (size_t)message_length);
            }else{
                onDataReceivedCallback(buff->data(), (size_t)message_length);
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
