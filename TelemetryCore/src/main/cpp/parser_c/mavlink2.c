#include "mavlink2.h"
#include "../SharedCppC/UAVTelemetryData.h"
#include <stdio.h>
#include <unistd.h>

mavlink_status_t status;
mavlink_message_t msg;

void mavlink_read_v2(UAVTelemetryData *td,OriginData *originData,const uint8_t *data,const size_t data_length) {
    //__android_log_print(ANDROID_LOG_DEBUG,"T","MAV:");
    for(int i=0; i<data_length; i++) {
        uint8_t c = data[i];
        if (mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) {
            td->validmsgsrx++;
        /*const uint8_t res=mavlink_frame_char(MAVLINK_COMM_0, c, &msg,&status);
        if(res!=MAVLINK_FRAMING_INCOMPLETE){
            __android_log_print(ANDROID_LOG_DEBUG,"not incomplete","%d",res);
        }
        if (res == MAVLINK_FRAMING_OK){
            __android_log_print(ANDROID_LOG_DEBUG,"received message with","ID %d, sequence: %d from component %d of system %d", msg.msgid, msg.seq, msg.compid, msg.sysid);
            //__android_log_print(ANDROID_LOG_DEBUG,"Message seq:","%d",msg.seq);
            td->validmsgsrx++;*/
            switch (msg.msgid){
                case MAVLINK_MSG_ID_GPS_RAW_INT:{
                    td->SatsInUse = mavlink_msg_gps_raw_int_get_satellites_visible(&msg);
                    td->CourseOG_Deg = mavlink_msg_gps_raw_int_get_cog(&msg)/100.0f;
                    break;
                }
                case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:{
                    td->Heading_Deg = mavlink_msg_global_position_int_get_hdg(&msg)/100.0f;
                    td->AltitudeGPS_m = mavlink_msg_global_position_int_get_relative_alt(&msg)/1000.0f;
                    td->Latitude_dDeg = mavlink_msg_global_position_int_get_lat(&msg)/10000000.0;
                    td->Longitude_dDeg = mavlink_msg_global_position_int_get_lon(&msg)/10000000.0;
                    break;
                }
                case MAVLINK_MSG_ID_ATTITUDE:{
                    td->Roll_Deg = (float)(mavlink_msg_attitude_get_roll(&msg)*57.2958);
                    td->Pitch_Deg = (float)(mavlink_msg_attitude_get_pitch(&msg)*57.2958);
                    break;
                }
                case MAVLINK_MSG_ID_SYS_STATUS:{
                    td->BatteryPack_V = (float)mavlink_msg_sys_status_get_voltage_battery(&msg)/1000.0f;
                    td->BatteryPack_A = (mavlink_msg_sys_status_get_current_battery(&msg)/100.0f);
                    td->BatteryPack_P= mavlink_msg_sys_status_get_battery_remaining(&msg);
                    break;
                }
                case MAVLINK_MSG_ID_BATTERY_STATUS:{
                     mavlink_battery_status_t battery_status;
                     mavlink_msg_battery_status_decode(&msg, &battery_status);
                     td->BatteryPack_mAh=battery_status.current_consumed;
                }break;
                case MAVLINK_MSG_ID_VFR_HUD:{
                    td->SpeedGround_KPH = mavlink_msg_vfr_hud_get_groundspeed(&msg)*3.6f;
                    td->SpeedAir_KPH = mavlink_msg_vfr_hud_get_airspeed(&msg)*3.6f;
                    td->SpeedClimb_KPH = mavlink_msg_vfr_hud_get_climb(&msg)*3.6f;
                    break;
                }
                case MAVLINK_MSG_ID_GPS_STATUS:{
                    break;
                }
                case MAVLINK_MSG_ID_HEARTBEAT:{
                    td->FlightMode_MAVLINK = mavlink_msg_heartbeat_get_custom_mode(&msg);
                    if (((mavlink_msg_heartbeat_get_base_mode(&msg) & 0b10000000) >> 7) == 0) {
                        td->FlightMode_MAVLINK_armed = false;
                    } else {
                        td->FlightMode_MAVLINK_armed = true;
                    }
                    break;
                }
                case MAVLINK_MSG_ID_RC_CHANNELS_RAW:{
                    td->RSSI1_Percentage_dBm = (float)(mavlink_msg_rc_channels_raw_get_rssi(&msg)*100/255);
                    break;
                }
                case MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN:{
                    //__android_log_print(ANDROID_LOG_DEBUG,"T","ORIGIN X:");
                    if(originData->writeByTelemetryProtocol){
                        originData->Latitude_dDeg=mavlink_msg_gps_global_origin_get_latitude(&msg)/10000000.0;
                        originData->Longitude_dDeg=mavlink_msg_gps_global_origin_get_longitude(&msg)/10000000.0;
                        //originData->Altitude_m=mavlink_msg_gps_global_origin_get_altitude(&msg)/1000.0f;
                        originData->hasBeenSet=true;
                    }
                    break;
                }
                case MAVLINK_MSG_ID_DISTANCE_SENSOR:{
                    td->Distance_m = mavlink_msg_distance_sensor_get_current_distance(&msg)/100.0f;
                    break;
                }
                default:
                    //__android_log_print(ANDROID_LOG_DEBUG,"Message:","%d",msg.msgid);
                    break;
            }
        }else{

        }
    }
}


