//
// Created by Constantin on 10.10.2017.
//

#include "TelemetryReceiver.h"
#include <cstdint>
#include <jni.h>
#include "../IDT.hpp"
#include <PositionHelper.hpp>
#include <StringHelper.hpp>
#include <WFBBackwardsCompatibility.h>

#include <locale>
#include <codecvt>
#include <android/asset_manager_jni.h>
#include <array>
#include <AndroidThreadPrioValues.hpp>
#include <NDKThreadHelper.hpp>
#include <AndroidLogger.hpp>
#include <NDKHelper.hpp>

extern "C"{
#include "../parser_c/ltm.h"
#include "../parser_c/frsky.h"
#include "../parser_c/mavlink2.h"
#include "../parser_c/smartport.h"
}

int TelemetryReceiver::getTelemetryPort(const SharedPreferences &settingsN, int T_Protocol) {
    int port=5700;
    switch (T_Protocol){
        case TelemetryReceiver::NONE:break;
        case TelemetryReceiver::LTM :port=settingsN.getInt(IDT::T_LTMPort);break;
        case TelemetryReceiver::MAVLINK:port=settingsN.getInt(IDT::T_MAVLINKPort);break;
        case TelemetryReceiver::SMARTPORT:port=settingsN.getInt(IDT::T_SMARTPORTPort);break;
        case TelemetryReceiver::FRSKY:port=settingsN.getInt(IDT::T_FRSKYPort);break;
        default:break;
    }
    return port;
}

TelemetryReceiver::TelemetryReceiver(JNIEnv* env,std::string DIR,GroundRecorderFPV* externalGroundRecorder,FileReader* externalFileReader,CONNECTED_SYSTEM connectedSystem1):
        GROUND_RECORDING_DIRECTORY(std::move(DIR)),
        mGroundRecorder((externalGroundRecorder== nullptr) ?( * new GroundRecorderFPV(DIR)) : *externalGroundRecorder),
        mFileReceiver((externalFileReader== nullptr) ?( * new FileReader(1024)) : *externalFileReader),
        isExternalFileReceiver(externalFileReader!= nullptr),
        connectedSystem(connectedSystem1){
    env->GetJavaVM(&javaVm);
    FileReader::RAW_DATA_CALLBACK callback=[this](const uint8_t* d,std::size_t len,GroundRecorderFPV::PACKET_TYPE packetType) {
        switch(packetType){
            case GroundRecorderFPV::PACKET_TYPE_VIDEO_H264:break;
            case GroundRecorderFPV::PACKET_TYPE_TELEMETRY_LTM:
                T_Protocol=LTM;
                this->onUAVTelemetryDataReceived(d,len);
                MLOGD<<"LTM";
                break;
            case GroundRecorderFPV::PACKET_TYPE_TELEMETRY_MAVLINK:
                T_Protocol=MAVLINK;
                this->onUAVTelemetryDataReceived(d,len);
                break;
            case GroundRecorderFPV::PACKET_TYPE_TELEMETRY_FRSKY:
                T_Protocol=FRSKY;
                this->onUAVTelemetryDataReceived(d,len);
                break;
            case GroundRecorderFPV::PACKET_TYPE_TELEMETRY_SMARTPORT:
                T_Protocol=SMARTPORT;
                this->onUAVTelemetryDataReceived(d,len);
                break;
            case GroundRecorderFPV::PACKET_TYPE_TELEMETRY_EZWB:
                this->onEZWBStatusDataReceived(d,len);
                break;
            case GroundRecorderFPV::PACKET_TYPE_TELEMETRY_ANDROD_GPS:{
                RawOriginData::Packet packet=RawOriginData::fromRawData(d,len);
                this->setHomeAndroid(packet[0], packet[1], packet[2]);
                originData.hasBeenSet=true;
            }break;
            default:break;
        }
    };
    mFileReceiver.setCallBack(1,callback);
}

