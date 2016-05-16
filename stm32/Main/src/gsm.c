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
#include "gsm.h"
#include "debug.h"
#include "systick.h"
#include "uart4.h"

/* PB5 -> GSM_PWRKEY */
#define SIM800C_PWRKEY_ON()    GPIOB->BRR  = 0x00000020
#define SIM800C_PWRKEY_OFF()   GPIOB->BSRR = 0x00000020
/* PC1 -> GSM_PSTATUS */
#define SIM800C_PWR_STATUS()   (GPIOC->IDR & 0x00000002)

#define GSM_MAX_DATA_SIZE 1024


static uint16_t gsm_res_cnt = 0;
static uint8_t gsm_datas[GSM_MAX_DATA_SIZE];
static uint8_t ParserCSQ(void);
static void PrintData(void);
static void DataReset(void);


/* retrieve data from UART4 DMA buffer */
void GSM_RetrieveData(void * src, size_t len)
{
  if(gsm_res_cnt + len < GSM_MAX_DATA_SIZE)
  {
    memcpy(&gsm_datas[gsm_res_cnt], src, len);
    gsm_res_cnt += len;
  }
}

/* send AT command */
void GSM_SendAT(uint8_t *data)
{ 
  uint16_t i = 0;
  uint16_t len = strlen((char*)data);
  DEBUG(">> %s\r\n", data);

  DataReset();
  for (i=0; i<len; i++)
  {
    UART4_SendByte(data[i]);
  }
  UART4_SendByte(0x0D);  
  UART4_SendByte(0x0A);        
}

/* send AT data */
void GSM_SendATData(uint8_t *data)
{ 
  uint16_t i = 0;
  uint16_t len = strlen((char*)data);
  DEBUG(">> %s\r\n", data);

  for (i=0; i<len; i++)
  {
    UART4_SendByte(data[i]);
  }
  UART4_SendByte(0x1A);  
}

uint8_t GSM_Wakeup(void)
{
	uint8_t err = 1;
	uint16_t i = 0;

  while(i < 10)
  {
    GSM_SendAT("AT+CSCLK=0");
    err = GSM_WaitString("OK", 2);
    if(err == 0)
    {
      return 0;
    }		
    i++;
  }
  
  return 1;
}

uint8_t GSM_PowerOn(void)
{
	uint16_t i = 0;

  DBG_LED2_OFF();
  
  /* SIM800C Power off ? */
  if(SIM800C_PWR_STATUS() == 0)
  {
    /* Press PWRKEY until STATUS is ON */   
    SIM800C_PWRKEY_ON();
    SysTick_Delay_ms(1500);
    SIM800C_PWRKEY_OFF();
    SysTick_Delay_ms(1800);
    while(SIM800C_PWR_STATUS() == 0)
    {
      SysTick_Delay_ms(200);
      i++;
      if(i > 10)
      {
        ERROR("\r\n GSM Power Failed !!! \r\n");
        return 1;
      }
    }

    INFO("\r\n PSTATUS is OK now ! \r\n");
  }
  else
  {
#if 0  
    INFO("\r\n Wait up SIM800C ! \r\n");
    i = 0;
    while(i < 5)  
    {
        err = GSM_Wakeup();
        if(err == 0)
        {
            break;
        }
        i++;
    } 
#endif    
  }

  DBG_LED2_ON();

  return 0;

}


