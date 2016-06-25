/*
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

#ifndef __NMEA_LIB_H__
#define __NMEA_LIB_H__

#include <stdbool.h>
#include <stddef.h>
#include "stm32f10x.h"
/**
 * @file
 * The table below describes which fields are present in the sentences that are
 * supported by the library.
 <pre>
 field/sentence       GPGGA   GPGSA   GPGSV   GPRMC   GPVTG
 present:               x       x       x       x       x
 smask:                 x       x       x       x       x
 utc (date):                                    x
 utc (time):            x                       x
 sig:                   x                       x1
 fix:                           x               x1
 PDOP:                          x
 HDOP:                  x       x
 VDOP:                          x
 lat:                   x                       x
 lon:                   x                       x
 elv:                   x
 speed:                                         x       x
 track:                                         x       x
 mtrack:                                                x
 magvar:                                        x
 satinfo (inuse count): x       x1
 satinfo (inuse):               x
 satinfo (inview):                      x

 x1 = not present in the sentence but the library sets it up.
 </pre>
 */

#define NMEA_SIG_FIRST (NMEA_SIG_BAD)
#define NMEA_SIG_BAD   (0)
#define NMEA_SIG_LOW   (1)
#define NMEA_SIG_MID   (2)
#define NMEA_SIG_HIGH  (3)
#define NMEA_SIG_RTKIN (4)
#define NMEA_SIG_FLRTK (5)
#define NMEA_SIG_ESTIM (6)
#define NMEA_SIG_MAN   (7)
#define NMEA_SIG_SIM   (8)
#define NMEA_SIG_LAST  (NMEA_SIG_SIM)

#define NMEA_FIX_FIRST (NMEA_FIX_BAD)
#define NMEA_FIX_BAD   (1)
#define NMEA_FIX_2D    (2)
#define NMEA_FIX_3D    (3)
#define NMEA_FIX_LAST  (NMEA_FIX_3D)

#define NMEA_MAXSAT    (64)
#define NMEA_SATINPACK (4)
#define NMEA_NSATPACKS (NMEA_MAXSAT / NMEA_SATINPACK)

#define NMEA_DEF_LAT   (0.0)
#define NMEA_DEF_LON   (0.0)


 

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * Date and time data
 * @see nmea_time_now
 */
typedef struct _nmeaTIME {
  int year;           /**< Years since 1900 */
  int mon;            /**< Months since January - [0,11] */
  int day;            /**< Day of the month - [1,31] */
  int hour;           /**< Hours since midnight - [0,23] */
  int min;            /**< Minutes after the hour - [0,59] */
  int sec;            /**< Seconds after the minute - [0,59] */
  int hsec;           /**< Hundredth part of second - [0,99] */
} nmeaTIME;

/**
 * Position data in fractional degrees or radians
 */
typedef struct _nmeaPOS {
  double lat;           /**< Latitude */
  double lon;           /**< Longitude */
} nmeaPOS;

/**
 * Information about satellite
 * @see nmeaSATINFO
 * @see nmeaGPGSV
 */
typedef struct _nmeaSATELLITE {
  int id;             /**< Satellite PRN number */
  int elv;            /**< Elevation in degrees, 90 maximum */
  int azimuth;          /**< Azimuth, degrees from true north, 000 to 359 */
  int sig;            /**< Signal, 00-99 dB */
} nmeaSATELLITE;

/**
 * Information about all satellites in view
 * @see nmeaINFO
 * @see nmeaGPGSV
 */
typedef struct _nmeaSATINFO {
  int inuse;            /**< Number of satellites in use (not those in view) */
  int in_use[NMEA_MAXSAT];    /**< IDs of satellites in use (not those in view) */
  int inview;           /**< Total number of satellites in view */
  nmeaSATELLITE sat[NMEA_MAXSAT]; /**< Satellites information (in view) */
} nmeaSATINFO;

/**
 * Summary GPS information from all parsed packets,
 * used also for generating NMEA stream
 * @see nmea_parse
 * @see nmea_GPGGA2info,  nmea_...2info
 */
