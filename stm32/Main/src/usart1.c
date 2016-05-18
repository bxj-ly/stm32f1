/*******************************************************************************
 * Copyright (C) 2016 Xuejun Bian, Yu Lu. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom 
 * the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software. 
 
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ******************************************************************************/
#include <stdarg.h>
#include <string.h>
#include "usart1.h"
#include "uart4.h"
#include "debug.h"

#define UART1_DMA_RCVBUFFER_SIZE 1024
#define UART1_DMA_SENDBUFF_SIZE 1024 
static uint8_t uart1_dma_receivebuffer[UART1_DMA_RCVBUFFER_SIZE];
static uint8_t uart1_dma_sendbuffer[UART1_DMA_SENDBUFF_SIZE];
static uint32_t dma15_len = 0;

static void USART1_TX_DMA_INIT(void);
static void USART1_RX_DMA_INIT(void);

/* USART1 Config
- BaudRate = 115200 baud
- Word Length = 8 Bits
- One Stop Bit 
- No parity 
- Hardware flow control disabled (RTS and CTS signals)  
- Receive and transmit enabled 
*/
void USART1_Config(void)
{
  USART_InitTypeDef USART_InitStructure;

 
  GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);  

  /* USART1 mode config */
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No ;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART1, &USART_InitStructure); 
  //USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
  USART_Cmd(USART1, ENABLE);

  USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);

}

void USART1_DMA_Config(void)
{
	USART1_TX_DMA_INIT();
	USART1_RX_DMA_INIT();
}

void USART1_TX_DMA_SEND(void * src, size_t len)
{
  memcpy(uart1_dma_sendbuffer, src, len);
  DMA_SetCurrDataCounter(DMA1_Channel4, len);
  DMA_Cmd (DMA1_Channel4,ENABLE);		
}

int fputc(int ch, FILE *f)
{
  while (!(USART1->SR & USART_FLAG_TXE));

  USART_SendData(USART1, (unsigned char) ch);
  while (!(USART1->SR & USART_FLAG_TXE)); 
  return (ch);
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/
void USART1_IRQHandler(void)
{
  uint8_t c;
  DEBUG_SetUSART1IntCnt();

  if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
  {
    c=USART1->SR;
    c=USART1->DR;
         
    dma15_len = DMA_GetCurrDataCounter(DMA1_Channel5);
    if(DEBUG_GetPlayMode() == SYS_MODE_ECHO){
      USART1_TX_DMA_SEND(uart1_dma_receivebuffer, (size_t)(UART1_DMA_RCVBUFFER_SIZE - dma15_len));
    }
    UART4_TX_DMA_SEND(uart1_dma_receivebuffer, (size_t)(UART1_DMA_RCVBUFFER_SIZE - dma15_len));
    
    DMA_Cmd(DMA1_Channel5, DISABLE); 
    DMA_SetCurrDataCounter(DMA1_Channel5, UART1_DMA_RCVBUFFER_SIZE);
    DMA_Cmd(DMA1_Channel5, ENABLE); 
  
  }
  
  if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
  {   
    c=USART1->DR;
#if 0		
    if(c==':') play_stage = 1;
    else if (c=='>' && play_stage == 1) play_stage = 2;
    else if(play_stage == 2 && c >= '0' && c < '9'){
      play_mode |= (uint32_t)0x00000001 << (c - '0');
      play_stage += 2;
    }
    else if (c=='<' && play_stage == 1) play_stage = 3;
    else if(play_stage == 3 && c >= '0' && c < '9'){
      play_mode &= ~((uint32_t)0x00000001 << (c - '0'));
      play_stage += 2;
    }
    else
      play_stage = 0;
#endif
    
    dma15_len = DMA_GetCurrDataCounter(DMA1_Channel5);
    //INFO("%c",c);
  }

}

void DMA1_Channel4_IRQHandler(void)
{	
	DEBUG_SetDMA1Channel4IntCnt();
	if(DMA_GetFlagStatus(DMA1_FLAG_TC4)==SET) 
	{  
		DMA_ClearFlag(DMA1_FLAG_TC4);
		DMA_Cmd (DMA1_Channel4,DISABLE);			
	}	
}

void DMA1_Channel5_IRQHandler(void)
{	
	DEBUG_SetDMA1Channel5IntCnt();
	if(DMA_GetFlagStatus(DMA1_FLAG_TC5)==SET) 
	{  		
		DMA_ClearFlag(DMA1_FLAG_TC5); 
	}	
	if(DMA_GetFlagStatus(DMA1_FLAG_HT5)==SET) 
	{  
		DMA_ClearFlag(DMA1_FLAG_HT5); 
	}	
	if(DMA_GetFlagStatus(DMA1_FLAG_TE5)==SET) 
	{  
		DMA_ClearFlag(DMA1_FLAG_TE5); 
	}		
}

static void USART1_TX_DMA_INIT(void)
{
	DMA_InitTypeDef DMA_InitStructure;

	DMA_DeInit(DMA1_Channel4); 
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;	   
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)uart1_dma_sendbuffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;	
	DMA_InitStructure.DMA_BufferSize = UART1_DMA_SENDBUFF_SIZE;   
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;	
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;	 
	DMA_InitStructure.DMA_Mode =  DMA_Mode_Normal ;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;  
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;   
	DMA_Init(DMA1_Channel4, &DMA_InitStructure); 	
	DMA_Cmd (DMA1_Channel4,DISABLE);
				
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);		
		
	DMA_ITConfig(DMA1_Channel4,DMA_IT_TC,ENABLE);  
}

static void USART1_RX_DMA_INIT(void)
{
	DMA_InitTypeDef DMA_InitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
  DMA_DeInit(DMA1_Channel5); 
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR; 
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)uart1_dma_receivebuffer;  
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; 
  DMA_InitStructure.DMA_BufferSize = UART1_DMA_RCVBUFFER_SIZE; 
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
  DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte; 
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; 
  DMA_InitStructure.DMA_Priority = DMA_Priority_High; 
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel5, &DMA_InitStructure);   
  DMA_Cmd(DMA1_Channel5, ENABLE); 	
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC | DMA_IT_HT , ENABLE); 	
}

