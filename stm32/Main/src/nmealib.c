 /**
 * This file is part of nmealib.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "nmealib.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

bool parse_attr_GGA (u8 *attr, unsigned int len, u8 index, u8 **data);
bool attr_GGA_handler_time(u8 *attr, unsigned int len, u8 index, u8 **data);
bool attr_GGA_handler_loc(u8 *attr, unsigned int len, u8 index, u8 **data);
bool attr_GGA_handler_NSEW(u8 *attr, unsigned int len, u8 index, u8 **data);
bool attr_GGA_handler_header(u8 *attr, unsigned int len, u8 index, u8 **data);
bool attr_GGA_handler_none(u8 *attr, unsigned int len, u8 index, u8 **data);


/**
 * Calculate crc control sum of a string.
 * If the string starts with a '$' then that character is skipped as per
 * the NMEA spec.
 *
 * @param s the string
 * @param len the length of the string
 * @return the crc
 */
int nmea_calc_crc(const char *s, const int len) {
  int chksum = 0;
  int it = 0;

  if (s[it] == '$')
    it++;

  for (; it < len; it++)
    chksum ^= (int) s[it];

  return chksum;
}


/**
 * Parse nmea GPGGA TIME (time only, no date) from a string.
 * The format that is used (hhmmss, hhmmss.s, hhmmss.ss or hhmmss.sss) is
 * determined by the length of the string.
 *
 * @param attr the string of time info
 * @param len the length of the string
 * @param index info the attr index of this line
 * @param data output struct's point
 * @return true on success, false otherwise
 */
bool attr_GGA_handler_time(u8 *attr, unsigned int len, u8 index, u8 **data){
  char tmp[3];
  data_store_t * val = (data_store_t *)*data;

  memset(tmp, 0x00, 3);
  if(len < 6  ||  (len >= 7 && attr[6] != '.')) // format is "hhmmss.ss" or "hhmmss" ,"hhmmss.s" , "hhmmss.sss"
    return true;
  tmp[0] = attr[0];
  tmp[1] = attr[1];
  val->time.hour = strtol(tmp, NULL, 10);
  tmp[0] = attr[2];
  tmp[1] = attr[3];
  val->time.min = strtol(tmp, NULL, 10);
  tmp[0] = attr[4];
  tmp[1] = attr[5];
  val->time.sec = strtol(tmp, NULL, 10);  
  if(len >=7){
    tmp[0] = attr[7];
    tmp[1] = attr[8];
    val->time.secp = strtol(tmp, NULL, 10);     
  }
 
  return true;
}
/**
 * Parse nmea GPGGA location from a string.
 * The format that is used (ddmm.mm or ddmm.mmmm) is
 * determined by the length of the string.
 *
 * @param attr the string of time info
 * @param len the length of the string
 * @param index info the attr index of this line
 * @param data output struct's point
 * @return true on success, false otherwise
 */