typedef struct _nmeaINFO {
  u32 present;       /**< Mask specifying which fields are present */

  int smask;            /**< Mask specifying from which sentences data has been obtained */

  nmeaTIME utc;         /**< UTC of position */

  int sig;            /**< GPS quality indicator: 0 = Invalid
                                                              1 = Fix;
                                          2 = Differential
                                          3 = Sensitive
                                          4 = Real Time Kinematic
                                          5 = Float RTK,
                                          6 = estimated (dead reckoning) (v2.3)
                                          7 = Manual input mode
                                          8 = Simulation mode) */

  int fix;            /**< Operating mode, used for navigation: 1 = Fix not available
                                                                            2 = 2D
                                                                            3 = 3D) */

  double PDOP;          /**< Position Dilution Of Precision */
  double HDOP;          /**< Horizontal Dilution Of Precision */
  double VDOP;          /**< Vertical Dilution Of Precision */

  double lat;           /**< Latitude in NDEG:  +/-[degree][min].[sec/60] */
  double lon;           /**< Longitude in NDEG: +/-[degree][min].[sec/60] */
  double elv;           /**< Antenna altitude above/below mean sea level (geoid) in meters */
  double speed;         /**< Speed over the ground in kph */
  double track;         /**< Track angle in degrees True */
  double mtrack;          /**< Magnetic Track angle in degrees True */
  double magvar;          /**< Magnetic variation degrees */

  nmeaSATINFO satinfo;      /**< Satellites information */
} nmeaINFO;





/**
 * NMEA packets type which parsed and generated by library
 */
enum nmeaPACKTYPE {
  GPNON = 0,      /**< Unknown packet type. */
  GPGGA = (1u << 0),  /**< GGA - Essential fix data which provide 3D location and accuracy data. */
  GPGSA = (1u << 1),  /**< GSA - GPS receiver operating mode, SVs used for navigation, and DOP values. */
  GPGSV = (1u << 2),  /**< GSV - Number of SVs in view, PRN numbers, elevation, azimuth & SNR values. */
  GPRMC = (1u << 3),  /**< RMC - Recommended Minimum Specific GPS/TRANSIT Data. */
  GPVTG = (1u << 4) /**< VTG - Actual track made good and speed over ground. */
};

/**
 * GGA packet information structure (Global Positioning System Fix Data)
 *
 * <pre>
 * GGA - essential fix data which provide 3D location and accuracy data.
 *
 * $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
 *
 * Where:
 *      GGA          Global Positioning System Fix Data
 *      123519       Fix taken at 12:35:19 UTC
 *      4807.038,N   Latitude 48 deg 07.038' N
 *      01131.000,E  Longitude 11 deg 31.000' E
 *      1            Signal quality: 0 = invalid
 *                                   1 = GPS fix (SPS)
 *                                   2 = DGPS fix
 *                                   3 = PPS fix
 *                               4 = Real Time Kinematic
 *                               5 = Float RTK
 *                                   6 = estimated (dead reckoning) (2.3 feature)
 *                               7 = Manual input mode
 *                               8 = Simulation mode
 *      08           Number of satellites being tracked
 *      0.9          Horizontal dilution of position
 *      545.4,M      Altitude, Meters, above mean sea level
 *      46.9,M       Height of geoid (mean sea level) above WGS84
 *                       ellipsoid
 *      (empty field) time in seconds since last DGPS update
 *      (empty field) DGPS station ID number
 *      *47          the checksum data, always begins with *
 *
 * If the height of geoid is missing then the altitude should be suspect. Some
 * non-standard implementations report altitude with respect to the ellipsoid
 * rather than geoid altitude. Some units do not report negative altitudes at
 * all. This is the only sentence that reports altitude.
 * </pre>
 */
typedef struct _nmeaGPGGA {
  u32 present;     /**< Mask specifying which fields are present, same as in nmeaINFO */
  nmeaTIME utc;       /**< UTC of position (just time) */
  double lat;         /**< Latitude in NDEG - [degree][min].[sec/60] */
  char ns;          /**< [N]orth or [S]outh */
  double lon;         /**< Longitude in NDEG - [degree][min].[sec/60] */
  char ew;          /**< [E]ast or [W]est */
  int sig;          /**< GPS quality indicator (0 = Invalid; 1 = Fix; 2 = Differential, 3 = Sensitive) */
  int satinuse;       /**< Number of satellites in use (not those in view) */
  double HDOP;        /**< Horizontal dilution of precision */
  double elv;         /**< Antenna altitude above/below mean sea level (geoid) */
  char elv_units;       /**< [M]eters (Antenna height unit) */
  double diff;        /**< Geoidal separation (Diff. between WGS-84 earth ellipsoid and mean sea level. '-' = geoid is below WGS-84 ellipsoid) */
  char diff_units;      /**< [M]eters (Units of geoidal separation) */
  double dgps_age;      /**< Time in seconds since last DGPS update */
  int dgps_sid;       /**< DGPS station ID number */
} nmeaGPGGA;

