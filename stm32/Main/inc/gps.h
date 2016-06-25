#ifndef __GPS_H
#define __GPS_H
#include "stm32f10x.h"
#include <stdio.h>
#include "nmealib.h"


void GPS_Position(void);

extern data_store_t GPS_data;
#endif  /*__GPS_H*/