bool attr_GGA_handler_loc(u8 *attr, unsigned int len, u8 index, u8 **data){
  char tmp[4];
  data_store_t *val = (data_store_t *) *data;
  memset(tmp, 0x00, 4);
  if(index == 2){
    if((len != 9 && len != 7) || attr[4] != '.') // format is "ddmm.mmmm" or "ddmm.mm"
      return true;    
    tmp[0] = attr[0];
    tmp[1] = attr[1];
    val->lat.deg = strtol(tmp, NULL, 10);
    tmp[0] = attr[2];
    tmp[1] = attr[3];
    val->lat.min = strtol(tmp, NULL, 10);
    tmp[0] = attr[5];
    tmp[1] = attr[6];
    val->lat.minp1 = strtol(tmp, NULL, 10);  
    if(len == 7)
      val->lat.minp2 = 0;
    else{
      tmp[0] = attr[7];
      tmp[1] = attr[8];
      val->lat.minp2 = strtol(tmp, NULL, 10);        
    }
  }
  else if(index == 4){

    if((len == 9 || len == 7) && attr[4] == '.'){  // format is "ddmm.mmmm" or "ddmm.mm"
      tmp[0] = attr[0];
      tmp[1] = attr[1];
      val->lon.deg = strtol(tmp, NULL, 10);
      tmp[0] = attr[2];
      tmp[1] = attr[3];
      val->lon.min = strtol(tmp, NULL, 10);
      tmp[0] = attr[5];
      tmp[1] = attr[6];
      val->lon.minp1 = strtol(tmp, NULL, 10);  
      if(len == 7)
        val->lon.minp2 = 0;
      else{
        tmp[0] = attr[7];
        tmp[1] = attr[8];
        val->lon.minp2 = strtol(tmp, NULL, 10);        
      }
    }
    else if((len == 10 || len == 8) && attr[5] == '.') {// format is "dddmm.mmmm" or "dddmm.mm"    
      tmp[0] = attr[0];
      tmp[1] = attr[1];
      tmp[2] = attr[2];
      val->lon.deg = strtol(tmp, NULL, 10);
      tmp[2] = '\0';
      tmp[0] = attr[3];
      tmp[1] = attr[4];
      val->lon.min = strtol(tmp, NULL, 10);
      tmp[0] = attr[6];
      tmp[1] = attr[7];
      val->lon.minp1 = strtol(tmp, NULL, 10);  
      if(len == 8)
        val->lon.minp2 = 0;
      else{
        tmp[0] = attr[8];
        tmp[1] = attr[9];
        val->lon.minp2 = strtol(tmp, NULL, 10);        
      }
    }
    else
      return true;
  }
  return true;
}


/**
 * Validate north/south or east/west and uppercase it.
 * Expects:
 * <pre>
 * 3. N or S (North or South)
 * 4. Longitude
 * 5. E or W (East or West) 
 *   c in { n, N, s, S } (for north/south)
 *   c in { e, E, w, W } (for east/west)
 * </pre>
 *
 * @param c a pointer to the character. The character will be converted to uppercase.
 * @param ns true: evaluate north/south, false: evaluate east/west
 * @return true when valid, false otherwise
 */
bool attr_GGA_handler_NSEW(u8 *attr, unsigned int len, u8 index, u8 **data){
  u8 *ret = *data;
  u8 dir = toupper(*attr);
  if(len == 0)
    return true;

  else if (index == 3) {
    if ((dir == 'N') || (dir == 'S')) {
#if defined(RESOLVED_DATA_TYPE)      
      *ret=dir;
      ret ++;
      *data = ret;
#else
      ((data_store_t *)ret)->flag.ns = (dir == 'N')?1:0;
#endif      
    }
      return true;
  } 
  else  if(index == 5){
    if ((dir == 'E') || (dir == 'W')) {
#if defined(RESOLVED_DATA_TYPE)      
      *ret=dir;
      ret ++;
      *data = ret;
#else
      ((data_store_t *)ret)->flag.ew = (dir == 'E')?1:0;
#endif  
    }
    return true;
  }

  return true;
}

/**
 * Parse Head of GPGGA from a string.
 *
 * @param attr the string of time info
 * @param len the length of the string
 * @param index info the attr index of this line
 * @param data output struct's point
 * @return true on success, false otherwise
 */
bool attr_GGA_handler_header(u8 *attr, unsigned int len, u8 index, u8 **data){
  u8 header[] = "$GPGGA";
  if(0 != memcmp(attr, header, strlen((char const *)header)))
    return false;
#if ! defined(RESOLVED_DATA_TYPE)
  ((data_store_t *)*data)->type = RESOLVED_GGA_DATA_TYPE;
  ((data_store_t *)*data)->len = sizeof(data_store_t) - sizeof(((data_store_t *)*data)->type)\
      - sizeof(((data_store_t *)*data)->len);  
#endif  
  return true;
}

/**
 * None operator of parse
 *
 * @param attr the string of time info
 * @param len the length of the string
 * @param index info the attr index of this line
 * @param data output struct's point
 * @return true on success, false otherwise
 */
bool attr_GGA_handler_none(u8 *attr, unsigned int len, u8 index, u8 **data){
  return true;
}


