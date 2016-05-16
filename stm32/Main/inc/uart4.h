#ifndef __UART4_H
#define	__UART4_H

#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include <stdio.h>

void UART4_Config(void);
void UART4_DMA_Config(void);
void UART4_TX_DMA_SEND(void * src, size_t len);
void UART4_SendByte(uint8_t data);


#endif /* __UART4_H */