void TelemetryReceiver::updateSettings(JNIEnv *env,jobject context) {
    SharedPreferences settingsN(env,context,"pref_telemetry");
    T_Protocol=static_cast<PROTOCOL_OPTIONS >(settingsN.getInt(IDT::T_PROTOCOL,1));
    T_Port=getTelemetryPort(settingsN,T_Protocol);
    EZWBS_Protocol=static_cast<EZWB_STATUS_PROTOCOL>(settingsN.getInt(IDT::EZWBS_Protocol));
    EZWBS_Port=settingsN.getInt(IDT::EZWBS_Port);
    MAVLINK_FLIGHTMODE_QUADCOPTER=settingsN.getBoolean(IDT::T_MAVLINK_FLIGHTMODE_QUADCOPTER);
    BATT_CAPACITY_MAH=settingsN.getInt(IDT::T_BATT_CAPACITY_MAH);
    BATT_CELLS_N=settingsN.getInt(IDT::T_BATT_CELLS_N);
    BATT_CELLS_V_WARNING1_ORANGE=settingsN.getFloat(IDT::T_BATT_CELLS_V_WARNING1_ORANGE);
    BATT_CELLS_V_WARNING2_RED=settingsN.getFloat(IDT::T_BATT_CELLS_V_WARNING2_RED),
            BATT_CAPACITY_MAH_USED_WARNING=settingsN.getInt(IDT::T_BATT_CAPACITY_MAH_USED_WARNING);
    ORIGIN_POSITION_ANDROID=settingsN.getBoolean(IDT::T_ORIGIN_POSITION_ANDROID);
    SOURCE_TYPE=static_cast<SOURCE_TYPE_OPTIONS >(settingsN.getInt(IDT::T_SOURCE));
    ENABLE_GROUND_RECORDING=settingsN.getBoolean(IDT::T_GROUND_RECORDING);
    T_PLAYBACK_FILENAME=settingsN.getString(IDT::T_PLAYBACK_FILENAME);
    LTM_FOR_INAV=true;
    T_METRIC_SPEED_HORIZONTAL= static_cast<METRIC_SPEED>(settingsN.getInt(IDT::T_METRIC_SPEED_HORIZONTAL));
    T_METRIC_SPEED_VERTICAL= static_cast<METRIC_SPEED>(settingsN.getInt(IDT::T_METRIC_SPEED_VERTICAL),1);
    //
    resetNReceivedTelemetryBytes();
    //std::memset (&uav_td, 0, sizeof(uav_td));
    uav_td.Pitch_Deg=0;
    std::memset (&originData, 0, sizeof(originData));
    originData.Latitude_dDeg=0;
    originData.Longitude_dDeg=0;
    originData.hasBeenSet=false;
    originData.writeByTelemetryProtocol=!ORIGIN_POSITION_ANDROID;
    std::memset (&wifibroadcastTelemetryData, 0, sizeof(wifibroadcastTelemetryData));
    for (auto &i : wifibroadcastTelemetryData.adapter) {
        i.current_signal_dbm=-99;
    }
    wifibroadcastTelemetryData.current_signal_joystick_uplink=-99;
    wifibroadcastTelemetryData.current_signal_telemetry_uplink=-99;
    wifibroadcastTelemetryData.wifi_adapter_cnt=1;
}

void TelemetryReceiver::startReceiving(JNIEnv *env,jobject context) {
    AAssetManager* assetManager=NDKHelper::getAssetManagerFromContext2(env,context);
    assert(mTelemetryDataReceiver.get()==nullptr);
    assert(mEZWBDataReceiver.get()== nullptr);
    updateSettings(env,context);
    MLOGD<<"Start receiving "<<SOURCE_TYPE;
    switch(SOURCE_TYPE){
        case UDP:{
            if(ENABLE_GROUND_RECORDING){
                mGroundRecorder.start();
            }
            if(T_Protocol!=TelemetryReceiver::NONE ){
                UDPReceiver::DATA_CALLBACK f= [=](const uint8_t data[],size_t data_length) {
                    this->onUAVTelemetryDataReceived(data,data_length);
                };
                mTelemetryDataReceiver=std::make_unique<UDPReceiver>(javaVm,T_Port,"T_UDP_R",FPV_VR_PRIORITY::CPU_PRIORITY_UDPRECEIVER_TELEMETRY,f);
                mTelemetryDataReceiver->startReceiving();
            }
            //ezWB is sending telemetry packets 128 bytes big. To speed up performance, i have a buffer  of 1024 bytes on the receiving end, though. This
            //should not add any additional latency
            if(EZWBS_Protocol!=DISABLED){
                UDPReceiver::DATA_CALLBACK f2 = [=](const uint8_t data[],size_t data_length) {
                    this->onEZWBStatusDataReceived(data, data_length);
                };
                mEZWBDataReceiver=std::make_unique<UDPReceiver>(javaVm,EZWBS_Port,"T_UDP_R2",FPV_VR_PRIORITY::CPU_PRIORITY_UDPRECEIVER_TELEMETRY,f2);
                mEZWBDataReceiver->startReceiving();
            }
        }break;
        case FILE:
        case ASSETS:{
            // If using external file receiver, start / stop is handled by the video player
            if(!isExternalFileReceiver){
                const bool useAsset=SOURCE_TYPE==ASSETS;
                const std::string filename = useAsset ? "telemetry/testlog."+getProtocolAsString() :T_PLAYBACK_FILENAME;
                mFileReceiver.startReading(useAsset ? assetManager : nullptr,filename);
            }
        }break;
        case EXTERNAL_DJI:{
            // values are set via the setXXX callbacks
        }break;
    }
}

void TelemetryReceiver::stopReceiving(JNIEnv* env,jobject androidContext) {
    if(mTelemetryDataReceiver){
        mTelemetryDataReceiver->stopReceiving();
        mTelemetryDataReceiver.reset();
    }
    if(mEZWBDataReceiver){
        mEZWBDataReceiver->stopReceiving();
        mEZWBDataReceiver.reset();
    }
    mFileReceiver.stopReadingIfStarted();
    mGroundRecorder.stop(env,androidContext);
}