//MAVLINK_MSG_ID_SYSTEM_TIME 2
//MAVLINK_MSG_ID_RAW_IMU 27
//MAVLINK_MSG_ID_SCALED_PRESSURE 29
//MAVLINK_MSG_ID_SERVO_OUTPUT_RAW 36
//MAVLINK_MSG_ID_MISSION_CURRENT 42
//MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT 62
//


//					td->Heading_Deg = mavlink_msg_gps_raw_int_get_cog(&msg)/100.0f;
//                  td->altitude_gps = mavlink_msg_gps_raw_int_get_alt(&msg)/1000.0f;
//                  td->Latitude_dDeg = mavlink_msg_gps_raw_int_get_lat(&msg)/10000000.0f;
//                  td->Longitude_dDeg = mavlink_msg_gps_raw_int_get_lon(&msg)/10000000.0f;

/* mavlink_status_t* chan_state = mavlink_get_channel_status(MAVLINK_COMM_0);

       if(!(chan_state->flags & MAVLINK_STATUS_FLAG_IN_MAVLINK1) == MAVLINK_STATUS_FLAG_IN_MAVLINK1)
       {
           chan_state->flags |=MAVLINK_STATUS_FLAG_IN_MAVLINK1;
       }*/

//__android_log_print(ANDROID_LOG_DEBUG,"MAV v1","not enabled");

//-C-fprintf(stdout, "Msg seq:%d sysid:%d, compid:%d  ", msg.seq, msg.sysid, msg.compid);

/*if (msg.magic == MAVLINK_STX_MAVLINK1) {
    __android_log_print(ANDROID_LOG_DEBUG,"v1","message");
}else{
    __android_log_print(ANDROID_LOG_DEBUG,"v2","message");
}*/

