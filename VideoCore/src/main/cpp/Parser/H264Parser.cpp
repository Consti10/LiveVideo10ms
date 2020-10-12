//
// Created by Constantin on 24.01.2018.
//
#include "H264Parser.h"
#include <cstring>
#include <android/log.h>
#include <endian.h>
#include <chrono>
#include <thread>
#include <StringHelper.hpp>

H264Parser::H264Parser(NALU_DATA_CALLBACK onNewNALU):
    onNewNALU(std::move(onNewNALU)),
    mParseRAW(std::bind(&H264Parser::newNaluExtracted, this, std::placeholders::_1)),
    mParseRTP(std::bind(&H264Parser::newNaluExtracted, this, std::placeholders::_1)){
}

void H264Parser::reset(){
    mParseRAW.reset();
    mParseRTP.reset();
    nParsedNALUs=0;
    nParsedKeyFrames=0;
    setLimitFPS(-1);
    lastForwardedSequenceNr=-1;
    droppedPacketsSinceLastForwardedPacket=0;
}

void H264Parser::parse_raw_h264_stream(const uint8_t *data,const size_t data_length) {
    //LOGD("H264Parser::parse_raw_h264_stream %d",data_length);
    mParseRAW.parseData(data,data_length);
}

void H264Parser::parse_rtp_h264_stream(const uint8_t *rtp_data,const size_t data_length) {
    mParseRTP.parseData(rtp_data,data_length);
}

void H264Parser::parseDjiLiveVideoData(const uint8_t *data,const size_t data_length) {
    //LOGD("H264Parser::parseDjiLiveVideoData %d",data_length);
    mParseRAW.parseDjiLiveVideoData(data,data_length);
}

void H264Parser::setLimitFPS(int maxFPS) {
    this->maxFPS=maxFPS;
}

void H264Parser::newNaluExtracted(const NALU& nalu) {
    using namespace std::chrono;
    //LOGD("H264Parser::newNaluExtracted");
    if(onNewNALU!= nullptr){
        onNewNALU(nalu);
    }
    nParsedNALUs++;
    const bool sps_or_pps=nalu.isSPS() || nalu.isPPS();
    if(sps_or_pps){
        nParsedKeyFrames++;
    }
    //sps or pps NALUs do not count as frames, as well as AUD
    //E.g. they won't create a frame on the output pipe)
    if(!(sps_or_pps || nalu.get_nal_unit_type()==NAL_UNIT_TYPE_AUD)){
        mFrameLimiter.limitFps(maxFPS);
    }
}



void H264Parser::debugSequenceNumbers(const uint32_t seqNr) {
    sequenceNumbers.push_back(seqNr);
    if(sequenceNumbers.size()>32) {
        std::stringstream ss;
        for (const auto seqnr:sequenceNumbers) {
            ss << ((int) seqnr) << " ";
        }
        int nOutOufOrderBroken=0;
        // Does not take out of order into account
        int nLostPackets=0;
        for (size_t i = 0; i < sequenceNumbers.size() - 1; i++) {
            if (((sequenceNumbers.at(i)) >= sequenceNumbers.at(i + 1))) {
                nOutOufOrderBroken++;
            }
            if ((sequenceNumbers.at(i) + 1) != sequenceNumbers.at(i + 1)) {
                nLostPackets++;
            }
        }
        MLOGD<<"Seq numbers. LostPackets: "<<nLostPackets<<"  nOutOufOrderBroken: "<<nOutOufOrderBroken<<" values : "<<ss.str();
        sequenceNumbers.resize(0);
    }
}

void H264Parser::parseCustom(const uint8_t *data, const std::size_t data_length) {
    //
    avgUDPPacketSize.add(std::chrono::nanoseconds(data_length));
    if(avgUDPPacketSize.getNSamples()>100){
        MLOGD<<"UDPPacketSize"
        <<" min:"<<StringHelper::memorySizeReadable(avgUDPPacketSize.getMin().count())
        <<" max:"<<StringHelper::memorySizeReadable(avgUDPPacketSize.getMax().count())
        <<" avg:"<<StringHelper::memorySizeReadable(avgUDPPacketSize.getAvg().count());
        avgUDPPacketSize.reset();
    }
    //
    //Custom protocol where N bytes of udp packet is the sequence number
    CustomUdpPacket customUdpPacket{0,&data[sizeof(uint32_t)],data_length-sizeof(uint32_t)};
    memcpy(&customUdpPacket.sequenceNumber,data,sizeof(uint32_t));
    debugSequenceNumbers(customUdpPacket.sequenceNumber);
    // First packet to ever arrive
    if(lastForwardedSequenceNr==-1){
        parse_raw_h264_stream(customUdpPacket.data,customUdpPacket.dataLength);
        lastForwardedSequenceNr=customUdpPacket.sequenceNumber;
        return;
    }
    //duplicate or old packet
    if(customUdpPacket.sequenceNumber<=lastForwardedSequenceNr){
        // If there was a change of sequence numbers restart after N packets
        droppedPacketsSinceLastForwardedPacket++;
        if(droppedPacketsSinceLastForwardedPacket>100){
            lastForwardedSequenceNr=-1;
            droppedPacketsSinceLastForwardedPacket=0;
        }
        return;
    }else{
        parse_raw_h264_stream(customUdpPacket.data,customUdpPacket.dataLength);
        lastForwardedSequenceNr=customUdpPacket.sequenceNumber;
        droppedPacketsSinceLastForwardedPacket=0;
    }
    //std::vector<uint8_t> tmp={customUdpPacket.data,customUdpPacket.data+customUdpPacket.dataLength};
    /*XPacket xPacket{customUdpPacket.sequenceNumber,{customUdpPacket.data,customUdpPacket.data+customUdpPacket.dataLength}};
    bufferedPackets.push_back(std::move(xPacket));
    if(bufferedPackets.size()>5){
        bufferedPackets.pop_front();
    }*/

    /*auto tmp2=std::make_pair(customUdpPacket.sequenceNumber, tmp);
    dataMap.insert(tmp2);
    if(dataMap.size()>5){
        dataMap::iterator it;
        for(auto it = dataMap.begin(); it != dataMap.end(); ++it) {
            keys.push_back(it->first);
        }
        auto smallestKey=std::min_element(keys.begin(), keys.end());
        dataMap.erase(smallestKey)
    }*/
    //
}