void TelemetryReceiver::onUAVTelemetryDataReceived(const uint8_t data[],size_t data_length){
    switch (T_Protocol){
        case TelemetryReceiver::LTM:
            ltm_read(&uav_td,&originData,data,data_length,LTM_FOR_INAV);
            break;
        case TelemetryReceiver::MAVLINK:
            mavlink_read_v2(&uav_td,&originData,data,data_length);
            break;
        case TelemetryReceiver::SMARTPORT:
            smartport_read(&uav_td,data,data_length);
            break;
        case TelemetryReceiver::FRSKY:
            frsky_read(&uav_td,data,data_length);
            break;
        default:
            MLOGE<<"TelR "<<T_Protocol;
            assert(false);
            break;
    }
    nTelemetryBytes+=data_length;
    if(T_Protocol!=TelemetryReceiver::MAVLINK){
        uav_td.BatteryPack_P=(int8_t)(uav_td.BatteryPack_mAh/BATT_CAPACITY_MAH*100.0f);
    }
    mGroundRecorder.writePacketIfStarted(data,data_length,static_cast<uint8_t>(T_Protocol));
}

void TelemetryReceiver::onEZWBStatusDataReceived(const uint8_t *data,const size_t data_length){
    nWIFIBRADCASTBytes+=data_length;
    switch(data_length){
        case WIFIBROADCAST_RX_STATUS_FORWARD_SIZE_BYTES:{
            const auto* struct_pointer= reinterpret_cast<const wifibroadcast_rx_status_forward_t*>(data);
            writeDataBackwardsCompatible(&wifibroadcastTelemetryData,struct_pointer);
            nWIFIBROADCASTParsedPackets++;
        };break;
        case WIFIBROADCAST_RX_STATUS_FORWARD_2_SIZE_BYTES:{
            memcpy(&wifibroadcastTelemetryData,data,WIFIBROADCAST_RX_STATUS_FORWARD_2_SIZE_BYTES);
            nWIFIBROADCASTParsedPackets++;
        };break;
        default:
            MLOGE<<"EZWB status data unknown length "<<data_length;
            nWIFIBRADCASTFailedPackets++;
            break;
    }
    mGroundRecorder.writePacketIfStarted(data,data_length,GroundRecorderFPV::PACKET_TYPE_TELEMETRY_EZWB);
}

int TelemetryReceiver::getNReceivedTelemetryBytes()const {
    return (int)(nTelemetryBytes);
}

void TelemetryReceiver::resetNReceivedTelemetryBytes() {
    nTelemetryBytes=0;
}

void TelemetryReceiver::setHomeAndroid(double latitude, double longitude, double attitude) {
    //When using LTM we also get the home data by the protocol
    originData.Latitude_dDeg=latitude;
    originData.Longitude_dDeg=longitude;
    //originData.Altitude_m=(float)attitude;
    originData.hasBeenSet=true;
    const auto data=RawOriginData::toRawData({latitude,longitude,attitude});
    mGroundRecorder.writePacketIfStarted(data.data(),data.size(),GroundRecorderFPV::PACKET_TYPE_TELEMETRY_ANDROD_GPS);
}

const UAVTelemetryData& TelemetryReceiver::getUAVTelemetryData()const{
    return uav_td;
}

const wifibroadcast_rx_status_forward_t2& TelemetryReceiver::get_ez_wb_forward_data() const{
    return wifibroadcastTelemetryData;
}

const OriginData& TelemetryReceiver::getOriginData() const {
    return originData;
}
long TelemetryReceiver::getNEZWBPacketsParsingFailed()const {
    return nWIFIBRADCASTFailedPackets;
}

int TelemetryReceiver::getBestDbm()const{
    //Taken from ez-wifibroadcast OSD.
    int cnt =  wifibroadcastTelemetryData.wifi_adapter_cnt;
    cnt = (cnt<=6) ? cnt : 6;
    int best_dbm = -100;
    // find out which card has best signal
    for(int i=0; i<cnt; i++) {
        const int curr_dbm=wifibroadcastTelemetryData.adapter[i].current_signal_dbm;
        if (best_dbm < curr_dbm) best_dbm = curr_dbm;
    }
    return best_dbm;
}

void TelemetryReceiver::setDecodingInfo(float currentFPS, float currentKiloBitsPerSecond,float avgParsingTime_ms,float avgWaitForInputBTime_ms,float avgDecodingTime_ms) {
    appOSDData.decoder_fps=currentFPS;
    appOSDData.decoder_bitrate_kbits=currentKiloBitsPerSecond;
    appOSDData.avgParsingTime_ms=avgParsingTime_ms;
    appOSDData.avgWaitForInputBTime_ms=avgWaitForInputBTime_ms;
    appOSDData.avgDecodingTime_ms=avgDecodingTime_ms;
}