/**
 * GSA packet information structure (Satellite status)
 *
 * <pre>
 * GSA - GPS DOP and active satellites.
 *
 * This sentence provides details on the nature of the fix. It includes the
 * numbers of the satellites being used in the current solution and the DOP.
 *
 * DOP (dilution of precision) is an indication of the effect of satellite
 * geometry on the accuracy of the fix. It is a unitless number where smaller
 * is better. For 3D fixes using 4 satellites a 1.0 would be considered to be
 * a perfect number, however for overdetermined solutions it is possible to see
 * numbers below 1.0.
 *
 * There are differences in the way the PRN's are presented which can effect the
 * ability of some programs to display this data. For example, in the example
 * shown below there are 5 satellites in the solution and the null fields are
 * scattered indicating that the almanac would show satellites in the null
 * positions that are not being used as part of this solution. Other receivers
 * might output all of the satellites used at the beginning of the sentence with
 * the null field all stacked up at the end. This difference accounts for some
 * satellite display programs not always being able to display the satellites
 * being tracked. Some units may show all satellites that have ephemeris data
 * without regard to their use as part of the solution but this is non-standard.
 *
 * $GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39
 *
 * Where:
 *      GSA      Satellite status
 *      A        Auto selection of 2D or 3D fix (M = manual)
 *      3        3D fix - values include: 1 = no fix
 *                                        2 = 2D fix
 *                                        3 = 3D fix
 *      04,05... PRNs of satellites used for fix (space for 12)
 *      2.5      PDOP (dilution of precision)
 *      1.3      Horizontal dilution of precision (HDOP)
 *      2.1      Vertical dilution of precision (VDOP)
 *      *39      the checksum data, always begins with *
 * </pre>
 */
typedef struct _nmeaGPGSA {
  u32 present;     /**< Mask specifying which fields are present, same as in nmeaINFO */
  char fix_mode;        /**< Mode (M = Manual, forced to operate in 2D or 3D; A = Automatic, 3D/2D) */
  int fix_type;       /**< Type, used for navigation (1 = Fix not available; 2 = 2D; 3 = 3D) */
  int sat_prn[NMEA_MAXSAT]; /**< PRNs of satellites used in position fix (0 for unused fields) */
  double PDOP;        /**< Dilution of precision */
  double HDOP;        /**< Horizontal dilution of precision */
  double VDOP;        /**< Vertical dilution of precision */
} nmeaGPGSA;

/**
 * GSV packet information structure (Satellites in view)
 *
 * <pre>
 * GSV - Satellites in View
 *
 * Shows data about the satellites that the unit might be able to find based on
 * its viewing mask and almanac data. It also shows current ability to track
 * this data. Note that one GSV sentence only can provide data for up to 4
 * satellites and thus there may need to be 3 sentences for the full
 * information. It is reasonable for the GSV sentence to contain more satellites
 * than GGA might indicate since GSV may include satellites that are not used as
 * part of the solution. It is not a requirement that the GSV sentences all
 * appear in sequence. To avoid overloading the data bandwidth some receivers
 * may place the various sentences in totally different samples since each
 * sentence identifies which one it is.
 *
 * The field called SNR (Signal to Noise Ratio) in the NMEA standard is often
 * referred to as signal strength. SNR is an indirect but more useful value than
 * raw signal strength. It can range from 0 to 99 and has units of dB according
 * to the NMEA standard, but the various manufacturers send different ranges of
 * numbers with different starting numbers so the values themselves cannot
 * necessarily be used to evaluate different units. The range of working values
 * in a given gps will usually show a difference of about 25 to 35 between the
 * lowest and highest values, however 0 is a special case and may be shown on
 * satellites that are in view but not being tracked.
 *
 * $GPGSV,2,1,08,01,40,083,46,02,17,308,41,12,07,344,39,14,22,228,45*75
 *
 * Where:
 *      GSV          Satellites in view
 *      2            Number of sentences for full data
 *      1            sentence 1 of 2
 *      08           Number of satellites in view
 *
 *      01           Satellite PRN number
 *      40           Elevation, degrees
 *      083          Azimuth, degrees
 *      46           SNR - higher is better
 *           for up to 4 satellites per sentence
 *
 *      *75          the checksum data, always begins with *
 * </pre>
 */
typedef struct _nmeaGPGSV {
  u32 present;     /**< Mask specifying which fields are present, same as in nmeaINFO */
  int pack_count;       /**< Total number of messages of this type in this cycle */
  int pack_index;       /**< Message number */
  int sat_count;        /**< Total number of satellites in view */
  nmeaSATELLITE sat_data[NMEA_SATINPACK];
} nmeaGPGSV;

