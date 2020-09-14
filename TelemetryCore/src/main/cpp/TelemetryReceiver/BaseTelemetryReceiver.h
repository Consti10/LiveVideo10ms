//
// Created by geier on 14/09/2020.
//

#ifndef LIVEVIDEO10MS_BASETELEMETRYRECEIVER_H
#define LIVEVIDEO10MS_BASETELEMETRYRECEIVER_H

#include <string>

class MTelemetryValue{
public:
    std::wstring prefix=std::wstring();
    std::wstring prefixIcon=std::wstring();
    float prefixScale=0.83f;
    std::wstring value=L"";
    double valueNotAsString;
    std::wstring metric=L"";
    int warning=0; //0==okay 1==orange 2==red and -1==green
    unsigned long getLength()const{
        return prefix.length()+value.length()+metric.length();
    }
    // Not every telemetry value has an Icon. If no icon was generated yet, a simple string
    // is used instead (example 'Batt' instead of Battery icon)
    bool hasIcon()const{
        return (!prefixIcon.empty());
    }
    std::wstring getPrefix()const{
        return hasIcon() ? prefixIcon : prefix;
    }
};

class BaseTelemetryReceiver{
public:
    BaseTelemetryReceiver()=default;
    enum TelemetryValueIndex{
        DECODER_FPS,
        DECODER_BITRATE,
        DECODER_LATENCY_DETAILED,
        DECODER_LATENCY_SUM,
        OPENGL_FPS,
        FLIGHT_TIME,
        //
        RX_1,
        BATT_VOLTAGE,
        BATT_CURRENT,
        BATT_USED_CAPACITY,
        BATT_PERCENTAGE,
        HOME_DISTANCE,
        VS,
        HS_GROUND,
        HS_AIR,
        LONGITUDE,
        LATITUDE,
        ALTITUDE_GPS,
        ALTITUDE_BARO,
        SATS_IN_USE,
        FLIGHT_STATUS_MAV_ONLY,
        //
        EZWB_DOWNLINK_VIDEO_RSSI,
        EZWB_DOWNLINK_VIDEO_RSSI2,
        EZWB_UPLINK_RC_RSSI,
        EZWB_UPLINK_RC_BLOCKS,
        EZWB_STATUS_AIR,
        EZWB_STATUS_GROUND,
        EZWB_BLOCKS,
        EZWB_RSSI_ADAPTER0,
        EZWB_RSSI_ADAPTER1,
        EZWB_RSSI_ADAPTER2,
        EZWB_RSSI_ADAPTER3,
        //EZWB_RSSI_ADAPTER4,
        //EZWB_RSSI_ADAPTER5,
        XXX
    };
    virtual MTelemetryValue getTelemetryValue(TelemetryValueIndex index)const= 0;
};
#endif //LIVEVIDEO10MS_BASETELEMETRYRECEIVER_H