void TelemetryReceiver::setOpenGLFPS(float fps) {
    appOSDData.opengl_fps=fps;
}

void TelemetryReceiver::setFlightTime(float timeSeconds) {
    appOSDData.flight_time_seconds=timeSeconds;
}

MTelemetryValue TelemetryReceiver::getTelemetryValue(TelemetryValueIndex index) const {
    MTelemetryValue ret = MTelemetryValue();
    //LOGD("Size %d",(int)sizeof(wifibroadcast_rx_status_forward_t2));
    ret.warning=0;
    switch (index){
        case BATT_VOLTAGE:{
            ret.prefix=L"Batt";
            ret.prefixIcon=ICON_BATTERY;
            ret.prefixScale=1.2f;
            ret.value= StringHelper::doubleToWString(uav_td.BatteryPack_V, 5, 2);
            ret.metric=L"V";
            float w1=BATT_CELLS_V_WARNING1_ORANGE*BATT_CELLS_N;
            float w2=BATT_CELLS_V_WARNING2_RED*BATT_CELLS_N;
            if(uav_td.BatteryPack_V<w1){
                ret.warning=1;
            }if(uav_td.BatteryPack_V<w2){
                ret.warning=2;
            }
        }
            break;
        case BATT_CURRENT:{
            ret.prefix=L"Batt";
            ret.prefixIcon=ICON_BATTERY;
            ret.prefixScale=1.2f;
            ret.value= StringHelper::doubleToWString(uav_td.BatteryPack_A, 5, 2);
            ret.metric=L"A";
        }
            break;
        case BATT_USED_CAPACITY:{
            ret.prefix=L"Batt";
            ret.prefixIcon=ICON_BATTERY;
            ret.prefixScale=1.2f;
            float val=uav_td.BatteryPack_mAh;
            ret.value= StringHelper::intToWString((int) val, 5);
            ret.metric=L"mAh";
            if(val>BATT_CAPACITY_MAH_USED_WARNING){
                ret.warning=2;
            }
        }
            break;
        case BATT_PERCENTAGE:{
            ret.prefix=L"Batt";
            ret.prefixIcon=ICON_BATTERY;
            ret.prefixScale=1.2f;
            float perc=uav_td.BatteryPack_P;
            ret.value= StringHelper::intToWString((int) std::round(perc), 3);
            ret.metric=L"%";
            if(perc<20.0f){
                ret.warning=1;
            }
            if(perc<10){
                ret.warning=2;
            }
        }
            break;
        case ALTITUDE_GPS:{
            ret.prefix=L"Alt(G)";
            ret.value= StringHelper::doubleToWString(uav_td.AltitudeGPS_m, 5, 2);
            ret.metric=L"m";
        }
            break;
        case ALTITUDE_BARO:{
            ret.prefix=L"Alt(B)";
            ret.value= StringHelper::doubleToWString(uav_td.AltitudeBaro_m, 5, 2);
            ret.metric=L"m";
        }
            break;
        case LONGITUDE:{
            ret.prefix=L"Lon";
            ret.prefixIcon=ICON_LONGITUDE;
            ret.value= StringHelper::doubleToWString(uav_td.Longitude_dDeg, 10, 8);
            ret.metric=L"";
        }
            break;
        case LATITUDE:{
            ret.prefix=L"Lat";
            ret.prefixIcon=ICON_LATITUDE;
            ret.value= StringHelper::doubleToWString(uav_td.Latitude_dDeg, 10, 8);
            ret.metric=L"";
        }
            break;
        case HS_GROUND:{
            ret.prefix=L"HS";
            if(T_METRIC_SPEED_HORIZONTAL==KMH){
                ret.valueNotAsString=uav_td.SpeedGround_KPH;
                ret.value= StringHelper::doubleToWString(uav_td.SpeedGround_KPH, 5, 2);
                ret.metric=L"km/h";
            }else{
                ret.valueNotAsString=uav_td.SpeedGround_KPH*KMH_TO_MS;
                ret.value= StringHelper::doubleToWString(uav_td.SpeedGround_KPH * KMH_TO_MS, 5, 2);
                ret.metric=L"m/s";
            }
        }
            break;
        case HS_AIR:{
            ret.prefix=L"HS";
            if(T_METRIC_SPEED_HORIZONTAL==KMH){
                ret.value= StringHelper::doubleToWString(uav_td.SpeedAir_KPH, 5, 2);
                ret.metric=L"km/h";
            }else{
                ret.value= StringHelper::doubleToWString(uav_td.SpeedAir_KPH * KMH_TO_MS, 5, 2);
                ret.metric=L"m/s";
            }
        }
            break;
        case FLIGHT_TIME:{
            ret.prefix=L"Time";
            float time=appOSDData.flight_time_seconds;
            if(time<60){
                ret.value= StringHelper::intToWString((int) std::round(time), 4);
                ret.metric=L"sec";
            }else{
                ret.value= StringHelper::intToWString((int) std::round(time / 60), 4);
                ret.metric=L"min";
            }
        }
            break;
        case HOME_DISTANCE:{
            ret.prefix=L"Home";
            ret.prefixIcon=ICON_HOME;
            if(!originData.hasBeenSet){
                ret.value=L"No origin";
                ret.metric=L"";
            }else{
                int distanceM=(int)distance_between(uav_td.Latitude_dDeg,uav_td.Longitude_dDeg,originData.Latitude_dDeg,originData.Longitude_dDeg);
                if(distanceM>1000){
                    ret.value= StringHelper::doubleToWString(distanceM / 1000.0, 5, 1);
                    ret.metric=L"km";
                }else{
                    ret.value= StringHelper::intToWString(distanceM, 5);
                    ret.metric=L"m";
                }
            }
        }
            break;
        case DECODER_FPS:{
            ret.prefix=L"Dec";
            ret.value= StringHelper::intToWString((int) std::round(appOSDData.decoder_fps), 4);
            ret.metric=L"fps";
        }
            break;
        case DECODER_BITRATE:{
            ret.prefix=L"Dec";
            const float kbits=appOSDData.decoder_bitrate_kbits;
            //Example: Dec: XXX kb/s with 15 max chars there are 5 left for the number (10=5+5)
            if(kbits>1024){
                float mbits=kbits/1024.0f;
                ret.value= StringHelper::doubleToWString(mbits, 5, 1);
                ret.metric=L"mb/s";
            }else{
                ret.value= StringHelper::doubleToWString(kbits, 5, 1);
                ret.metric=L"kb/s";
            }
        }
            break;
        case OPENGL_FPS:{
            ret.prefix=L"OGL";
            ret.value= StringHelper::intToWString((int) std::round(appOSDData.opengl_fps), 4);
            ret.metric=L"fps";
        }
            break;
        case EZWB_DOWNLINK_VIDEO_RSSI:{
            if(connectedSystem==DJI){
                ret.prefix=L"DJI";
                ret.value= StringHelper::intToWString(uav_td.DJI_linkQualityDown_P, 5);
                ret.metric=L"%";
            }else{
                ret.prefix=L"ezWB";
                ret.value= StringHelper::intToWString(getBestDbm(), 5);
                ret.metric=L"dBm";
            }
        }
            break;
        case EZWB_DOWNLINK_VIDEO_RSSI2:{
            ret.prefix=L"";
            ret.value=L"X";
            ret.metric=L"x";
        }
            break;
        case RX_1:{
            ret.prefix=L"RX1";
            ret.value= StringHelper::intToWString((int) uav_td.RSSI1_Percentage_dBm, 4);
            if(T_Protocol==TelemetryReceiver::MAVLINK){
                ret.metric=L"%";
            }else{
                ret.metric=L"dBm";
            }
        }
            break;
        case SATS_IN_USE:{
            ret.prefix=L"Sat";
            ret.prefixIcon=ICON_SATELITE;
            ret.value= StringHelper::intToWString(uav_td.SatsInUse, 3);
            ret.metric=L"";
        }
            break;
        case VS:{
            ret.prefix=L"VS";
            if(T_METRIC_SPEED_VERTICAL==KMH){
                ret.value= StringHelper::doubleToWString(uav_td.SpeedClimb_KPH, 5, 2);
                ret.metric=L"km/h";
            }else{
                ret.value= StringHelper::doubleToWString(uav_td.SpeedClimb_KPH * KMH_TO_MS, 5, 2);
                ret.metric=L"m/s";
            }
        }
            break;
        case DECODER_LATENCY_DETAILED:{
            ret.prefix=L"Dec";
            ret.value= StringHelper::doubleToWString(appOSDData.avgParsingTime_ms, 2, 1) + L"," +
                       StringHelper::doubleToWString(appOSDData.avgWaitForInputBTime_ms, 2, 1) + L"," +
                       StringHelper::doubleToWString(appOSDData.avgDecodingTime_ms, 2, 1);
            ret.metric=L"ms";
        }
            break;
        case DECODER_LATENCY_SUM:{
            ret.prefix=L"Dec";
            float total=appOSDData.avgParsingTime_ms+appOSDData.avgWaitForInputBTime_ms+appOSDData.avgDecodingTime_ms;
            ret.value= StringHelper::doubleToWString(total, 4, 2);
            ret.metric=L"ms";
        }
            break;
        case FLIGHT_STATUS_MAV_ONLY:{
            if(T_Protocol==TelemetryReceiver::MAVLINK){
                ret.value= TelemetryHelper::getMAVLINKFlightModeAsWString(
                        MAVLINK_FLIGHTMODE_QUADCOPTER, uav_td.FlightMode_MAVLINK,
                        uav_td.FlightMode_MAVLINK_armed);
            }else{
                ret.value=L"MAV only";
            }
            ret.metric=L"";
        }
            break;
        case EZWB_UPLINK_RC_RSSI:{
            if(connectedSystem==DJI){
                ret.value= StringHelper::intToWString(uav_td.DJI_linkQualityUp_P, 5);
                ret.metric=L"%";
            }else{
                ret.value= StringHelper::intToWString(
                        wifibroadcastTelemetryData.current_signal_joystick_uplink, 5);
                ret.metric=L"dBm";
            }
        }
            break;
        case EZWB_UPLINK_RC_BLOCKS:{
            std::wstring s1 = StringHelper::intToWString(
                    (int) wifibroadcastTelemetryData.lost_packet_cnt_rc, 6);
            std::wstring s2 = StringHelper::intToWString(
                    (int) wifibroadcastTelemetryData.lost_packet_cnt_telemetry_up, 6);
            ret.value=s1+L"/"+s2;
            ret.metric=L"";
        }
            break;
        case EZWB_STATUS_AIR:{
            std::wstring s2= StringHelper::intToWString(wifibroadcastTelemetryData.cpuload_air, 2);
            std::wstring s3=L"%";
            std::wstring s4= StringHelper::intToWString(wifibroadcastTelemetryData.temp_air, 3);
            std::wstring s5=L"°";
            ret.value=s2+s3+L" "+s4+s5;
            ret.prefix=L"CPU";
            ret.prefixIcon=ICON_CHIP;
        }
            break;
        case EZWB_STATUS_GROUND:{
            std::wstring s2= StringHelper::intToWString(wifibroadcastTelemetryData.cpuload_gnd, 2);
            std::wstring s3=L"%";
            std::wstring s4= StringHelper::intToWString(wifibroadcastTelemetryData.temp_gnd, 3);
            std::wstring s5=L"°";
            ret.value=s2+s3+L" "+s4+s5;
            ret.prefix=L"CPU";
            ret.prefixIcon=ICON_CHIP;
        }
            break;
        case EZWB_BLOCKS:{
            std::wstring s1 = StringHelper::intToWString(
                    (int) wifibroadcastTelemetryData.damaged_block_cnt, 6);
            std::wstring s2 = StringHelper::intToWString(
                    (int) wifibroadcastTelemetryData.lost_packet_cnt, 6);
            ret.value=s1+L"/"+s2;
            ret.prefix=L"";
        }
            break;
        case EZWB_RSSI_ADAPTER0:{
            ret= getTelemetryValueEZWB_RSSI_ADAPTERS_0to5(0);
        }break;
        case EZWB_RSSI_ADAPTER1:{
            ret= getTelemetryValueEZWB_RSSI_ADAPTERS_0to5(1);
        }break;
        case EZWB_RSSI_ADAPTER2:{
            ret= getTelemetryValueEZWB_RSSI_ADAPTERS_0to5(2);
        }break;
        case EZWB_RSSI_ADAPTER3:{
            ret= getTelemetryValueEZWB_RSSI_ADAPTERS_0to5(3);
        }break;
//        case EZWB_RSSI_ADAPTER4:{
//            ret= getTelemetryValueEZWB_RSSI_ADAPTERS_0to5(4);
//        }break;
//        case EZWB_RSSI_ADAPTER5:{
//            ret= getTelemetryValueEZWB_RSSI_ADAPTERS_0to5(5);
//        }break;
        default:
            ret.prefix=L"A";
            ret.value=L"B";
            ret.metric=L"C";
            break;
    }
    return ret;
}

