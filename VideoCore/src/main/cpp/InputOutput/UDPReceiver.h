
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

//Starts a new thread that continuously checks for new data on UDP port

class UDPReceiver {
public:
    typedef std::function<void(const uint8_t[],size_t)> DATA_CALLBACK;
    typedef std::function<void(const std::string)> SOURCE_IP_CALLBACK;
public:
    /**
     * @param port : The port to listen on
     * @param CPUPriority: The priority the receiver thread will run with
     * @param onDataReceivedCallback: called every time new data is received
     * @param WANTED_RCVBUF_SIZE: The buffer allocated by the OS might not be sufficient to buffer incoming data when receiving at a high data rate
     * If @param WANTED_RCVBUF_SIZE is bigger than the size allocated by the OS a bigger buffer is requested, but it is not
     * guaranteed that the size is actually increased. Use 0 to leave tha buffer size untouched
     */
    UDPReceiver(int port,const std::string& name,int CPUPriority,const DATA_CALLBACK& onDataReceivedCallback,size_t WANTED_RCVBUF_SIZE=0);
    /**
     * Register a callback that is called once and contains the IP address of the first received packet's sender
     */
    void registerOnSourceIPFound(const SOURCE_IP_CALLBACK& onSourceIP);
    /**
     * Start receiver thread,which opens UDP port
     */
    void startReceiving();
    /**
     * Stop and join receiver thread, which closes port
     */
    void stopReceiving();
    long getNReceivedBytes()const;
    std::string getSourceIPAddress()const;
    int getPort()const;
private:
    void receiveFromUDPLoop();
    const DATA_CALLBACK onDataReceivedCallback=nullptr;
    SOURCE_IP_CALLBACK onSourceIP= nullptr;
    const int mPort;
    const int mCPUPriority;
    const size_t WANTED_RCVBUF_SIZE;
    const std::string mName;
    ///We need this reference to stop the receiving thread
    int mSocket;
    std::string senderIP="0.0.0.0";
    std::atomic<bool> receiving=false;
    std::atomic<long> nReceivedBytes=0;
    std::unique_ptr<std::thread> mUDPReceiverThread;
    //https://en.wikipedia.org/wiki/User_Datagram_Protocol
    //65,507 bytes (65,535 − 8 byte UDP header − 20 byte IP header).
    static constexpr const size_t UDP_PACKET_MAX_SIZE=65507;
};

#endif // FPV_VR_UDPRECEIVER_H