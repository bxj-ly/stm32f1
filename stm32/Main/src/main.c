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
#include "lmp91000.h"
#include "beeper.h"
#include "spi.h"
#include "vcom.h"
#include "nmealib.h"

static void INIT_All(void);
static void SIM800C_PowerOn(void);
static void UART_MSG_Polling(void);
static void Event_Polling(void);

int main(void)
{
    INIT_All();
    SIM800C_PowerOn();

    for(;;)
    {   
        /* LED twinkles */
        #define TWINKLE_INTERVAL 2000
        if(TIM2_Ms_Cycle(TWINKLE_INTERVAL)){
            //DBG_LED1_ON();
        }
        if(TIM2_Ms_Half(TWINKLE_INTERVAL)){
            //DBG_LED1_OFF();
            //DEBUG_MonitorState();    
        }
        
        UART_MSG_Polling();
        Event_Polling();
    }
  
}

static void INIT_All(void) 
{
    RCC_Configuration();
    GPIO_Configuration(); 

#if defined(USART1_AS_DEBUG_COM)
    USART1_Config();
    USART1_DMA_Config();
#endif


    UART4_Config();
    UART4_DMA_Config(); 

    TIM2_Init();
 #if defined(VIRTUALCOM_AS_DEBUG_COM)
    VCOM_Config();
#endif   
    //BEEPER_GPIOConfiguration();

    DBG_LED1_OFF();
    DBG_LED2_OFF();
    INFO("\r\n");
    INFO("***********************************************\r\n"); 
    INFO("*                                             *\r\n"); 
    INFO("*  HWYTREE - Innovation meets quality! Y^_^Y  *\r\n"); 
    INFO("*                                             *\r\n"); 
    INFO("***********************************************\r\n"); 

    CAN_ProtocolScan();
    I2C_LMP91000_Init();
    SPI1_Init();

}

static void SIM800C_PowerOn(void)
{
    uint8_t err = 1;

    if(DEBUG_GetPlayMode() & SYS_MODE_AUTO_CONN){
        err = GSM_PowerOn();
        if(0 == err) {
            INFO("\r\n GSM Power ON!");
        }
        else {
            ERROR("\r\n GSM Power ON failed!");
            SYS_Reset();
        }
        /* No echo */
        GSM_SendAT("ATE0");
        err = GSM_WaitForMsg("OK",2);  
        if(err > 0)
            SYS_Reset();
    }

}
static void UART_MSG_Polling(void)
{
    // TODO: 
    //       1,Need a Message Pool or Message Queue or FIFO for  the efficiency and robust of sending and receiving buffers  
    //       2,Error and warning Monitor for receiving a lot message overflow the receive buffer. (Using TC HT interrupt handler)   
    //       3,message retreat middle layer  like "playmode controller".    
    USART1_RX_MSG_Proc();
    UART4_RX_MSG_Proc();
}

static void Event_Polling(void)
{
    uint8_t err = 1;
    uint32_t cnt = 0;

    DBG_LED1_ON();

    cnt = TIM2_GetTick();
    if(DEBUG_GetPlayMode() & SYS_MODE_AUTO_CONN){
         if(cnt % 1000 == 0) {
            CAN_CheckAllStatus(); 
            GPS_Position();
            
            err = GSM_CheckSignalStrength();
            if(0 != err) {
                ERROR("\r\n GSM Signal failed!");
                SYS_Reset();
            } 
            err = GSM_GPRSSendData();
            if(0 != err) {
                /* Deactivate GPRS PDP Context - AT+CIPSHUT SHUT OK*/
                GSM_SendAT("AT+CIPSHUT");    
                err = GSM_WaitForMsg("SHUT OK", 15);
                if(err > 0)
                    SYS_Reset();
            }
        }
    }
    DBG_LED1_OFF();
    //SysTick_Delay_ms(200);

}