/**
 * RMC -packet information structure (Recommended Minimum sentence C)
 *
 * <pre>
 * RMC - Recommended Minimum sentence C
 *
 * NMEA has its own version of essential gps pvt (position, velocity,
 * time) data. It is called RMC, the Recommended Minimum, which will look
 * similar to:
 *
 * $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
 * $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W,A*6A (v2.3)
 *
 * Where:
 *      RMC          Recommended Minimum sentence C
 *      123519       Fix taken at 12:35:19 UTC
 *      A            Status A=active or V=Void.
 *      4807.038,N   Latitude 48 deg 07.038' N
 *      01131.000,E  Longitude 11 deg 31.000' E
 *      022.4        Speed over the ground in knots
 *      084.4        Track angle in degrees True
 *      230394       Date - 23rd of March 1994
 *      003.1,W      Magnetic Variation
 *      A            Mode A=autonomous, D=differential, E=Estimated,
 *                        N=not valid, S=Simulator (NMEA v2.3)
 *      *6A          The checksum data, always begins with *
 * </pre>
 */
typedef struct _nmeaGPRMC {
  u32 present;     /**< Mask specifying which fields are present, same as in nmeaINFO */
  nmeaTIME utc;       /**< UTC of position */
  char status;        /**< Status (A = active or V = void) */
  double lat;         /**< Latitude in NDEG - [degree][min].[sec/60] */
  char ns;          /**< [N]orth or [S]outh */
  double lon;         /**< Longitude in NDEG - [degree][min].[sec/60] */
  char ew;          /**< [E]ast or [W]est */
  double speed;       /**< Speed over the ground in knots */
  double track;       /**< Track angle in degrees True */
  double magvar;        /**< Magnetic variation degrees (Easterly var. subtracts from true course) */
  char magvar_ew;       /**< [E]ast or [W]est */
  char mode;          /**< Mode indicator of fix type (A=autonomous, D=differential, E=Estimated, N=not valid, S=Simulator) */
} nmeaGPRMC;

/**
 * VTG packet information structure (Track made good and ground speed)
 *
 * <pre>
 * VTG - Velocity made good.
 *
 * The gps receiver may use the LC prefix instead of GP if it is emulating
 * Loran output.
 *
 * $GPVTG,054.7,T,034.4,M,005.5,N,010.2,K*48
 *
 * where:
 *      VTG          Track made good and ground speed
 *      054.7,T      True track made good (degrees)
 *      034.4,M      Magnetic track made good
 *      005.5,N      Ground speed, knots
 *      010.2,K      Ground speed, Kilometers per hour
 *      *48          Checksum
 * </pre>
 */
typedef struct _nmeaGPVTG {
  u32 present;     /**< Mask specifying which fields are present, same as in nmeaINFO */
  double track;       /**< True track made good (degrees) */
  char track_t;       /**< Fixed text 'T' indicates that track made good is relative to true north */
  double mtrack;        /**< Magnetic track made good */
  char mtrack_m;        /**< Fixed text 'M' */
  double spn;         /**< Ground speed, knots */
  char spn_n;         /**< Fixed text 'N' indicates that speed over ground is in knots */
  double spk;         /**< Ground speed, kilometers per hour */
  char spk_k;         /**< Fixed text 'K' indicates that speed over ground is in kilometers/hour */
} nmeaGPVTG;


//#pragma pack(push) 
//#pragma pack(1)
/**
 * time data
 * 
 */
typedef struct {
  u8 hour;           /** Hours since midnight - [0,23] */
  u8 min;            /** Minutes after the hour - [0,59] */
  u8 sec;            /** Seconds after the minute - [0,59] */
  u8 secp;           /** Hundredth part of second - [0,99] */
}nmea_time_t;
/**
 * location data
 * degree minute
 */
typedef struct {
  u8 deg;           /** degree - 0~180 */
  u8 min;           /** minute of degree - 0-60 */
  u8 minp1;         /** .xx of minute - 0-100*/
  u8 minp2;         /** .00xx of minute - 0-100 */
}nmea_loc_t;
/**
 * info data
 * lat north south ; lon east west; signal quality
 */
typedef struct{
  u8 ns:1;
  u8 ew:1;
  u8 signal:3;
}data_flag_t;
/**
 * data
 * this board needs
 */
typedef struct{
  u16 type;         /** type of data */
  u16 len;          /** len of this data , here to end */
  data_flag_t flag; 
  nmea_time_t time;
  nmea_loc_t lat;
  nmea_loc_t lon;
}data_store_t;
//#pragma pack(pop)

#define RESOLVED_GGA_DATA_TYPE          0x3150


/**
 * Parse info from a string.
 *
 * @param attr the string of time info
 * @param len the length of the string
 * @param index info the attr index of this line
 * @param data output struct's point
 * @return true on success, false otherwise
 */
typedef bool (* attr_GGA_handler)(u8 *attr, unsigned int len, u8 index, u8 ** data);

typedef struct {
  u16 type;
  u16 len;
  u8 data[];
}resolved_data_t;


extern bool nmea_parse_GGA(u8 *s, const int len, bool has_checksum, u8 *pack);


#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __NMEA_LIB_H__ */
