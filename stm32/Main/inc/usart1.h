#ifndef __USART1_H
#define	__USART1_H

#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include <stdio.h>

#define UART1_DMA_RCVBUFFER_SIZE 1024
extern uint8_t uart1_dma_receivebuffer[];

void USART1_Config(void);
void USART1_DMA_Config(void);

int USART1_TX_DMA_Send(void * src, size_t len);
int fputc(int ch, FILE *f);
void USART1_RX_MSG_Proc(void);
#endif /* __USART1_H */