MTelemetryValue
TelemetryReceiver::getTelemetryValueEZWB_RSSI_ADAPTERS_0to5(int adapter) const {
    MTelemetryValue ret = MTelemetryValue();
    ret.warning=0;
    if(adapter>6 || adapter<0){
        adapter=0;
        MLOGD<<"Adapter error";
    }
    if(adapter<wifibroadcastTelemetryData.wifi_adapter_cnt){
        const std::wstring s1= StringHelper::intToWString(
                (int) wifibroadcastTelemetryData.adapter[adapter].current_signal_dbm, 4);
        const std::wstring s2=L"dBm [";
        const std::wstring s3= StringHelper::intToWString(
                (int) wifibroadcastTelemetryData.adapter[adapter].received_packet_cnt, 7);
        const std::wstring s4=L"]";
        ret.value=s1+s2+s3+s4;
    }else{
        ret.value=L"";
    }
    return ret;
}


std::string TelemetryReceiver::getStatisticsAsString()const {
    std::ostringstream ostream;
    if(SOURCE_TYPE==UDP){
        if(T_Protocol!=TelemetryReceiver::NONE){
            ostream<<"\nListening for "+getProtocolAsString()+" telemetry on port "<<T_Port;
            ostream<<"\nReceived: "<<nTelemetryBytes<<"B"<<" | Packets:"<<uav_td.validmsgsrx;
            ostream<<"\n";
        }else{
            ostream<<"UAV telemetry disabled\n";
        }
        if(EZWBS_Protocol!=DISABLED){
            ostream<<"\nListening for "+getSystemAsString()+" statistics on port "<<EZWBS_Port;
            ostream<<"\nReceived: "<<nWIFIBRADCASTBytes<<"B | Packets:"<<nWIFIBROADCASTParsedPackets;
            ostream<<"\n";
        }else{
            ostream<<"\n"+getSystemAsString()+" telemetry disabled\n";
        }
    }else{
        ostream<<"Source type==File. ("+getProtocolAsString()+") Select UDP as data source\n";
        ostream<<"nTelemetryBytes"<<nTelemetryBytes<<"\n";
        ostream<<"nParsedBytes"<<mFileReceiver.getNReceivedBytes()<<"\n";
        ostream<<"Packets:"<<uav_td.validmsgsrx<<"\n";
    }
    return ostream.str();
}

