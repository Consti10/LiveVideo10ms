/* #################################################################################################################
 * LightTelemetry protocol (LTM)
 *
 * Ghettostation one way telemetry protocol for really low bitrates (1200/2400 bauds).
 *
 * Protocol details: 3 different frames, little endian.
 *   G Frame (GPS position) (2hz @ 1200 bauds , 5hz >= 2400 bauds): 18BYTES
 *    0x24 0x54 0x47 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF  0xFF   0xC0
 *     $     T    G  --------LAT-------- -------LON---------  SPD --------ALT-------- SAT/FIX  CRC
 *   A Frame (Attitude) (5hz @ 1200bauds , 10hz >= 2400bauds): 10BYTES
 *     0x24 0x54 0x41 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xC0
 *      $     T   A   --PITCH-- --ROLL--- -HEADING-  CRC
 *   S Frame (Sensors) (2hz @ 1200bauds, 5hz >= 2400bauds): 11BYTES
 *     0x24 0x54 0x53 0xFF 0xFF  0xFF 0xFF    0xFF    0xFF      0xFF       0xC0
 *      $     T   S   VBAT(mv)  Current(ma)   RSSI  AIRSPEED  ARM/FS/FMOD   CRC
 * ################################################################################################################# */
#include "ltm.h"
#include "../SharedCppC/UAVTelemetryData.h"


static uint8_t LTMserialBuffer[LIGHTTELEMETRY_GFRAMELENGTH-4];
static uint8_t LTMreceiverIndex;
static uint8_t LTMcmd;
static uint8_t LTMrcvChecksum;
static uint8_t LTMreadIndex;
static uint8_t LTMframelength;



uint8_t ltmread_u8()  {
    return LTMserialBuffer[LTMreadIndex++];
}

uint16_t ltmread_u16() {
    uint16_t t = ltmread_u8();
    t |= (uint16_t)ltmread_u8()<<8;
    return t;
}

uint32_t ltmread_u32() {
    uint32_t t = ltmread_u16();
    t |= (uint32_t)ltmread_u16()<<16;
    return t;
}

static enum _serial_state {
    IDLE,
    HEADER_START1,
    HEADER_START2,
    HEADER_MSGTYPE,
    HEADER_DATA
}
        c_state = IDLE;

int ltm_read(UAVTelemetryData *td,OriginData *originData,const uint8_t *data,const size_t data_length,const bool readAltitudeSigned) {
    int i;

    for(i=0; i<data_length; ++i) {
        uint8_t c = data[i];
        if (c_state == IDLE) {
            c_state = (c=='$') ? HEADER_START1 : IDLE;
        }
        else if (c_state == HEADER_START1) {
            c_state = (c=='T') ? HEADER_START2 : IDLE;
        }
        else if (c_state == HEADER_START2) {
            switch (c) {
                case 'G':
                    LTMframelength = LIGHTTELEMETRY_GFRAMELENGTH;
                    c_state = HEADER_MSGTYPE;
                    break;
                case 'A':
                    LTMframelength = LIGHTTELEMETRY_AFRAMELENGTH;
                    c_state = HEADER_MSGTYPE;
                    break;
                case 'S':
                    LTMframelength = LIGHTTELEMETRY_SFRAMELENGTH;
                    c_state = HEADER_MSGTYPE;
                    break;
                case 'O':
                    LTMframelength = LIGHTTELEMETRY_OFRAMELENGTH;
                    c_state = HEADER_MSGTYPE;
                    break;
                case 'N':
                    LTMframelength = LIGHTTELEMETRY_NFRAMELENGTH;
                    c_state = HEADER_MSGTYPE;
                    break;
                case 'X':
                    LTMframelength = LIGHTTELEMETRY_XFRAMELENGTH;
                    c_state = HEADER_MSGTYPE;
                    break;
                default:
                    c_state = IDLE;
            }
            LTMcmd = c;
            LTMreceiverIndex=0;
        }
        else if (c_state == HEADER_MSGTYPE) {
            if(LTMreceiverIndex == 0) {
                LTMrcvChecksum = c;
            }
            else {
                LTMrcvChecksum ^= c;
            }
            if(LTMreceiverIndex == LTMframelength-4) {   // received checksum byte
                if(LTMrcvChecksum == 0) {
                    if(ltm_check(td,originData,readAltitudeSigned) == 1)
                    c_state = IDLE;
                }
                else { // wrong checksum, drop packet
                    c_state = IDLE;
                }
            }
            else LTMserialBuffer[LTMreceiverIndex++]=c;
        }
    }
    return 0;
}

