#ifndef __USART1_H
#define	__USART1_H

#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include <stdio.h>

void USART1_Config(void);
void USART1_DMA_Config(void);
int USART1_TX_DMA_SEND(void * src, size_t len);
int fputc(int ch, FILE *f);

#endif /* __USART1_H */
