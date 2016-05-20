#ifndef __UART4_H
#define	__UART4_H

#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include <stdio.h>

#define UART4_DMA_RCVBUFFER_SIZE 1024
extern uint8_t uart4_dma_receivebuffer[];

void UART4_Config(void);
void UART4_DMA_Config(void);

void UART4_TX_DMA_Send(void * src, size_t len);
void UART4_SendByte(uint8_t data);
void UART4_RX_MSG_Proc(void);

#endif /* __UART4_H */