// --------------------------------------------------------------------------------------
// Decoded received commands
int ltm_check(UAVTelemetryData *td,OriginData *originData,const bool readAltitudeSigned) {
    LTMreadIndex = 0;

    if (LTMcmd==LIGHTTELEMETRY_GFRAME)  {
        td->Latitude_dDeg = (ltmread_u32())/10000000.0;
        td->Longitude_dDeg = (ltmread_u32())/10000000.0;
        td->SpeedGround_KPH = (ltmread_u8() * 3.6f); // convert to kmh
        td->AltitudeGPS_m = readAltitudeSigned ?
                 (((signed)ltmread_u32())/100.0f) //needs to be signed with iNAV
                :((ltmread_u32())/100.0f);
        uint8_t ltm_satsfix = ltmread_u8();
        td->SatsInUse = ((ltm_satsfix >> 2) & 0xFF);
        td->validmsgsrx++;

    }else if (LTMcmd==LIGHTTELEMETRY_AFRAME)  {
        td->Pitch_Deg = (int16_t)ltmread_u16();
        td->Roll_Deg =  (int16_t)ltmread_u16();
        td->Heading_Deg = (float)((int16_t)ltmread_u16());
        if (td->Heading_Deg < 0 ) td->Heading_Deg = td->Heading_Deg + 360; //convert from -180/180 to 0/360Â°
        td->validmsgsrx++;

    }else if (LTMcmd==LIGHTTELEMETRY_OFRAME)  {
        if(originData->writeByTelemetryProtocol){
            originData->Latitude_dDeg = (double)((int32_t)ltmread_u32())/10000000;
            originData->Longitude_dDeg = (double)((int32_t)ltmread_u32())/10000000;
            //originData->Altitude_m= (ltmread_u32())/100.0f;
            originData->hasBeenSet=true;
        }
        td->validmsgsrx++;

    }else if (LTMcmd==LIGHTTELEMETRY_XFRAME)  {
        //HDOP 		uint16 HDOP * 100
        //hw status 	uint8
        //LTM_X_counter 	uint8
        //Disarm Reason 	uint8
        //(unused) 		1byte
        //-C-td->ltm_hdop = (float)((uint16_t)ltmread_u16())/10000.0f;
        //-C-printf("LTM X FRAME:\n");
        //-C-printf("GPS hdop:%.2f  ", td->ltm_hdop);
    }else if (LTMcmd==LIGHTTELEMETRY_SFRAME)  {
        //Vbat 			uint16, mV
        //Battery Consumption 	uint16, mAh
        //RSSI 			uchar
        //Airspeed 			uchar, m/s
        //Status 			uchar
        td->BatteryPack_V = (ltmread_u16()/1000.0f);
        td->BatteryPack_mAh = ltmread_u16();
        td->RSSI1_Percentage_dBm = (float)ltmread_u8();

        uint8_t uav_airspeedms = ltmread_u8();
        td->SpeedAir_KPH = (uint32_t)(uav_airspeedms * 3.6f); // convert to kmh

        uint8_t ltm_armfsmode = ltmread_u8();
        //td->armed = (uint8_t)(ltm_armfsmode & 0b00000001);
        //-C-td->ltm_failsafe = (ltm_armfsmode >> 1) & 0b00000001;
        //-C-td->ltm_flightmode = (ltm_armfsmode >> 2) & 0b00111111;
        td->validmsgsrx++;
    }
    printf("\n");
    return 0;
}
