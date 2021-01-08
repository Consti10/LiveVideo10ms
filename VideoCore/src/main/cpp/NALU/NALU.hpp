//
// Created by Constantin on 2/6/2019.
//

#ifndef LIVE_VIDEO_10MS_ANDROID_NALU_H
#define LIVE_VIDEO_10MS_ANDROID_NALU_H

//https://github.com/Dash-Industry-Forum/Conformance-and-reference-source/blob/master/conformance/TSValidator/h264bitstream/h264_stream.h


#include <string>
#include <chrono>
#include <sstream>
#include <array>
#include <vector>
#include <h264_stream.h>
#include <android/log.h>
#include <AndroidLogger.hpp>
#include <variant>
#include <optional>
#include <StringHelper.hpp>

#include "H26X.hpp"
#include "NALUnitType.hpp"

#include <h265_sps_parser.h>
#include <h265_vps_parser.h>


/**
 * A NALU either contains H264 data (default) or H265 data
 * NOTE: Only when copy constructing a NALU it owns the data, else it only holds a data pointer (that might get overwritten by the parser if you hold onto a NALU)
 * Also, H264 and H265 is slightly different
 * The constructor of the NALU does some really basic validation - make sure the parser never produces a NALU where this validation would fail
 */
class NALU{
private:
    static uint8_t* makeOwnedCopy(const uint8_t* data,size_t data_len){
        auto ret=new uint8_t[data_len];
        memcpy(ret,data,data_len);
        return ret;
    }
public:
    // test video white iceland: Max 1024*117. Video might not be decodable if its NALU buffers size exceed the limit
    // But a buffer size of 1MB accounts for 60fps video of up to 60MB/s or 480 Mbit/s. That should be plenty !
    static constexpr const auto NALU_MAXLEN=1024*1024;
    // Application should re-use NALU_BUFFER to avoid memory allocations
    using NALU_BUFFER=std::array<uint8_t,NALU_MAXLEN>;
    // Copy constructor allocates new buffer for data (heavy)
    NALU(const NALU& nalu):
    ownedData(std::vector<uint8_t>(nalu.getData(),nalu.getData()+nalu.getSize())),
    data(ownedData->data()),data_len(nalu.getSize()),creationTime(nalu.creationTime),IS_H265_PACKET(nalu.IS_H265_PACKET){
        //MLOGD<<"NALU copy constructor";
    }
    // Default constructor does not allocate a new buffer,only stores some pointer (light)
    NALU(const NALU_BUFFER& data1,const size_t data_length,const bool IS_H265_PACKET1=false,const std::chrono::steady_clock::time_point creationTime=std::chrono::steady_clock::now()):
            data(data1.data()),data_len(data_length),creationTime{creationTime},IS_H265_PACKET(IS_H265_PACKET1){
        // Validate correctness of NALU (make sure parser never forwards NALUs where this assertion fails)
        assert(hasValidPrefix());
        assert(getSize()>=getMinimumNaluSize(IS_H265_PACKET1));
    };
    // tmp
    NALU(const uint8_t* data1,size_t data_len1,const bool IS_H265_PACKET1=false,const std::chrono::steady_clock::time_point creationTime=std::chrono::steady_clock::now()):
            data(data1),data_len(data_len1),creationTime{creationTime},IS_H265_PACKET(IS_H265_PACKET1)
    {
        assert(hasValidPrefix());
        assert(getSize()>=getMinimumNaluSize(IS_H265_PACKET1));
    }
    ~NALU()= default;
private:
    // With the default constructor a NALU does not own its memory. This saves us one memcpy. However, storing a NALU after the lifetime of the
    // Non-owned memory expired is also needed in some places, so the copy-constructor creates a copy of the non-owned data and stores it in a optional buffer
    // WARNING: Order is important here (Initializer list). Declare before data pointer
    const std::optional<std::vector<uint8_t>> ownedData={};
    const uint8_t* data;
    const size_t data_len;
public:
    const bool IS_H265_PACKET;
    // creation time is used to measure latency
    const std::chrono::steady_clock::time_point creationTime;
public:
    // returns true if starts with 0001, false otherwise
    bool hasValidPrefix()const{
        return data[0]==0 && data[1]==0 &&data[2]==0 &&data[3]==1;
    }
    static std::size_t getMinimumNaluSize(const bool isH265){
        // 4 bytes prefix, 1 byte header for h264, 2 byte header for h265
        return isH265 ? 6 : 5;
    }
public:
    // pointer to the NALU data with 0001 prefix
    const uint8_t* getData()const{
        return data;
    }
    // size of the NALU data with 0001 prefix
    const size_t getSize()const{
        return data_len;
    }
    //pointer to the NALU data without 0001 prefix
    const uint8_t* getDataWithoutPrefix()const{
        return &getData()[4];
    }
    //size of the NALU data without 0001 prefix
    const ssize_t getDataSizeWithoutPrefix()const{
        return getSize()-4;
    }
    bool isSPS()const{
        if(IS_H265_PACKET){
            return get_nal_unit_type()==NALUnitType::H265::NAL_UNIT_SPS;
        }
        return (get_nal_unit_type() == NAL_UNIT_TYPE_SPS);
    }
    bool isPPS()const{
        if(IS_H265_PACKET){
            return get_nal_unit_type()==NALUnitType::H265::NAL_UNIT_PPS;
        }
        return (get_nal_unit_type() == NAL_UNIT_TYPE_PPS);
    }
    // VPS NALUs are only possible in H265
    bool isVPS()const{
        assert(IS_H265_PACKET);
        return get_nal_unit_type()==NALUnitType::H265::NAL_UNIT_VPS;
    }
    bool isAUD()const{
        if(IS_H265_PACKET){
            return get_nal_unit_type()==NALUnitType::H265:: NAL_UNIT_ACCESS_UNIT_DELIMITER;
        }
        return (get_nal_unit_type() == NAL_UNIT_TYPE_AUD);
    }
    // return the nal unit type (quick)
    int get_nal_unit_type()const{
        if(IS_H265_PACKET){
            return (getData()[4] & 0x7E)>>1;
        }
        const auto lol=(H264::nal_unit_header_t*)getDataWithoutPrefix();
        assert(lol->nal_unit_type==(getData()[4]&0x1f));
        return getData()[4]&0x1f;
    }
    std::string get_nal_name()const{
        if(IS_H265_PACKET){
            return NALUnitType::H265::unitTypeName(get_nal_unit_type());
        }
        return NALUnitType::H264::unitTypeName(get_nal_unit_type());
    }
    // For debugging, return the whole NALU data as a big string for logging
    std::string dataAsString()const{
        return StringHelper::vectorAsString(std::vector<uint8_t>(getData(),getData()+getSize()));
    }
    void debug()const{
        if(IS_H265_PACKET){
            if(isSPS()){
                auto sps=h265nal::H265SpsParser::ParseSps(&getData()[6],getSize()-6);
                if(sps!=absl::nullopt){
                    MLOGD<<"SPS:"<<sps->getPicSizeInCtbsY()<<" NN:"<<sps->pic_width_in_luma_samples<<","<<sps->pic_height_in_luma_samples;
                }else{
                    MLOGD<<"SPS parse error";
                }
            }else if(isVPS()){
                auto vps=h265nal::H265VpsParser::ParseVps(&getData()[6],getSize()-6);
                if(vps!=absl::nullopt){
                    MLOGD<<"VPS:"<<vps->vps_num_units_in_tick;
                }else{
                    MLOGD<<"VPS parse error";
                }
            }
            return;
        }else{
            if(isSPS()){
                auto sps=H264::SPS(getData(),getSize());
                MLOGD<<"SPS:"<<sps.asString();
                //MLOGD<<"Has vui"<<sps.parsed.vui_parameters_present_flag;
                //MLOGD<<"SPS Latency:"<<H264Stream::latencyAffectingValues(&sps.parsed);
                MLOGD<<"XSPS"<<StringHelper::vectorAsString(std::vector<uint8_t>(getData(),getData()+getSize()));

                //H264::testSPSConversion(getData(),getSize());
                //RBSPHelper::test_unescape_escape(std::vector<uint8_t>(&getData()[5],&getData()[5]+getSize()-5));
            }else if(isPPS()){
                auto pps=H264::PPS(getData(),getSize());
                MLOGD<<"PPS:"<<pps.asString();
                MLOGD<<"XPPS"<<StringHelper::vectorAsString(std::vector<uint8_t>(getData(),getData()+getSize()));
            }else if(isAUD()){
                MLOGD<<"AUD:"<<StringHelper::vectorAsString(std::vector<uint8_t>(getData(),getData()+getSize()));
            }else{
                MLOGD<<get_nal_name();
                if(get_nal_unit_type()==NAL_UNIT_TYPE_CODED_SLICE_IDR || get_nal_unit_type()==NAL_UNIT_TYPE_CODED_SLICE_NON_IDR){
                    const auto sliceHeader=getSliceHeaderH264();
                    MLOGD<<"Slice header:"<<sliceHeader.asString();
                }
            }
        }
    }

