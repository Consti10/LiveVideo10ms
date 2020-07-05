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
typedef struct {
    uint32_t validmsgsrx;

    float BatteryPack_V;  //Resolution only mVolt
    float BatteryPack_A;  //Resolution only mAmpere.
    float BatteryPack_mAh; //Already used capacity, in mAh
    float BatteryPack_P; //remaining battery, in percentage.Only sent by MAVLINK -
    // else it has to be calculated manually via already used capacity and battery capacity

    float AltitudeGPS_m; // GPS altitude in m. Relative (to origin) if possible
    float AltitudeBaro_m;// barometric altitude in m. Relative (to origin) if possible

    double Longitude_dDeg; //decimal degrees, Longitude of Aircraft
    double Latitude_dDeg; //decimal degrees Latitude of Aircraft

    float Roll_Deg; //Roll of the aircraft, in degrees
    float Pitch_Deg; //Pitch of the aircraft, in degrees

    float Heading_Deg; //Heading of the aircraft, in degrees
    float CourseOG_Deg; //course over ground (often reported by the compass) of the aircraft, in degrees

    float SpeedGround_KPH; // ( km/h) ==VS1
    float SpeedAir_KPH; // ( km/h) ==VS2
    float SpeedClimb_KPH; //(km/h) ==HS

    int SatsInUse; //n of Satelites visible for GPS on aircraft
    float RSSI1_Percentage_dBm; //RSSI of receiver if not using EZ-WB (rarely transmitted by telemetry protocol). When using MAVLINK in %, else in dBm

    int FlightMode_MAVLINK;
    bool FlightMode_MAVLINK_armed;

} UAVTelemetryData;




#endif //FPV_VR_EZ_WIFIBROADCAST_MISC_H