uint8_t GSM_GPRSConnect(void)
{
  uint8_t err = 0; 	
  uint8_t i = 0;

  while(i < 50)
  {
    GSM_SendAT("AT");
    err = GSM_WaitString("OK", 2);
    if(err == 0)
    {
      break;
    }		
    i++;
  }

  /* No echo */
  GSM_SendAT("ATE1");
  err = GSM_WaitString("OK",2);  
  if(err > 0)
    return err;		

  /* Displays signal strength and channel bit error rate */
  GSM_SendAT("AT+CSQ"); 
  err = GSM_WaitString("+CSQ:", 10);    //AT+CGREG? +CGREG: 0,3 OK
  if(err > 0)
    return err; 

  err = ParserCSQ();
  if(0 == err)
  {
    /* Network Registration Status */
    GSM_SendAT("AT+CGREG?");
    err = GSM_WaitString("+CGREG:", 2);    //AT+CGREG? +CGREG: 0,3 OK
    if(err > 0)
      return err;

    /* Check if the MS is connected to the GPRS network */
    GSM_SendAT("AT+CGATT?");
    err = GSM_WaitString("+CGATT:", 2);    //AT+CGATT? +CGATT: 0 OK
    if(err > 0)
      return err;	

    SysTick_Delay_ms(100);

    /* Deactivate GPRS PDP Context */
    GSM_SendAT("AT+CIPSHUT");    
    err = GSM_WaitString("SHUT OK", 15);    //AT+CIPSHUT SHUT OK
    if(err > 0)
      return err;	

    SysTick_Delay_ms(100);

    /* Start Task and Set APN, USER NAME, PASSWORD */
    GSM_SendAT("AT+CSTT");    
    err=GSM_WaitString("OK", 5);    //AT+CSTT OK
    if(err>0)
      return err;	

    /* Bring Up Wireless Connection with GPRS or CSD */
    GSM_SendAT("AT+CIICR");
    err=GSM_WaitString("OK", 10);    //AT+CIICR OK
    if(err>0)
      return err;	
    SysTick_Delay_ms(500);
  
    /* Get Local IP Address */
    GSM_SendAT("AT+CIFSR");
    err=GSM_WaitString("+CIFSR", 2);    //AT+CIFSR 10.222.243.153
    if(err>0)
    return err;	 
#if 0    
    /* Configure Module as Server */
    GSM_SendAT("AT+CIPSERVER=1,6001");
    err=GSM_WaitString("OK", 10);    //AT+CIICR OK
    if(err>0)
      return err;	
    SysTick_Delay_ms(500);
#endif

    /* Start up TCP or UDP connection */
    GSM_SendAT("AT+CIPSTART=\"TCP\",\"www.coolbug.cn\",6001"); 
    err=GSM_WaitString("CONNECT OK",10);
    if(err>0)
    return err;	

  }

  return err;    
}

/* timeout seconds */
uint8_t GSM_WaitString(uint8_t *p, uint8_t timeout)
{
  char *p_find = NULL;
  uint8_t cnt = 0;
  uint8_t err = 1;

  SysTick_Delay_ms(100);
  timeout *= 10;
  while(cnt < timeout)
  {
    if(gsm_res_cnt > 0)
    {
      p_find = strstr((char*)gsm_datas, (char*)p);
      if(NULL != p_find)
      {
        err = 0;
        break;
      }
    }
    SysTick_Delay_ms(100);
    cnt++;
  }

  //PrintData();
  return err;
} 


static void PrintData(void)
{
  uint16_t i = 0;
  /* output response */
  DEBUG("\r\n------------------\r\n ");
  for(i=0;i<gsm_res_cnt;i++)
    DEBUG("%c",gsm_datas[i]);
  DEBUG("\r\n------------------\r\n ");
}

static void DataReset(void)
{
  /* reset status */
  gsm_res_cnt=0;
  memset(gsm_datas, 0x00, GSM_MAX_DATA_SIZE);
}

/* Signal Quality Report 
Response
+CSQ: <rssi>,<ber>
OK  
Parameters
<rssi>
0 -115 dBm or less
1 -111 dBm
2...30 -110... -54 dBm
31 -52 dBm or greater
99 not known or not detectable
<ber> (in percent):
0...7 As RXQUAL values in the table in GSM 05.08 [20] subclause 7.2.4
99 Not known or not detectable  

rssi 2-31 is OK
ber 0-7 is OK
 AT+CSQ
+CSQ: 20,0

OK
*/
static uint8_t ParserCSQ(void)
{
  uint8_t i = 0;
  uint8_t rssi = 0;
  uint8_t ber = 0;
  uint8_t err = 0;
  char *p = NULL;  
  
  p = strchr((char *)gsm_datas, ':'); 
  for(i = 0; i < 5; i++)
  {
    p++;
    if(*p > 0x2F && *p < 0x3A)
    {
      rssi *= 10;
      rssi += *p - 0x30;
    }
    else if(*p == ',')
      break;
  }

  p = strchr((char *)gsm_datas, ','); 
  for(i = 0; i < 10; i++)
  {
    p++;
    if(*p > 0x2F && *p < 0x3A)
    {
      ber *= 10;
      ber += *p - 0x30;
    }
    else if(*p == 0x0D || *p == 0x0A)
      break; 
  }
  INFO("\r\n rssi=%d ber=%d\r\n ", rssi, ber);

  if(rssi > 1 && rssi < 32)
  {
    if(ber > 7)
    {
      err=1;  
    }
  }
  else
  {
    err=2;
  }

  return err;
}


