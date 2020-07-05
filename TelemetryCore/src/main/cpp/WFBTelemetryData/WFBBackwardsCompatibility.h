//
// Created by Consti10 on 29/04/2019.
//

#ifndef FPV_VR_PRIVATE_WFBBACKWARDSCOMPATIBILITY_H
#define FPV_VR_PRIVATE_WFBBACKWARDSCOMPATIBILITY_H

#include "WFBTelemetryData.h"

//the newer struct has more values then the old one
//but is mostly backwards compatible
void writeDataBackwardsCompatible(wifibroadcast_rx_status_forward_t2 *dst,
                                  const wifibroadcast_rx_status_forward_t *source){
    dst->damaged_block_cnt=source->damaged_block_cnt;
    dst->lost_packet_cnt=source->lost_packet_cnt;
    dst->skipped_packet_cnt=source->skipped_packet_cnt;
    dst->received_packet_cnt=source->received_packet_cnt;
    dst->kbitrate=source->kbitrate;
    dst->kbitrate_measured=source->kbitrate_measured;
    dst->kbitrate_set=source->kbitrate_set;
    dst->lost_packet_cnt_telemetry_up=source->lost_packet_cnt_telemetry_up;
    dst->lost_packet_cnt_telemetry_down=source->lost_packet_cnt_telemetry_down;
    dst->lost_packet_cnt_msp_up=source->lost_packet_cnt_msp_up;
    dst->lost_packet_cnt_msp_down=source->lost_packet_cnt_msp_down;
    dst->lost_packet_cnt_rc=source->lost_packet_cnt_rc;

    dst->joystick_connected=source->joystick_connected;
    dst->cpuload_gnd=source->cpuload_gnd;
    dst->temp_gnd=source->temp_gnd;
    dst->cpuload_air=source->cpuload_air;
    dst->temp_air=source->temp_air;
    dst->wifi_adapter_cnt=source->wifi_adapter_cnt;
    for(int i=0;i<6;i++){
        const wifi_adapter_rx_status_forward_t* o1=&source->adapter[i];
        wifi_adapter_rx_status_forward_t2* n1=&dst->adapter[i];
        n1->received_packet_cnt=o1->received_packet_cnt;
        n1->current_signal_dbm=o1->current_signal_dbm;
        n1->type=o1->type;
    }
}

#endif //FPV_VR_PRIVATE_WFBBACKWARDSCOMPATIBILITY_H
