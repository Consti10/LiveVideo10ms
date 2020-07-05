
#ifndef FPV_VR_MAVLINK2_H
#define FPV_VR_MAVLINK2_H

#include "mavlink_v2/common/mavlink.h"
#include "../SharedCppC/UAVTelemetryData.h"
#include "../SharedCppC/OriginData.h"

void mavlink_read_v2(UAVTelemetryData *td,OriginData *originData,const uint8_t *data, const size_t data_length);

#endif
