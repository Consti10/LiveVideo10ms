
#include "frsky.h"
#include "../SharedCppC/UAVTelemetryData.h"

int frsky_parse_buffer(frsky_state_t *state, UAVTelemetryData *td,const uint8_t *data,const size_t data_length) {
	int i;
	for(i=0; i<data_length; ++i) {
		uint8_t ch = data[i];
		switch(state->sm_state) {
			case 0:
				if(ch == 0x5e)
					state->sm_state = 1;
				break;
			case 1:
				if(ch == 0x5e)
					state->sm_state = 2;
				else
					state->sm_state = 0;
				break;
			case 2:
				if(ch == 0x5e) {
					state->pkg_pos = 0;
					frsky_interpret_packet(state, td);
				}
				else {
					if(state->pkg_pos >= sizeof(state->pkg)) {
						state->pkg_pos = 0;
						state->sm_state = 0;
					} else {
						state->pkg[state->pkg_pos] = ch;
						state->pkg_pos++;
					}
				}
				break;
			default:
				state->sm_state = 0;
				break;
		}
	}
	return 0;
}

int frsky_interpret_packet(frsky_state_t *state, UAVTelemetryData *td) {
	uint16_t data;
	int new_data = 1;

	data = *(uint16_t*)(state->pkg+1);
	switch(state->pkg[0]) {
		case ID_VOLTAGE_AMP:
			td->validmsgsrx++;
			td->BatteryPack_V = (data/1000.0f);
			break;
		case ID_ALTITUDE_BP:
			td->validmsgsrx++;
			td->AltitudeBaro_m = data/100.0f;
			break;
		case ID_ALTITUDE_AP:
			//td->altitude_baro += data/100;
			//printf("Baro Altitude AP:%f  ", td->altitude_baro);
			break;
		case ID_GPS_ALTITUDE_BP:
			td->validmsgsrx++;
			td->AltitudeGPS_m = data/100.0f;
			break;
		case ID_LONGITUDE_BP:
			td->validmsgsrx++;
			td->Longitude_dDeg = data / 100;
			td->Longitude_dDeg += 1.0 * (data - td->Longitude_dDeg * 100) / 60;
			break;
		case ID_LONGITUDE_AP:
			td->validmsgsrx++;
			td->Longitude_dDeg +=  1.0 * data / 60 / 10000;
			break;
		case ID_LATITUDE_BP:
			td->validmsgsrx++;
			td->Latitude_dDeg = data / 100;
			td->Latitude_dDeg += 1.0 * (data - td->Latitude_dDeg * 100) / 60;
			break;
		case ID_LATITUDE_AP:
			td->validmsgsrx++;
			td->Latitude_dDeg +=  1.0 * data / 60 / 10000;
			break;
		case ID_COURSE_BP:
			td->validmsgsrx++;
			td->Heading_Deg = data;
			break;
		case ID_GPS_SPEED_BP:
			td->validmsgsrx++;
			td->SpeedGround_KPH = (uint32_t)(1.0 * data / 0.0194384449);
			break;
		case ID_GPS_SPEED_AP:
			td->validmsgsrx++;
			td->SpeedGround_KPH += 1.0 * data / 1.94384449; //now we are in cm/s
			td->SpeedGround_KPH = td->SpeedGround_KPH / 100 / 1000 * 3600; //now we are in km/h
			break;
		case ID_ACC_X:
			td->validmsgsrx++;
			//-C-td->x = data;
			td->Roll_Deg=data;
			break;
		case ID_ACC_Y:
			//-C-td->validmsgsrx++;
			//-C-td->y = data;
			break;
		case ID_ACC_Z:
			td->validmsgsrx++;
			//-C-td->z = data;
			td->Pitch_Deg=data;
			break;
		case ID_E_W:
			//-C-td->validmsgsrx++;
			//-C-td->ew = data;
			//-C-printf("E/W:%d  ", td->ew);
			break;
		case ID_N_S:
			//-C-td->validmsgsrx++;
			//-C-td->ns = data;
			//-C-printf("N/S:%d  ", td->ns);
			break;
		default:
			new_data = 0;
			//printf("%x\n", pkg[0]);
			break;
	}
	printf("\n");
	return new_data;
}
