//
// Created by geier on 12/04/2020.
//

#ifndef FPV_VR_OS_ORIGINDATA_H
#define FPV_VR_OS_ORIGINDATA_H

#include <stdio.h>
#include <stdbool.h>

/*
 * Only set by LTM and MAVLINK, optionally set by android gps
 */
typedef struct {
    //float Altitude_m;
    double Latitude_dDeg;
    double Longitude_dDeg;
    bool hasBeenSet;
    bool writeByTelemetryProtocol; //false if we use the ANDROID GPS for the origin
} OriginData;

#ifdef __cplusplus
#include <array>
//For ground recording
namespace RawOriginData{
    typedef std::array<double,3> Packet;
    static Packet fromRawData(const uint8_t* data,size_t len){
        Packet origin{0};
        if(len==origin.size()*sizeof(origin[0])){
            memcpy(origin.data(),data,len);
        }
        return origin;
    }
    static std::array<uint8_t,3*sizeof(double)> toRawData(const Packet& packet){
        std::array<uint8_t,3*sizeof(double)> ret;
        memcpy(ret.data(),packet.data(),3*sizeof(double));
        return ret;
    }
}

#endif

#endif //FPV_VR_OS_ORIGINDATA_H
