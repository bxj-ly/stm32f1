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

static void INIT_All(void);
static void MSG_Polling(void);

int main(void)
{
    uint8_t err = 1;
    uint32_t cnt = 0;

    INIT_All();
    if(DEBUG_GetPlayMode() & SYS_MODE_AUTO_CONN){
        err = GSM_PowerOn();
        if(0 == err) {
            INFO("\r\n GSM Power ON!");
        }
        else {
            ERROR("\r\n GSM Power ON failed!");
            return -1;
        }

        err = GSM_GPRSConnect();
        if(0 == err) {
            INFO("\r\n GPRS connection OK!");
        }
        else {
            ERROR("\r\n GPRS connection failed!");
            return -1;
        }  

        err = GSM_GPRSBuildTCPLink();
        if(0 == err) {
            INFO("\r\n TCP connection OK!");
        }
        else {
            ERROR("\r\n TCP connection failed!");
            return -1;
        } 

    }

    for(;;)
    {   
    /* LED twinkles */
#define TWINKLE_INTERVAL 2000
        if(TIM2_Ms_Cycle(TWINKLE_INTERVAL)){
            DBG_LED1_ON();
            cnt++;
            if(DEBUG_GetPlayMode() & SYS_MODE_AUTO_CONN){
                CAN_CheckAllStatus();  
                if(cnt >= 1) {
                    cnt = 0;
                    //GSM_Location();       
                    err = GSM_MsgQuickCheck("CXALL");
                    if(err == 0) {
                        GSM_GPRSPushCarStatus();
                    }

                    err = GSM_MsgQuickCheck("\"REPLY\":\"OK\"");
                    if(err == 0) {
                        err = GSM_ConnectionHeartBeat();
                        if(err > 0) {
                            ERROR("\r\n HB failed !");
                        }
                    }
                    
                    err = GSM_MsgQuickCheck("\"RESESSION\":\"YES\"");
                    if(err == 0) {
                        GSM_ConnectionHeartBeat();
                    } 

                    err = GSM_MsgQuickCheck("\"INSTRUCTION\":\"CXBPS\"");
                    if(err == 0) {
                        //GSM_GPRSBeeperStatus();
                        GSM_ConnectionHeartBeat();
                    }                
                            
                    err = GSM_MsgQuickCheck("\"INSTRUCTION\":\"BPOPEN\"}");
                    if(err == 0) {
                        GSM_DataReset();
                        BEEPER_ON();
                        GSM_ConnectionHeartBeat();
                        INFO("\r\n Beeper Open OK!");
                    }        
                    
                    err = GSM_MsgQuickCheck("\"INSTRUCTION\":\"BPCLOSE\"}");
                    if(err == 0) {
                        GSM_DataReset();
                        BEEPER_OFF();
                        GSM_ConnectionHeartBeat();
                        INFO("\r\n Beeper Close OK!");
                    }         
                    
                }
            }
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

    CAN_ProtocolScan();
    I2C_LMP91000_Init();
    BEEPER_GPIOConfiguration();

}

static void MSG_Polling(void)
{
    // TODO: 
    //       1,Need a Message Pool or Message Queue or FIFO for  the efficiency and robust of sending and receiving buffers  
    //       2,Error and warning Monitor for receiving a lot message overflow the receive buffer. (Using TC HT interrupt handler)   
    //       3,message retreat middle layer  like "playmode controller".    
    USART1_RX_MSG_Proc();
    UART4_RX_MSG_Proc();
}