    //Returns video width and height if the NALU is an SPS
    std::array<int,2> getVideoWidthHeightSPS()const{
        assert(isSPS());
        if(IS_H265_PACKET){
            return {1280,720};
            auto sps=h265nal::H265SpsParser::ParseSps(&getData()[6],getSize()-6);
            if(sps!=absl::nullopt){
                std::array<int,2> ret={(int)sps->pic_width_in_luma_samples,(int)sps->pic_height_in_luma_samples};
                return ret;
            }
            MLOGE<<"Couldn't parse h265 sps";
            return {640,480};
        }else{
            const auto sps=H264::SPS(getData(),getSize());
            return sps.getWidthHeightPx();
        }
    }
     const H264::slice_header_t& getSliceHeaderH264()const{
        assert(!IS_H265_PACKET);
        assert(get_nal_unit_type()==NAL_UNIT_TYPE_CODED_SLICE_IDR || get_nal_unit_type()==NAL_UNIT_TYPE_CODED_SLICE_NON_IDR);
        assert(getSize()>6);
        const H264::slice_header_t* ret1=static_cast<const H264::slice_header_t*>((const void*)&getDataWithoutPrefix()[1]);
        return *ret1;
    }
    //
    static NALU createExampleH264_AUD(){
        return NALU(H264::EXAMPLE_AUD.data(),H264::EXAMPLE_AUD.size());
    }
};

typedef std::function<void(const NALU& nalu)> NALU_DATA_CALLBACK;


#endif //LIVE_VIDEO_10MS_ANDROID_NALU_H