/**
* 
* === GGA - Global Positioning System Fix Data ===
* 
* Time, Position and fix related data for a GPS receiver.
* 
* ------------------------------------------------------------------------------
*                                                       11
*         1         2       3 4        5 6 7  8   9  10 |  12 13  14   15
*         |         |       | |        | | |  |   |   | |   | |   |    |
*  $--GGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh<CR><LF>
* ------------------------------------------------------------------------------
* 
* Field Number: 
* 
* 1. Universal Time Coordinated (UTC)
* 2. Latitude
* 3. N or S (North or South)
* 4. Longitude
* 5. E or W (East or West)
* 6. GPS Quality Indicator,
*      - 0 - fix not available,
*      - 1 - GPS fix,
*      - 2 - Differential GPS fix
*            (values above 2 are 2.3 features)
*      - 3 = PPS fix
*      - 4 = Real Time Kinematic
*      - 5 = Float RTK
*      - 6 = estimated (dead reckoning)
*      - 7 = Manual input mode
*      - 8 = Simulation mode
* 7. Number of satellites in view, 00 - 12
* 8. Horizontal Dilution of precision (meters)
* 9. Antenna Altitude above/below mean-sea-level (geoid) (in meters)
* 10. Units of antenna altitude, meters
* 11. Geoidal separation, the difference between the WGS-84 earth
*      ellipsoid and mean-sea-level (geoid), "-" means mean-sea-level
*      below ellipsoid
* 12. Units of geoidal separation, meters
* 13. Age of differential GPS data, time in seconds since last SC104
*      type 1 or 9 update, null field when DGPS is not used
* 14. Differential reference station ID, 0000-1023
* 15. Checksum
**/

attr_GGA_handler attr_GGA_handler_list[]={
  attr_GGA_handler_header,    //0 header
  attr_GGA_handler_time,      //1 utc
  attr_GGA_handler_loc,      //2 Lat
  attr_GGA_handler_NSEW,      //3 N S
  attr_GGA_handler_loc,      //4 LON
  attr_GGA_handler_NSEW,      //5 E W
  attr_GGA_handler_none,      //6 available
  attr_GGA_handler_none,      //7 satellite number
  attr_GGA_handler_none,      //8 precision of hori 
  attr_GGA_handler_none,      //9
  attr_GGA_handler_none,      //10

};


bool parse_attr_GGA (u8 *attr, unsigned int len, u8 index, u8 **data){
  if(index >= sizeof(attr_GGA_handler_list)/sizeof(attr_GGA_handler))
    return false;
  return attr_GGA_handler_list[index](attr, len, index, data);
}


bool nmea_parse_GGA(u8 *s, const int len, bool has_checksum, u8 *pack){

  u8 attrIndex = 0;
  u8 * p = (u8 *)s;
  u8 * attr = (u8 *)s;
  u8 * data = pack;

#if defined(RESOLVED_DATA_TYPE)

  ((resolved_data_t *)data)->type = RESOLVED_GGA_DATA_TYPE; 
  ((resolved_data_t *)data)->len = 0;     // length
  data =  ((resolved_data_t *)data)->data;
#else

  ((data_store_t *)data)->type = 0;
  ((data_store_t *)data)->len = 0;
#endif

  while (p && (p-(u8 *)s) < len 
    && p[0] != '\0' && p[0] != '\r' && p[0] != '\n'
    && attrIndex < sizeof(attr_GGA_handler_list)
    ){
        
    if(p[0] == ','){
      if (!parse_attr_GGA(attr, p-attr, attrIndex, &data))
        break;
     // p[0] = '\0';
      p++;
      attr=p;    
      attrIndex ++;
    }else
      p++;
  }

#if defined(RESOLVED_DATA_TYPE)
  if(data <= ((resolved_data_t *)pack)->data){ 
      return false; // <4 haven't get datas
  }else{
    ((resolved_data_t *)pack)->len = data -  ((resolved_data_t *)pack)->data;
    return true;
  }
#else
  if(((data_store_t *)data)->type != 0)
    return true;
  else
    return false;
#endif  
}