std::string TelemetryReceiver::getAllTelemetryValuesAsString() const {
    std::wstringstream ss;
    for( int i = TelemetryValueIndex ::DECODER_FPS; i != TelemetryValueIndex::EZWB_RSSI_ADAPTER3; i++ ){
        const auto indx = static_cast<TelemetryValueIndex>(i);
        const auto val=getTelemetryValue(indx);
        ss<<val.prefix<<" "<<val.value<<" "<<val.metric<<"\n";
    }
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    const std::string converted_str = converter.to_bytes( ss.str());
    return converted_str;
}

float TelemetryReceiver::getHeading_Deg() const {
    return uav_td.Heading_Deg;
}

float TelemetryReceiver::getCourseOG_Deg() const {
    return uav_td.CourseOG_Deg;
}

float TelemetryReceiver::getHeadingHome_Deg() const {
    return (float)course_to(uav_td.Latitude_dDeg,uav_td.Longitude_dDeg,originData.Latitude_dDeg,originData.Longitude_dDeg);
}

std::string TelemetryReceiver::getProtocolAsString() const {
    std::stringstream ss;
    switch (T_Protocol){
        case TelemetryReceiver::LTM:ss<<"ltm";break;
        case TelemetryReceiver::MAVLINK:ss<<"mavlink";break;
        case TelemetryReceiver::SMARTPORT:ss<<"smartport";break;
        case TelemetryReceiver::FRSKY:ss<<"frsky";break;
        default:ss<<"unknown";break;
    }
    return ss.str();
}

