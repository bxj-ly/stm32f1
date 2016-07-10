#ifndef __OBD_H
#define  __OBD_H

#include "stm32f10x.h"
#include "stm32f10x_can.h"

#define CANx                        CAN2
#define CAN_500K                    4
#define CAN_250K                    8
#define CAN_125K                    16
#define RCC_APBxPeriph_CAN_IO       RCC_APB2Periph_GPIOB
#define CAN_RXD                     GPIO_Pin_12
#define CAN_TXD                     GPIO_Pin_13
#define CAN_IO                      GPIOB
#define CAN_PinRemap                DISABLE
#define CAN_IRQHandler              CAN2_RX0_IRQHandler

void CAN_Config(uint8_t velocity, uint8_t can_id_type);
char* CAN_ReadDTC(CanTxMsg* DTCCmd,ErrorStatus* err);
uint8_t* Send_CANFrame(CanTxMsg* TxMessage,ErrorStatus* err);
char* PCBU(uint16_t dtc);
void ClearRAM(uint8_t* ram,uint32_t n);

typedef enum 
{
    OBD_PROTOCOL_UNKNOWN    = 0,
    OBD_ISO14230_4ADDR      = 1,
    OBD_ISO14230_4HL        = 2,
    OBD_ISO9141_2ADDR       = 3,
    OBD_ISO15765_4STD_500K  = 4, 
    OBD_ISO15765_4EXT_500K  = 5, 
    OBD_ISO15765_4STD_250K  = 6, 
    OBD_ISO15765_4EXT_250K  = 7
}OBD_PROTOCOL_E;
void OBD_ProtocolDetect(ErrorStatus *err);
OBD_PROTOCOL_E OBD_GetProtocol(void);

typedef enum {
    OBD_DS_DTC_CNT      = 00,  /* DTC Count */
    OBD_DS_RPM          = 48,  /* engine Revolutions Per Minute */
    OBD_DS_VSS          = 49,  /* Vehicle Speed Sensor */
    OBD_DS_ECT          = 41,  /* Engine Coolant Temperature */
    OBD_DS_MAF          = 52,  /* Mass Air Flow */
    OBD_DS_MAP          = 47,  /* Manifold Absolute Pressure */
    OBD_DS_TP           = 53,  /* Throttle Positioner */
    OBD_DS_O2B1S1       = 56,  /* O2 sensor B1S1 */
    OBD_DS_PCT          = 40,  /* Load Percent */
} OBD_DS_E;
char* OBD_ReadDS(OBD_DS_E cmd, ErrorStatus* err);
void OBD_KeepLink(void);
char* OBD_ReadDTC(ErrorStatus* err);
void OBD_CleanUpDTC(ErrorStatus* err);

#endif
