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

#define ROLLER_HOST "www.hwytree.com"
#define ROLLER_ADDR1 "http://www.hwytree.com/roller1.asp"
#define ROLLER_ADDR2 "http://www.hwytree.com/roller2.asp"
#define ROLLER_PHONE_NUM "18802150339"
#define ROLLER_HTTP_PROTOCOL "HTTP/1.1"
#define ROLLER_USER_AGENT "User-Agent: Fiddler"

static uint16_t gsm_rcv_data_cnt = 0;
static uint8_t gsm_rcv_datas[GSM_MAX_DATA_SIZE];
static uint8_t gsm_snd_datas[GSM_MAX_DATA_SIZE];

static uint8_t cookie[500];
static uint8_t longitude[16];
static uint8_t latitude[16];

static uint8_t WaitFor(uint8_t *p,uint32_t timeout);
static void DataReset(void);
static uint8_t ParserCSQ(void);
static uint8_t ParserCookie(uint32_t timeout);
static uint8_t ParserLocation(void);

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
  DataReset();
}

/* retrieve data from UART4 DMA buffer */
void GSM_RetrieveData(void * src, size_t len)
{
  if(gsm_rcv_data_cnt + len < GSM_MAX_DATA_SIZE)
  {
    memcpy(&gsm_rcv_datas[gsm_rcv_data_cnt], src, len);
    gsm_rcv_data_cnt += len;
  }
}

