#ifndef __OBD_H
#define  __OBD_H

#include "stm32f10x.h"
#include "stm32f10x_can.h"

#define CANx                         CAN2
#define CAN_500K                     4
#define CAN_250K                     8
#define CAN_125K                     16
#define RCC_APBxPeriph_CAN_IO        RCC_APB2Periph_GPIOB
#define CAN_RXD					     GPIO_Pin_12
#define CAN_TXD						 GPIO_Pin_13
#define CAN_IO						 GPIOB
#define CAN_PinRemap				 DISABLE
#define CAN_IRQHandler               CAN2_RX0_IRQHandler

void CAN_Config(uint8_t velocity, uint8_t can_id_type);
char* CAN_ReadDTC(CanTxMsg* DTCCmd,ErrorStatus* err);
uint8_t* Send_CANFrame(CanTxMsg* TxMessage,ErrorStatus* err);
char* PCBU(uint16_t dtc);
void ClearRAM(uint8_t* ram,uint32_t n);


#endif
