//
// Created by Constantin on 12.01.2018.
//

#ifndef FPV_VR_EZ_WIFIBROADCAST_MISC_H
#define FPV_VR_EZ_WIFIBROADCAST_MISC_H

#include <stdio.h>
#include <stdbool.h>

/**
 * This struct was once similar to the uav_telemetry_data in EZ-WB OSD, but it evolved a lot over time
 * and now has almost no similarities.
 */
 // TODO: Ideally this would use the structs from mavlink (they are well documented with units) and ltm for example writes these values,too
typedef struct {
    uint32_t validmsgsrx;

    float BatteryPack_V;  //Resolution most likely mVolt
    float BatteryPack_A;  //Resolution most likely mAmpere.
    float BatteryPack_mAh; //Already used capacity, in mAh
    float BatteryPack_P; //remaining battery, in percentage.Only sent by MAVLINK or DJI -
    // else it has to be calculated manually via already used capacity and battery capacity

    float AltitudeGPS_m; // GPS altitude in m. Relative (to origin) if possible
    float AltitudeBaro_m;// barometric altitude in m. Relative (to origin) if possible

    double Longitude_dDeg; //decimal degrees, Longitude of Aircraft
    double Latitude_dDeg; //decimal degrees Latitude of Aircraft

    float Roll_Deg; //Roll of the aircraft, in degrees. positive is right side down
    float Pitch_Deg; //Pitch of the aircraft, in degrees. positive is nose up

    float Heading_Deg; //Heading of the aircraft, in degrees
    float CourseOG_Deg; //course over ground (often reported by the compass) of the aircraft, in degrees
    // TODO use meters per second instead of kilometers per hour (more scientific)
    float SpeedGround_KPH; // ( km/h) ==VS1
    float SpeedAir_KPH; // ( km/h) ==VS2
    float SpeedClimb_KPH; //(km/h) ==HS

    int SatsInUse; //n of Satelites visible for GPS on aircraft
    float RSSI1_Percentage_dBm; //RSSI of receiver if not using EZ-WB (rarely transmitted by telemetry protocol). When using MAVLINK in %, else in dBm

    int FlightMode_MAVLINK;
    bool FlightMode_MAVLINK_armed;

    // These elements are for DJI drones
    int DJI_linkQualityUp_P;
    int DJI_linkQualityDown_P;
    float DJI_Gimbal_Attitude_Yaw_Degree;

    float Distance_m; // Current distance reading of the rangefinder

} UAVTelemetryData;




#endif //FPV_VR_EZ_WIFIBROADCAST_MISC_H
