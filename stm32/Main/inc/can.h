#ifndef __CAN_H
#define  __CAN_H

#include "stm32f10x.h"
#include "stm32f10x_can.h"

typedef enum
{
CAN_RPM = 0x0C,     /* engine Revolutions Per Minute */
CAN_VSS = 0x0D,     /* Vehicle Speed Sensor */
CAN_ECT = 0x05,     /* Engine Coolant Temperature */
CAN_MAF = 0x10,     /* Mass Air Flow */
CAN_MAP = 0x0B,     /* Manifold Absolute Pressure */
CAN_TP  = 0x11,     /* Throttle Positioner */
CAN_O2B1S1 = 0x14,  /* O2 sensor B1S1 */
CAN_LOAD_PCT = 0x04 /* Load Percent */
}E_CAN_CMD;

void CAN_Config(void);
void CAN_CheckStatus(E_CAN_CMD Cmd);


#endif