std::string TelemetryReceiver::getSystemAsString() const {
    const std::string ret=(EZWBS_Protocol==EZWB_16_rc6) ? "EZ-Wifibroadcast" : "OpenHD";
    return ret;
}


//----------------------------------------------------JAVA bindings---------------------------------------------------------------
#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_telemetry_core_TelemetryReceiver_##method_name

inline jlong jptr(TelemetryReceiver *videoPlayerN) {
    return reinterpret_cast<intptr_t>(videoPlayerN);
}
inline TelemetryReceiver *native(jlong ptr) {
    return reinterpret_cast<TelemetryReceiver *>(ptr);
}

extern "C" {
JNI_METHOD(jlong , createInstance)
(JNIEnv *env,jclass unused,jobject context,jstring groundRecordingDirectory,jlong externalGroundRecorder,jlong externalFileReader,jint connectedSystem) {
    const auto str=NDKArrayHelper::DynamicSizeString(env,groundRecordingDirectory);
    auto* telemetryReceiver = new TelemetryReceiver(env,str,reinterpret_cast<GroundRecorderFPV*>(externalGroundRecorder),
                                                    reinterpret_cast<FileReader*>(externalFileReader),
                                                    static_cast<TelemetryReceiver::CONNECTED_SYSTEM>(connectedSystem));
    return jptr(telemetryReceiver);
}

JNI_METHOD(void, deleteInstance)
(JNIEnv *env,jclass unused,jlong testReceiverN) {
    TelemetryReceiver* telemetryReceiver=native(testReceiverN);
    delete (telemetryReceiver);
}

JNI_METHOD(void, startReceiving)
(JNIEnv *env,jclass unused,jlong testReceiverN,jobject androidContext) {
    TelemetryReceiver* p=native(testReceiverN);
    p->startReceiving(env,androidContext);
}

JNI_METHOD(void, stopReceiving)
(JNIEnv *env,jclass unused,jlong testReceiverN,jobject androidContext) {
    TelemetryReceiver* testRecN=native(testReceiverN);
    testRecN->stopReceiving(env,androidContext);
}

JNI_METHOD(jstring , getStatisticsAsString)
(JNIEnv *env,jclass unused,jlong telemetryReceiverN) {
    TelemetryReceiver* p=native(telemetryReceiverN);
    jstring ret = env->NewStringUTF(p->getStatisticsAsString().c_str());
    return ret;
}

JNI_METHOD(jstring , getEZWBDataAsString)
(JNIEnv *env,jclass unused,jlong telemetryReceiver) {
    TelemetryReceiver* telRecN=native(telemetryReceiver);
    std::string s = TelemetryHelper::getEZWBDataAsString(telRecN->wifibroadcastTelemetryData);
    jstring ret = env->NewStringUTF(s.c_str());
    return ret;
}

JNI_METHOD(jstring , getTelemetryDataAsString)
(JNIEnv *env,jclass unused,jlong telemetryReceiver) {
    TelemetryReceiver* telRecN=native(telemetryReceiver);
    const std::string s=telRecN->getAllTelemetryValuesAsString();
    jstring ret = env->NewStringUTF(s.c_str());
    return ret;
}
JNI_METHOD(jboolean , anyTelemetryDataReceived)
(JNIEnv *env,jclass unused,jlong testReceiverN) {
    TelemetryReceiver* testRecN=native(testReceiverN);
    bool ret = (testRecN->getNReceivedTelemetryBytes() > 0);
    return (jboolean) ret;
}

JNI_METHOD(jboolean, receivingEZWBButCannotParse)
(JNIEnv *env,jclass unused,jlong testReceiverN) {
    TelemetryReceiver* testRecN=native(testReceiverN);
    return (jboolean) (testRecN->getNEZWBPacketsParsingFailed() > 0);
}

JNI_METHOD(jboolean , isEZWBIpAvailable)
(JNIEnv *env,jclass unused,jlong testReceiverN) {
    TelemetryReceiver* testRecN=native(testReceiverN);
    return (jboolean) (testRecN->getNReceivedTelemetryBytes()>0);
}

JNI_METHOD(jstring, getEZWBIPAdress)
(JNIEnv *env,jclass unused,jlong testReceiverN) {
    TelemetryReceiver* testRec=native(testReceiverN);
    jstring ret = env->NewStringUTF("");
    return ret;
}

JNI_METHOD(void, setDecodingInfo)
(JNIEnv *env,jclass unused,jlong nativeInstance,
        jfloat currentFPS, jfloat currentKiloBitsPerSecond,jfloat avgParsingTime_ms,jfloat avgWaitForInputBTime_ms,jfloat avgDecodingTime_ms) {
    TelemetryReceiver* instance=native(nativeInstance);
    instance->setDecodingInfo((float)currentFPS,(float)currentKiloBitsPerSecond,(float)avgParsingTime_ms,(float)avgWaitForInputBTime_ms,(float)avgDecodingTime_ms);
}

JNI_METHOD(void, setHomeLocation)
(JNIEnv *env,jclass unused,jlong nativeInstance,
 jdouble latitude, jdouble longitude, jdouble attitude) {
    TelemetryReceiver* instance=native(nativeInstance);
    instance->setHomeAndroid((double) latitude, (double) longitude, (double) attitude);
}

JNI_METHOD(void, setDJIValues)
(JNIEnv *env,jclass unused,jlong nativeInstance,
 jdouble Latitude_dDeg,jdouble Longitude_dDeg,jfloat AltitudeX_m,jfloat Roll_Deg,jfloat Pitch_Deg,jfloat SpeedClimb_KPH,jfloat SpeedGround_KPH,jint SatsInUse,jfloat Heading_Deg) {
    TelemetryReceiver* instance=native(nativeInstance);
    instance->uav_td.Latitude_dDeg=Latitude_dDeg;
    instance->uav_td.Longitude_dDeg=Longitude_dDeg;
    instance->uav_td.AltitudeGPS_m=AltitudeX_m;
    instance->uav_td.AltitudeBaro_m=AltitudeX_m;
    instance->uav_td.Roll_Deg=Roll_Deg;
    instance->uav_td.Pitch_Deg=Pitch_Deg;
    instance->uav_td.SpeedClimb_KPH=SpeedClimb_KPH;
    instance->uav_td.SpeedGround_KPH=SpeedGround_KPH;
    instance->uav_td.SatsInUse=SatsInUse;
    instance->uav_td.Heading_Deg=Heading_Deg;
}

JNI_METHOD(void, setDJIBatteryValues)
(JNIEnv *env,jclass unused,jlong nativeInstance,
 jfloat BatteryPack_P,jfloat BatteryPack_A,jfloat BatteryPack_V) {
    TelemetryReceiver* instance=native(nativeInstance);
    instance->uav_td.BatteryPack_P=BatteryPack_P;
    instance->uav_td.BatteryPack_A=BatteryPack_A;
    instance->uav_td.BatteryPack_V=BatteryPack_V;
}

JNI_METHOD(void, setDJISignalQuality)
(JNIEnv *env,jclass unused,jlong nativeInstance,jint qualityUpPercentage,jint qualityDownPercentage) {
    TelemetryReceiver* instance=native(nativeInstance);
    instance->uav_td.DJI_linkQualityUp_P=qualityUpPercentage;
    instance->uav_td.DJI_linkQualityDown_P=qualityDownPercentage;
}

JNI_METHOD(void, nativeIncrementOsdViewMode)
(JNIEnv *env,jclass unused,jlong nativeInstance) {
    TelemetryReceiver* instance=native(nativeInstance);
    int value=instance->OSD_DISPLAY_MODE;
    value++;
    value=value % 3;
    instance->OSD_DISPLAY_MODE=value;
}

}
