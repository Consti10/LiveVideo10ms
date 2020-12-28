//
// Created by Constantin on 24.01.2018.
//
#include "H26XParser.h"
#include <cstring>
#include <android/log.h>
#include <endian.h>
#include <chrono>
#include <thread>
#include <StringHelper.hpp>


H26XParser::H26XParser(NALU_DATA_CALLBACK onNewNALU):
        onNewNALU(std::move(onNewNALU)),
        mParseRAW(std::bind(&H26XParser::newNaluExtracted, this, std::placeholders::_1)),
        mDecodeRTP(std::bind(&H26XParser::newNaluExtracted, this, std::placeholders::_1)){
}

void H26XParser::reset(){
    mParseRAW.reset();
    mDecodeRTP.reset();
    nParsedNALUs=0;
    nParsedKeyFrames=0;
    setLimitFPS(-1);
    lastForwardedSequenceNr=-1;
    droppedPacketsSinceLastForwardedPacket=0;
}

void H26XParser::parse_raw_h264_stream(const uint8_t *data, const size_t data_length) {
    mParseRAW.parseData(data,data_length);
}

void H26XParser::parse_raw_h265_stream(const uint8_t *data, const size_t data_length) {
    mParseRAW.parseData(data,data_length,true);
}

void H26XParser::parse_rtp_h264_stream(const uint8_t *rtp_data, const size_t data_length) {
    mDecodeRTP.parseRTPH264toNALU(rtp_data, data_length);
}

void H26XParser::parse_rtp_h265_stream(const uint8_t *rtp_data, const size_t data_length) {
    mDecodeRTP.parseRTPH265toNALU(rtp_data, data_length);
}

void H26XParser::parseDjiLiveVideoDataH264(const uint8_t *data, const size_t data_length) {
    mParseRAW.parseDjiLiveVideoDataH264(data,data_length);
}

void H26XParser::parseJetsonRawSlicedH264(const uint8_t *data, const size_t data_length) {
    mParseRAW.parseJetsonRawSlicedH264(data,data_length);
}

void H26XParser::setLimitFPS(int maxFPS1) {
    this->maxFPS=maxFPS1;
}

void H26XParser::newNaluExtracted(const NALU& nalu) {
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
    const bool isNotARealFrame=nalu.IS_H265_PACKET ? (nalu.isSPS() || nalu.isPPS() || nalu.isVPS() || nalu.isAUD()) : (nalu.isSPS() || nalu.isPPS() || nalu.isAUD());
    if(!(isNotARealFrame)){
        mFrameLimiter.limitFps(maxFPS);
    }
}

void H26XParser::debugSequenceNumbers(const uint32_t seqNr) {
    sequenceNumbers.push_back(seqNr);
    if(sequenceNumbers.size()>32) {
        int nOutOufOrderBroken=0;
        // Does not take out of order into account
        int nLostPackets=0;
        AvgCalculator avgDeltaBetweenSeqNrs;
        std::vector<int> diffs{};
        for (size_t i = 0; i < sequenceNumbers.size() - 1; i++) {
            if (((sequenceNumbers.at(i)) >= sequenceNumbers.at(i + 1))) {
                nOutOufOrderBroken++;
            }
            if ((sequenceNumbers.at(i) + 1) != sequenceNumbers.at(i + 1)) {
                nLostPackets++;
            }
            const int diff=sequenceNumbers.at(i+1)-sequenceNumbers.at(i);
            diffs.push_back(diff);
            avgDeltaBetweenSeqNrs.add(std::chrono::nanoseconds(diff));
        }
        const bool lostOrBroken=nOutOufOrderBroken>0 || nLostPackets>0;
        MLOGD<<"Seq numbers.LOB:"<<lostOrBroken<<" LostPackets: "<<nLostPackets<<"  nOutOfOrderBroken: "<<nOutOufOrderBroken<<" values : "<<
        //StringHelper::vectorAsString(sequenceNumbers)<<"\n"<<
        StringHelper::vectorAsString(diffs);
        /*MLOGD<<"SeqNrDiffs "<<lostOrBroken
             <<" min:"<<StringHelper::memorySizeReadable(avgDeltaBetweenSeqNrs.getMin().count())
             <<" max:"<<StringHelper::memorySizeReadable(avgDeltaBetweenSeqNrs.getMax().count())
             <<" avg:"<<StringHelper::memorySizeReadable(avgDeltaBetweenSeqNrs.getAvg().count());*/

        sequenceNumbers.resize(0);
    }
}

void H26XParser::parseCustom(const uint8_t *data, const std::size_t data_length) {
    //
    avgUDPPacketSize.add(data_length);
    if(avgUDPPacketSize.getNSamples()>100){
        MLOGD<<"UDPPacketSize "<<avgUDPPacketSize.getAvgReadable();
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
    // duplicate or old packet
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
}

void H26XParser::parseCustomRTPinsideFEC(const uint8_t *data, const std::size_t data_len) {

    mFECDecoder.add_block(data, data_len);
    //std::vector<uint8_t> obuf;
    //obuf.reserve(1024*1024);
    for (std::shared_ptr<FECBlock> sblk = mFECDecoder.get_block(); sblk; sblk = mFECDecoder.get_block()) {
        // One block should be equal to one rtp packet
        const uint8_t* sblkData=sblk->data();
        const size_t sblkDataLength=sblk->data_length();
        if(sblkDataLength>10){
            MLOGD<<"Parsing rtp "<<sblkDataLength;
            const auto* rtp_header=(rtp_header_t*)sblkData;
            const auto seqNr=rtp_header->getSequence();
            debugSequenceNumbers(seqNr);
            mDecodeRTP.parseRTPH264toNALU(sblkData, sblkDataLength);
        }else{
            MLOGD<<"Weird packet"<<sblkDataLength;
        }

        //std::copy(sblk->data(), sblk->data() + sblk->data_length(),
        //          std::back_inserter(obuf));
    }
    /*if(obuf.size()>0){
        MLOGD<<"Got rtp packet";
        //parse_raw_h264_stream(obuf.data(),obuf.size());
        mDecodeRTP.parseRTPtoNALU(obuf.data(),obuf.size());
    }*/
}

