
#ifndef FPV_VR_UDPRECEIVER_H
#define FPV_VR_UDPRECEIVER_H

#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <android/log.h>
#include <iostream>
#include <thread>
#include <atomic>

class UDPReceiver {
public:
    typedef std::function<void(uint8_t[],int)> DATA_CALLBACK;
    typedef std::function<void(const char*)> SOURCE_IP_CALLBACK;
public:
    UDPReceiver(int port,const std::string& name,int CPUPriority,int buffsize,const DATA_CALLBACK& onDataReceivedCallback);
    void registerOnSourceIPfound(const SOURCE_IP_CALLBACK& onSourceIP);
    void startReceiving();
    void stopReceiving();
    long getNReceivedBytes()const;
    std::string getSourceIPAddress()const;
    int getPort()const;
private:
    void receiveFromUDPLoop();
    DATA_CALLBACK onDataReceivedCallback=nullptr;
    SOURCE_IP_CALLBACK onSourceIP= nullptr;
    const int mPort;
    const int mCPUPriority;
    const int mBuffsize;
    const std::string mName;
    std::string mIP="0.0.0.0";
    std::atomic<bool> receiving;
    std::atomic<long> nReceivedBytes;
    int mSocket;
    std::unique_ptr<std::thread> mUDPReceiverThread;
};

#endif // FPV_VR_UDPRECEIVER_H