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
#include <string.h>
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_dma.h"
#include "uart4.h"
#include "usart1.h"
#include "debug.h"
#include "gsm.h"

#define UART4_DMA_RCVBUFFER_SIZE 1024
#define UART4_DMA_SENDBUFF_SIZE 1024 
static uint8_t uart4_dma_receivebuffer[UART4_DMA_RCVBUFFER_SIZE];
static uint8_t uart4_dma_sendbuffer[UART4_DMA_SENDBUFF_SIZE];

static void UART4_TX_DMA_INIT(void);
static void UART4_RX_DMA_INIT(void);

/* UART4 Config
- BaudRate = 115200 baud
- Word Length = 8 Bits
- One Stop Bit 
- No parity 
- Hardware flow control disabled (RTS and CTS signals)  
- Receive and transmit enabled 
*/

void UART4_Config(void)
{
  USART_InitTypeDef USART_InitStructure;
	
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No ;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(UART4, &USART_InitStructure); 
//  USART_ITConfig(UART4,USART_IT_RXNE, ENABLE);
  USART_Cmd(UART4, ENABLE);

  USART_ITConfig(UART4, USART_IT_IDLE, ENABLE);
}

void UART4_DMA_Config(void)
{
	UART4_TX_DMA_INIT();
	UART4_RX_DMA_INIT();
}

void UART4_SendByte(uint8_t data)
{
   /* wart for transmit data register empty */
   while (!(UART4->SR & USART_FLAG_TXE));
   /* send data */
   UART4->DR = (data & 0x00FF);
   /* wait for transmission compelete */
   while (!(UART4->SR & USART_FLAG_TC));   
}

void UART4_TX_DMA_SEND(void * src, size_t len)
{
		memcpy(uart4_dma_sendbuffer, src, len);
		DMA_SetCurrDataCounter(DMA2_Channel5, len);
		DMA_Cmd (DMA2_Channel5,ENABLE);		
}

void UART4_IRQHandler(void)
{
	uint8_t c;
	uint16_t rec_len = 0;
	DEBUG_SetUART4IntCnt();

	if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)
	{ 	
		if(DEBUG_GetPlayMode() & SYS_MODE_MON4)
		{
	    c=UART4->DR;
	  	INFO("%c",c);    
		}
	} 
	if(USART_GetITStatus(UART4, USART_IT_IDLE) != RESET)
	{
		c=UART4->SR;
		c=UART4->DR;
	
		rec_len = DMA_GetCurrDataCounter(DMA2_Channel3);
    GSM_RetrieveData(uart4_dma_receivebuffer, (size_t)(UART4_DMA_RCVBUFFER_SIZE - rec_len));
    if(DEBUG_GetPlayMode() == SYS_MODE_ECHO){
		  USART1_TX_DMA_SEND(uart4_dma_receivebuffer, (size_t)(UART4_DMA_RCVBUFFER_SIZE - rec_len));
    }    
		DMA_Cmd(DMA2_Channel3, DISABLE); 
		DMA_SetCurrDataCounter(DMA2_Channel3, UART4_DMA_RCVBUFFER_SIZE);
		DMA_Cmd(DMA2_Channel3, ENABLE); 		

	}
}

void DMA2_Channel5_IRQHandler(void)
{	
	DEBUG_SetDMA2Channel5IntCnt();
	if(DMA_GetFlagStatus(DMA2_FLAG_TC5)==SET) 
	{  
			
		DMA_ClearFlag(DMA2_FLAG_TC5);
		DMA_Cmd (DMA2_Channel5,DISABLE);			
	}	
}

void DMA2_Channel3_IRQHandler(void)
{	
	DEBUG_SetDMA2Channel3IntCnt();
	if(DMA_GetFlagStatus(DMA2_FLAG_TC3)==SET) 
	{  		
		DMA_ClearFlag(DMA2_FLAG_TC3); 
	}	
	if(DMA_GetFlagStatus(DMA2_FLAG_HT3)==SET) 
	{  
		DMA_ClearFlag(DMA2_FLAG_HT3); 
	}	
	if(DMA_GetFlagStatus(DMA2_FLAG_TE3)==SET) 
	{  
		DMA_ClearFlag(DMA2_FLAG_TE3); 
	}		
}

static void UART4_TX_DMA_INIT(void)
{
	DMA_InitTypeDef DMA_InitStructure;

	DMA_DeInit(DMA2_Channel5); 
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART4->DR;	   
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)uart4_dma_sendbuffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;	
	DMA_InitStructure.DMA_BufferSize = UART4_DMA_SENDBUFF_SIZE;   
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;	
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;	 
	DMA_InitStructure.DMA_Mode =  DMA_Mode_Normal ;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;  
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;   
	DMA_Init(DMA2_Channel5, &DMA_InitStructure); 	
	DMA_Cmd (DMA2_Channel5,DISABLE);
				
	USART_DMACmd(UART4, USART_DMAReq_Tx, ENABLE);		
		
	DMA_ITConfig(DMA2_Channel5,DMA_IT_TC,ENABLE);  
}



static void UART4_RX_DMA_INIT(void)
{
	DMA_InitTypeDef DMA_InitStructure;

  DMA_DeInit(DMA2_Channel3); 
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART4->DR; 
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)uart4_dma_receivebuffer;  
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; 
  DMA_InitStructure.DMA_BufferSize = UART4_DMA_RCVBUFFER_SIZE; 
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
  DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte; 
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; 
  DMA_InitStructure.DMA_Priority = DMA_Priority_High; 
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA2_Channel3, &DMA_InitStructure);   
  DMA_Cmd(DMA2_Channel3, ENABLE); 	
	USART_DMACmd(UART4, USART_DMAReq_Rx, ENABLE);

	DMA_ITConfig(DMA2_Channel3, DMA_IT_TC | DMA_IT_HT , ENABLE); 	
}

