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
#include "stm32f10x.h"
#include "usart1.h"
#include "can.h"
#include "systick.h"
#include "tim2.h"
#include "debug.h"
#include "sys_init.h"
#include "uart4.h"
#include "gsm.h"
#include "gps.h"

static void INIT_All(void);


void Check_PlayMode_Cmd(char * cmd)
{
    
    if(cmd[0]==':'){
        printf("\r\n:");
        if (cmd[1]=='>' ) {
            if(cmd[2] >= '0' && cmd[2] < '9'){
                DEBUG_AddPlayMode( (uint32_t)0x00000001 << (cmd[2] - '0'));
                printf("Add PlayMode %c, Current Mode : %d\r\n", cmd[2], DEBUG_GetPlayMode());
            }
        }
        else if (cmd[1]=='<' ) {
            if(cmd[2] >= '0' && cmd[2] < '9'){
                DEBUG_RemovePlayMode( (uint32_t)0x00000001 << (cmd[2] - '0'));
                printf("Remove PlayMode %c, Current Mode : %d\r\n", cmd[2], DEBUG_GetPlayMode());
            }
        }
        else if(cmd[1]=='=' && cmd[2] == '=')
             printf("Current Mode : %d\r\n", DEBUG_GetPlayMode());
    }      
}

void USART1_RX_MSG_Proc(void)
{
    uint32_t res_len = 0;
    res_len = DMA_GetCurrDataCounter(DMA1_Channel5);
    
    
    if(UART1_DMA_RCVBUFFER_SIZE - res_len){

        if(DEBUG_GetPlayMode() & SYS_MODE_ECHO){
            USART1_TX_DMA_Send(uart1_dma_receivebuffer, (size_t)(UART1_DMA_RCVBUFFER_SIZE - res_len));
        }
        if(DEBUG_GetPlayMode() & SYS_MODE_1TH4){
            UART4_TX_DMA_Send(uart1_dma_receivebuffer, (size_t)(UART1_DMA_RCVBUFFER_SIZE - res_len));
        }

        //play mode control
        Check_PlayMode_Cmd((char *)uart1_dma_receivebuffer);
     
        DMA_Cmd(DMA1_Channel5, DISABLE); 
        DMA_SetCurrDataCounter(DMA1_Channel5, UART1_DMA_RCVBUFFER_SIZE);
        DMA_Cmd(DMA1_Channel5, ENABLE);  
    }
}


void UART4_RX_MSG_Proc(void){
    uint16_t res_len = 0;
    res_len = DMA_GetCurrDataCounter(DMA2_Channel3);

    if(UART4_DMA_RCVBUFFER_SIZE - res_len) {
        GSM_RetrieveData(uart4_dma_receivebuffer, (size_t)(UART4_DMA_RCVBUFFER_SIZE - res_len));
        if(DEBUG_GetPlayMode() & SYS_MODE_1TH4){
            USART1_TX_DMA_Send(uart4_dma_receivebuffer, (size_t)(UART4_DMA_RCVBUFFER_SIZE - res_len));
        }    
        DMA_Cmd(DMA2_Channel3, DISABLE); 
        DMA_SetCurrDataCounter(DMA2_Channel3, UART4_DMA_RCVBUFFER_SIZE);
        DMA_Cmd(DMA2_Channel3, ENABLE);       
    }
}


void MSG_Polling(void)
{
    // TODO: 
    //       1,Need a Message Pool or Message Queue or FIFO for  the efficiency and robust of sending and receiving buffers  
    //       2,Error and warning Monitor for receiving a lot message overflow the receive buffer. (Using TC HT interrupt handler)   
    //       3,message retreat middle layer  like "playmode controller".    
    USART1_RX_MSG_Proc();
    UART4_RX_MSG_Proc();
}

int main(void)
{	
	uint8_t err = 1;

	INIT_All();

        
    err = GSM_PowerOn();
    if(0 == err)
        INFO("\r\n GSM Power ON!");
    else
        ERROR("\r\n GSM Power ON failed!");
        
   if(DEBUG_GetPlayMode() & SYS_MODE_AUTO_CONN){
        err = GSM_GPRSConnect();
        if(0 == err)
            INFO("\r\n GPRS connection OK!");
        else
            ERROR("\r\n GPRS connection failed!");
        
        GSM_Location();
        GSM_GPRSSendData();
    }
	for(;;)
	{   
		/* LED twinkles */
        #define TWINKLE_INTERVAL 2000
		if(TIM2_Ms_Cycle(TWINKLE_INTERVAL)){
			DBG_LED1_ON();
   
		}
        if(TIM2_Ms_Half(TWINKLE_INTERVAL)){
            DBG_LED1_OFF();
			DEBUG_MonitorState();    
        }
        

        MSG_Polling();
        
	}
  
}

static void INIT_All(void) 
{
    RCC_Configuration();
    NVIC_Configuration();
    GPIO_Configuration(); 

    USART1_Config();
    USART1_DMA_Config();
    UART4_Config();
    UART4_DMA_Config(); 

    TIM2_Init();

    DBG_LED1_OFF();
    DBG_LED2_OFF();
        
    INFO("\r\n");
    INFO("***********************************************\r\n"); 
    INFO("*                                             *\r\n"); 
    INFO("*  HWYTREE - Innovation meets quality! Y^_^Y  *\r\n"); 
    INFO("*                                             *\r\n"); 
    INFO("***********************************************\r\n"); 
}


