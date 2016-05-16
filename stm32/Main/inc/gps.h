#ifndef __GPS_H
#define __GPS_H
#include "stm32f10x.h"
#include <stdio.h>

#define GPS_DATA_SIZE 1024

extern uint16_t gps_res_cnt;
extern uint8_t gps_datas[GPS_DATA_SIZE];


#endif  /*__GPS_H*/
