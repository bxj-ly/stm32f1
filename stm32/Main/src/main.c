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

int main(void)
{	
	uint8_t err = 1;

	INIT_All();

	err = GSM_PowerOn();
	if(0 == err)
		INFO("\r\n GSM Power ON!");
	else
		ERROR("\r\n GSM Power ON failed!");

	err = GSM_GPRSConnect();
	if(0 == err)
		INFO("\r\n GPRS connection OK!");
	else
		ERROR("\r\n GPRS connection failed!");
	
	GSM_Location();
	GSM_GPRSSendData();

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
        
        // TODO: message retreat middle layer 
        //       polling buffers
        //       Error and warning Monitor to USART1 
        
	}
  
}

static void INIT_All(void) 
{
    RCC_Configuration();
    NVIC_Configuration();
    GPIO_Configuration(); 

    USART1_Config();
    UART4_Config();

    TIM2_Init();

    DBG_LED1_OFF();
    DBG_LED2_OFF();
    USART1_DMA_Config();
    UART4_DMA_Config();  
    INFO("\r\n");
    INFO("***********************************************\r\n"); 
    INFO("*                                             *\r\n"); 
    INFO("*  HWYTREE - Innovation meets quality! Y^_^Y  *\r\n"); 
    INFO("*                                             *\r\n"); 
    INFO("***********************************************\r\n"); 
}


