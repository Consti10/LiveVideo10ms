//
// Created by geier on 28/02/2020.
//

#ifndef LIVEVIDEOSTREAMPRODUCER_UDPSENDER_H
#define LIVEVIDEOSTREAMPRODUCER_UDPSENDER_H

#include <string>
#include <arpa/inet.h>
#include <array>
#include <TimeHelper.hpp>

class UDPSender{
public:
    /**
     * Construct a UDP sender that sends UDP data packets
     * @param IP ipv4 address to send data to
     * @param Port port for sending data
     */
    UDPSender(const std::string& IP,const int Port);
    ~UDPSender();
    //
    void mySendTo(const uint8_t* data,ssize_t data_length);
    //https://en.wikipedia.org/wiki/User_Datagram_Protocol
    //65,507 bytes (65,535 − 8 byte UDP header − 20 byte IP header).
    static constexpr const size_t UDP_PACKET_MAX_SIZE=65507;
private:
    int sockfd;
    sockaddr_in address{};
    Chronometer timeSpentSending;
};


#endif //LIVEVIDEOSTREAMPRODUCER_UDPSENDER_H
