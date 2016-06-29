#ifndef __ISO15765_4_H
#define __ISO15765_4_H

ErrorStatus ISO15765_4_ProtocolDetect(void);

typedef enum {
    ISO15765_4STD_500K = 0, 
    ISO15765_4EXT_500K = 1, 
    ISO15765_4STD_250K = 3, 
    ISO15765_4EXT_250K = 4
} ISO15765_4_TYPE;
void ISO15765_4_Config(ISO15765_4_TYPE type);

char* ISO15765_4_ReadDTC(ErrorStatus* err);

typedef enum {
    CAN_DTC_CN = 00,  /* DTC Count */
    CAN_RPM = 48,     /* engine Revolutions Per Minute */
    CAN_VSS = 49,     /* Vehicle Speed Sensor */
    CAN_ECT = 41,     /* Engine Coolant Temperature */
    CAN_MAF = 52,     /* Mass Air Flow */
    CAN_MAP = 47,     /* Manifold Absolute Pressure */
    CAN_TP  = 53,     /* Throttle Positioner */
    CAN_O2B1S1 = 56,  /* O2 sensor B1S1 */
    CAN_LOAD_PCT = 40 /* Load Percent */

} ISO15765_4_DS;
void ISO15765_4_CleanUpDTC(ErrorStatus* err);
char* ISO15765_4_ReadDS(ISO15765_4_DS cmd, ErrorStatus* err);

#endif


