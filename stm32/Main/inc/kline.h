#ifndef __KLINE_H
#define __KLINE_H
#include "stm32f10x.h"
#include <stdio.h>
#include "obd.h"


extern u8 recvData;
extern u8 KrecvData;
extern u8 kline_input_flag;

void KLINE_GPIO_Config(void);
u8 ISO14230_4ADDR_start(u8 KSW[2]);
u8 ISO14230_4HL_start(u8 KSW[7]);
u8 ISO9141_2ADDR_start(u8 KSW[2]);	
u8 ISO_14230_LINK_KEEP(u8 KSW[7]);
u8 ISO_14230_DATA_READ(u8 enqcode, u8 RD[255]);
u8 ISO_9141_2_LINK_KEEP(u8 KSW[7]);
u8 ISO_9141_2_DATA_READ(u8 enqcode,u8 RD[255]);
u8 ISO_14230_DTC_READ(u8 RD[255]);
u8 ISO_9141_2_DTC_READ(u8 RD[255]) ;
u8 ISO_14230_DTC_CLEAR(u8 KSW[7]);
u8 ISO_9141_2_DTC_CLEAR(u8 KSW[7]);

OBD_PROTOCOL_E KLINE_ProtocolDetect(ErrorStatus *err);
char* ISO14230_4ADDR_ReadDS(OBD_DS_E cmd, ErrorStatus* err);
char* ISO9141_2ADDR_ReadDS(OBD_DS_E cmd, ErrorStatus* err);

#endif  /*__BT_H*/