uint8_t GSM_Wakeup(void)
{
	uint8_t err = 1;
	uint16_t i = 0;

  while(i < 10)
  {
    GSM_SendAT("AT+CSCLK=0");
    err = WaitFor("OK", 2);
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
    /* 
      Note: 
      A HEX string such as "00 49 49 49 49 FF FF FF FF" will be sent out through serial port at the baud rate of 115200 immediately after SIM800 Series is powered on. 
      The string shall be ignored since it is used for synchronization with PC tool. 
      Only enter AT Command through serial port after SIM800 Series is powered on and Unsolicited Result Code "RDY" is received from serial port. 
      If auto-bauding is enabled, the Unsolicited Result Codes "RDY" and so on are not indicated when you start up the ME, and the "AT" prefix, or "at" prefix must be set at the beginning of each command line.
      If we are using auto-bauding, it is recommended to wait 3 to 5 seconds before sending the first AT character. 
      Otherwise undefined characters might be returned. 

      Right now, we are using fixed baud rate. (1. AT+IPR=115200 2. AT&W 3. reset sim800C)
      If the SIM card's inserted, we'll receive the stings as the following:
      
      RDY
      
      +CFUN: 1
      
      +CPIN: READY
      
      Call Ready
      
      SMS Ready

      So we can start our operation by wait for "SMS Ready"
      */
    //SysTick_Delay_ms(5000);
    WaitFor("SMS Ready", 20);
   
    
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

  /* No echo */
  GSM_SendAT("ATE0");
  err = WaitFor("OK",2);  
  if(err > 0)
    return err;		

  /* Displays signal strength and channel bit error rate - +CSQ: 20,0*/
  GSM_SendAT("AT+CSQ"); 
  err = WaitFor("+CSQ:", 10);
  if(err > 0)
    return err; 

  err = ParserCSQ();
  if(0 == err)
  {
    /* Network Registration Status  - CGREG: 0,3 OK*/
    GSM_SendAT("AT+CGREG?");
    err = WaitFor("+CGREG:", 2);
    if(err > 0)
      return err;

    /* Check if the MS is connected to the GPRS network - +CGATT: 0 OK*/
    GSM_SendAT("AT+CGATT?");
    err = WaitFor("+CGATT:", 2);
    if(err > 0)
      return err;	

    SysTick_Delay_ms(100);

    /* Deactivate GPRS PDP Context - AT+CIPSHUT SHUT OK*/
    GSM_SendAT("AT+CIPSHUT");    
    err = WaitFor("SHUT OK", 15);
    if(err > 0)
      return err;	

    SysTick_Delay_ms(100);

    /* Start Task and Set APN, USER NAME, PASSWORD */
    GSM_SendAT("AT+CSTT");    
    err=WaitFor("OK", 5);
    if(err>0)
      return err;	

    /* Bring Up Wireless Connection with GPRS or CSD */
    GSM_SendAT("AT+CIICR");
    err=WaitFor("OK", 10);
    if(err>0)
      return err;	
    SysTick_Delay_ms(500);
  
    /* Get Local IP Address - 10.222.243.153*/
    GSM_SendAT("AT+CIFSR");
    err=WaitFor(".", 2);
    if(err>0)
      return err;	 

  }

  return err;    
}


uint8_t GSM_Location(void)
{
  uint8_t err = 0; 	

  /* No echo */
  GSM_SendAT("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  err = WaitFor("OK",2);  
  if(err > 0)
    return err;		

  GSM_SendAT("AT+SAPBR=3,1,\"APN\",\"CMNET\"");
  err = WaitFor("OK",2);  
  if(err > 0)
    goto 	LOCATION_EXIT;

  GSM_SendAT("AT+SAPBR=1,1");
  err = WaitFor("OK",2);  
  if(err > 0)
    goto 	LOCATION_EXIT;
  
  GSM_SendAT("AT+SAPBR=2,1");
  err = WaitFor("OK",2);  
  if(err > 0)
    goto 	LOCATION_EXIT; 

  GSM_SendAT("AT+CIPGSMLOC=1,1");
  err = WaitFor("OK",20);  
  if(err > 0)
    goto 	LOCATION_EXIT; 

  err = ParserLocation();
  if(err > 0)
  {
  }

LOCATION_EXIT:

  GSM_SendAT("AT+SAPBR=0,1");
  err = WaitFor("OK",2);  
  if(err > 0)
    return err; 

	return err;
}

uint8_t GSM_GPRSSendData(void)
{
  uint8_t err = 0; 	

  /* Start up TCP or UDP connection */
  GSM_SendAT("AT+CIPSTART=\"TCP\",\"www.hwytree.com\",80"); 
  err = WaitFor("CONNECT OK",20);
  if(err > 0)
    return err;	

  /* Send Data Through TCP or UDP Connection */
  GSM_SendAT("AT+CIPSEND"); 
  err = WaitFor(">",2);
  if(err > 0)
    return err; 

  sprintf((char*)gsm_snd_datas, 
    "GET %s?session=0&pn=%s %s\r\n%s\r\nHost: %s\r\n\r\n\0", 
    ROLLER_ADDR1,
    ROLLER_PHONE_NUM,
    ROLLER_HTTP_PROTOCOL,
    ROLLER_USER_AGENT,
    ROLLER_HOST
    );
  GSM_SendATData(gsm_snd_datas);
  err = WaitFor("SEND OK",2);
  if(err > 0)
    return err;   
  
  err = ParserCookie(10);
  if(0 == err)
  {
    /* Send Data Through TCP or UDP Connection */
    GSM_SendAT("AT+CIPSEND"); 
    err = WaitFor(">",2);
    if(err > 0)
      return err;  
    
    sprintf((char*)gsm_snd_datas, 
      "GET %s?session=1&pn=%s %s\r\n%s\r\nHost: %s\r\nCookie: %s\r\n\r\n\0", 
      ROLLER_ADDR1,
      ROLLER_PHONE_NUM,
      ROLLER_HTTP_PROTOCOL,
      ROLLER_USER_AGENT,
      ROLLER_HOST,    
      (char*)cookie
      );
    GSM_SendATData(gsm_snd_datas);    
    err = WaitFor("SEND OK",2);
    if(err > 0)
      return err;   

    
    err = WaitFor("CXALL",60*60);
    if(err > 0)
      return err; 

    /* Send Data Through TCP or UDP Connection */
    GSM_SendAT("AT+CIPSEND"); 
    err = WaitFor(">",2);
    if(err > 0)
      return err; 

#define ROLLER_ACCEPT "Accept: text/html,application/json"
#define ROLLER_ACCEPT_LA "Accept-Language: en-US"
#define ROLLER_CTT_TYPE "Content-Type: application/json"
#define ROLLER_CONNECTION "Connection: keep-alive"
#define ROLLER_USER_AGENT2 "User-Agent: JackBian"
#define ROLLER_SEND_DATA "{\"DATA\":[{\"pn\":\"18802150339\"},{\"alarmdata\":\"0\"},{\"commanddata\":\"10\",\"OBDZS\":\"80\", \"OBDCS\":\"60\", \"OBDSW\":\"20\", \"OBDKQLL\":\"30\", \"OBDQGJDYL\":\"20\", \"OBDJQMKD\":\"10\", \"OBDYNDCGQZ\":\"15\", \"OBDFHBFB\":\"30\", \"CNWD\":\"25\", \"CNYNN\":\"20\"},{\"bookdata\":\"0\"}]}\r\n" 

    sprintf((char*)gsm_snd_datas, 
      "POST %s %s\r\n%s\r\n%s\r\nContent-Length: %d\r\n%s\r\n%s\r\nHost: %s:80\r\n%s\r\n\r\n%s",
      ROLLER_ADDR2,
      ROLLER_HTTP_PROTOCOL,
      ROLLER_ACCEPT,
      ROLLER_ACCEPT_LA,
      strlen(ROLLER_SEND_DATA),
      ROLLER_CTT_TYPE,
      ROLLER_CONNECTION,
      ROLLER_HOST,  
      ROLLER_USER_AGENT2,
      ROLLER_SEND_DATA
      );

    GSM_SendATData(gsm_snd_datas);
    err = WaitFor("SEND OK",2);
    if(err > 0)
      return err;     
  }
  else
  {
    ERROR("No Cookie \r\n");
  }

  return err;

}

/* timeout seconds */
uint8_t WaitFor(uint8_t *p, uint32_t timeout)
{
  char *p_find = NULL;
  uint32_t cnt = 0;
  uint8_t err = 1;

  SysTick_Delay_ms(100);
  timeout *= 10;
  while(cnt < timeout)
  {
    if(gsm_rcv_data_cnt > 0)
    {
      p_find = strstr((char*)gsm_rcv_datas, (char*)p);
      if(NULL != p_find)
      {
        err = 0;
        break;
      }
    }
    SysTick_Delay_ms(100);
    cnt++;
  }

  return err;
} 

static void DataReset(void)
{
  /* reset status */
  gsm_rcv_data_cnt=0;
  memset(gsm_rcv_datas, 0x00, GSM_MAX_DATA_SIZE);
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
  
  p = strchr((char *)gsm_rcv_datas, ':'); 
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

  p = strchr((char *)gsm_rcv_datas, ','); 
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

static uint8_t ParserCookie(uint32_t timeout)
{
  char *p_find = NULL;
  uint32_t cnt = 0;
  uint8_t err = 1;
	uint8_t i = 0;

  SysTick_Delay_ms(100);
  timeout *= 10;
  while(cnt < timeout)
  {
    //DEBUG("gsm_rcv_datas=%s\r\n", gsm_rcv_datas);
    if(gsm_rcv_data_cnt > 0)
    {
      p_find = strstr((char *)gsm_rcv_datas, "Set-Cookie:"); 
      if(NULL != p_find)
      {
        err = 0;
        while(*p_find != ':') p_find++;
        p_find++;
        for(i = 0; i < 50; i++)
        {
          if(*p_find == ';')
            break;
          else
            cookie[i] = *p_find;

          p_find++;
        }  

        //DEBUG("cookie=%s\r\n", cookie);
        break;
      }
    }
    SysTick_Delay_ms(100);
    cnt++;
  }


  return err;
}

static uint8_t ParserLocation(void)
{
  char *p_find = NULL;
  uint8_t err = 1;
	uint8_t i;

  p_find = strchr((char *)gsm_rcv_datas, ',');
  for(i = 0; i < 16; i++)
  {
    p_find++;
    if(*p_find == ',')
      break;
    else
      longitude[i] = *p_find;
  }  

  for(i = 0; i < 16; i++)
  {
    p_find++;
    if(*p_find == ',')
      break;
    else
      latitude[i] = *p_find;
  } 

  if(longitude[0] != 0 && latitude[0] != 0)
    err = 0;
  
  return err;

}


