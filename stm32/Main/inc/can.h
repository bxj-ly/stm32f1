#ifndef __CAN_H
#define  __CAN_H

#include "stm32f10x.h"
#include "stm32f10x_can.h"

#define CAN_CHK_STDID        0x07df
#define CAN_RESP_STDID       0x07E8

#define CAN_CHK_CMD_LEN      2
#define CAN_CHK_CMD_H        0x01
#define CAN_CHK_RESP_H       0x04

#define CAN_RPM              0x0C
#define CAN_VVS              0x0D
#define CAN_ECT              0x05
#define CAN_MAF              0x10
#define CAN_MAP              0x0B
#define CAN_TP               0x11
#define CAN_O2B1S1           0x14
#define CAN_LOAD_PCT         0x04

static void CAN_GPIO_Config(void);
static void CAN_NVIC_Config(void);
static void CAN_Mode_Config(void);
static void CAN_Filter_Config(void);
void CAN_Config(void);
void CAN_SetMsg(uint8_t Cmd);

extern  CanTxMsg TxMessage;
extern  CanRxMsg RxMessage;
extern __IO uint8_t RxMsgReceived;
#endif
