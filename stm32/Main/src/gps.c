/*******************************************************************************
 * Copyright (C) 2016 Xuejun Bian, Yu Lu. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom 
 * the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software. 
 
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ******************************************************************************/
#include <string.h>
#include "systick.h"
#include "gps.h"
#include "debug.h"
#include "spi.h"

data_store_t GPS_data;

void GPS_Position(void)
{
    uint8_t i = 0;
    uint8_t data[68];
    
    double longitude;
    double latitude;

    data[64] = 0x00;
    SPI1_GetByte(0x31);
    for(i=0;i<64;i++)
    {
        SysTick_Delay_ms(10);
        data[i] = SPI1_GetByte(i|0x80);
    }
    INFO("%s\r\n",data);
    memset(&GPS_data, 0x00, sizeof(data_store_t));
    nmea_parse_GGA((u8 *)data, 64, false, (u8 *)&GPS_data);
    INFO("type=%d,len=%d,ew=%d,ns=%d,sig=%d,time=%d:%d:%d.%d,lat=%d:%d.%02d%02d, lon=%d:%d.%02d%02d\r\n",
        GPS_data.type, 
        GPS_data.len,
        GPS_data.flag.ew,
        GPS_data.flag.ns,
        GPS_data.flag.signal,
        GPS_data.time.hour,
        GPS_data.time.min,
        GPS_data.time.sec,
        GPS_data.time.secp,
        GPS_data.lat.deg,
        GPS_data.lat.min,
        GPS_data.lat.minp1,
        GPS_data.lat.minp2,
        GPS_data.lon.deg,
        GPS_data.lon.min,
        GPS_data.lon.minp1,
        GPS_data.lon.minp2);
  if(GPS_data.lon.deg != 0)
  {
    longitude = (double)GPS_data.lon.deg + ((double)GPS_data.lon.min + (double)GPS_data.lon.minp1 / 100 + (double)GPS_data.lon.minp2 / 10000) / 60;
    latitude = (double)GPS_data.lat.deg + ((double)GPS_data.lat.min + (double)GPS_data.lat.minp1 / 100 + (double)GPS_data.lat.minp2 / 10000) / 60;
    INFO("\"GPSDW\":\"%.06f,%.07f\"",longitude, latitude);
  }
}