/*                                case MAVLINK_MSG_ID_BATTERY_STATUS:
					fprintf(stdout, "BATTERY_STATUS ");
                                        break;
                                case MAVLINK_MSG_ID_ALTITUDE:
					fprintf(stdout, "ALTITUDE ");
                                        break;
                                case MAVLINK_MSG_ID_LOCAL_POSITION_NED:
					fprintf(stdout, "LOCAL_POSITION_NED ");
                                        break;
                                case MAVLINK_MSG_ID_SERVO_OUTPUT_RAW:
					fprintf(stdout, "SERVO_OUTPUT_RAW ");
                                        break;
                                case MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED:
					fprintf(stdout, "POSITION_TARGET_LOCAL_NED ");
                                        break;
                                case MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT:
					fprintf(stdout, "POSITION_TARGET_GLOBAL_INT ");
                                        break;
                                case MAVLINK_MSG_ID_ESTIMATOR_STATUS:
					fprintf(stdout, "ESTIMATOR_STATUS ");
                                        break;
                                case MAVLINK_MSG_ID_WIND_COV:
					fprintf(stdout, "WIND_COV ");
                                        break;
                                case MAVLINK_MSG_ID_VIBRATION:
					fprintf(stdout, "VIBRATION ");
                                        break;
                                case MAVLINK_MSG_ID_HOME_POSITION:
					fprintf(stdout, "HIGHRES_IMU ");
                                        break;
                                case MAVLINK_MSG_ID_NAMED_VALUE_FLOAT:
					fprintf(stdout, "NAMED_VALUE_FLOAT ");
                                        break;
                                case MAVLINK_MSG_ID_NAMED_VALUE_INT:
					fprintf(stdout, "NAMED_VALUE_INT ");
                                        break;
                                case MAVLINK_MSG_ID_PARAM_VALUE:
					fprintf(stdout, "PARAM_VALUE ");
                                        break;
                                case MAVLINK_MSG_ID_PARAM_SET:
					fprintf(stdout, "PARAM_SET ");
                                        break;
                                case MAVLINK_MSG_ID_PARAM_REQUEST_READ:
					fprintf(stdout, "PARAM_REQUEST_READ ");
                                        break;
                                case MAVLINK_MSG_ID_PARAM_REQUEST_LIST:
					fprintf(stdout, "PARAM_REQUEST_LIST ");
                                        break;
                                case MAVLINK_MSG_ID_RC_CHANNELS_SCALED:
					fprintf(stdout, "RC_CHANNELS_SCALED ");
                                        break;
                                case MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE:
					fprintf(stdout, "RC_CHANNELS_OVERRIDE ");
                                        break;
                                case MAVLINK_MSG_ID_RC_CHANNELS:
					fprintf(stdout, "RC_CHANNELS ");
                                        break;
                                case MAVLINK_MSG_ID_MANUAL_CONTROL:
					fprintf(stdout, "MANUAL_CONTROL ");
                                        break;
                                case MAVLINK_MSG_ID_COMMAND_LONG:
					fprintf(stdout, "COMMAND_LONG:%d ",mavlink_msg_command_long_get_command(&msg));
                                        break;
                                case MAVLINK_MSG_ID_STATUSTEXT:
					fprintf(stdout, "STATUSTEXT: severity:%d ",mavlink_msg_statustext_get_severity(&msg));
                                        break;
                                case MAVLINK_MSG_ID_SYSTEM_TIME:
					fprintf(stdout, "SYSTEM_TIME ");
                                        break;
                                case MAVLINK_MSG_ID_PING:
					fprintf(stdout, "PING ");
                                        break;
                                case MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL:
					fprintf(stdout, "CHANGE_OPERATOR_CONTROL ");
                                        break;
                                case MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK:
					fprintf(stdout, "CHANGE_OPERATOR_CONTROL_ACK ");
                                        break;
                                case MAVLINK_MSG_ID_MISSION_WRITE_PARTIAL_LIST:
					fprintf(stdout, "MISSION_WRITE_PARTIAL_LIST ");
                                        break;
                                case MAVLINK_MSG_ID_MISSION_ITEM:
					fprintf(stdout, "MISSION_ITEM ");
                                        break;
                                case MAVLINK_MSG_ID_MISSION_REQUEST:
					fprintf(stdout, "MISSION_REQUEST ");
                                        break;
                                case MAVLINK_MSG_ID_MISSION_SET_CURRENT:
					fprintf(stdout, "MISSION_SET_CURRENT ");
                                        break;
                                case MAVLINK_MSG_ID_MISSION_CURRENT:
					fprintf(stdout, "MISSION_CURRENT ");
                                        break;
                                case MAVLINK_MSG_ID_MISSION_REQUEST_LIST:
					fprintf(stdout, "MISSION_REQUEST_LIST ");
                                        break;
                                case MAVLINK_MSG_ID_MISSION_COUNT:
					fprintf(stdout, "MISSION_COUNT ");
                                        break;
                                case MAVLINK_MSG_ID_MISSION_CLEAR_ALL:
					fprintf(stdout, "MISSION_CLEAR_ALL ");
                                        break;
                                case MAVLINK_MSG_ID_MISSION_ITEM_REACHED:
					fprintf(stdout, "MISSION_ITEM_REACHED ");
                                        break;
                                case MAVLINK_MSG_ID_MISSION_ACK:
					fprintf(stdout, "MISSION_ACK ");
                                        break;
                                case MAVLINK_MSG_ID_MISSION_ITEM_INT:
					fprintf(stdout, "MISSION_ITEM_INT ");
                                        break;
                                case MAVLINK_MSG_ID_MISSION_REQUEST_INT:
					fprintf(stdout, "MISSION_REQUEST_INT ");
                                        break;
                                case MAVLINK_MSG_ID_SET_MODE:
					fprintf(stdout, "SET_MODE ");
                                        break;
                                case MAVLINK_MSG_ID_REQUEST_DATA_STREAM:
					fprintf(stdout, "REQUEST_DATA_STREAM ");
                                        break;
                                case MAVLINK_MSG_ID_DATA_STREAM:
					fprintf(stdout, "DATA_STREAM ");
*/
