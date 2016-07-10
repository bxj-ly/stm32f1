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
#include "kline.h"
#include "gps.h"
#include "ISO15765_4.h"
#include "spi.h"
#include "sys_init.h"
#include "obd.h"

/* PB5 -> GSM_PWRKEY */
#define SIM800C_PWRKEY_ON()    GPIOB->BRR  = 0x00000020
#define SIM800C_PWRKEY_OFF()   GPIOB->BSRR = 0x00000020
/* PC1 -> GSM_PSTATUS */
#define SIM800C_PWR_STATUS()   (GPIOC->IDR & 0x00000002)

#define GSM_MAX_DATA_SIZE 1024

#define ROLLER_HOST "www.hwytree.com"
#define ROLLER_ADDR1 "http://www.hwytree.com/roller1.asp"
#define ROLLER_ADDR2 "http://www.hwytree.com/roller2.asp"
#define ROLLER_PHONE_NUM "13012345678"
#define ROLLER_HTTP_PROTOCOL "HTTP/1.1"
#define ROLLER_USER_AGENT "User-Agent: Roller"

#define ROLLER_ACCEPT "Accept: text/html,application/json"
#define ROLLER_ACCEPT_LA "Accept-Language: en-US"
#define ROLLER_CTT_TYPE "Content-Type: application/json"
#define ROLLER_CONNECTION "Connection: keep-alive"

uint16_t battery_voltage;
uint16_t mcc = 0;
uint16_t mnc = 0;
uint16_t cellid = 0;
uint16_t lac = 0;

#define BT_DEV_NAME "SPP-CA"
uint16_t bt_fan_device_id;
uint16_t bt_fan_profile_id;

static uint16_t gsm_rcv_data_cnt = 0;
static uint8_t gsm_rcv_datas[GSM_MAX_DATA_SIZE];
static uint8_t gsm_snd_datas[GSM_MAX_DATA_SIZE];
static uint8_t gsm_json_datas[GSM_MAX_DATA_SIZE];

static void set_fixed_baudrate(void);
static uint8_t ParserCSQ(void);
static uint16_t ParserCADC(void);
static void ParserCENG(void);
static uint8_t ParserBTFanDeviceID(void);
static uint8_t ParserBTFanProfileID(void);
static uint8_t ParserBTStatus(void);


/* send AT command */
void GSM_SendAT(uint8_t *data)
{ 
    uint16_t i = 0;
    uint16_t len = strlen((char*)data);
    DEBUG(">> %s\r\n", data);

    GSM_DataReset();
    for (i=0; i<len; i++) {
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

    for (i=0; i<len; i++)  {
        UART4_SendByte(data[i]);
    }
    UART4_SendByte(0x1A);  
    GSM_DataReset();
}

void GSM_DataReset(void)
{
    /* reset status */
    gsm_rcv_data_cnt=0;
    memset(gsm_rcv_datas, 0x00, GSM_MAX_DATA_SIZE);
}

/* retrieve data from UART4 DMA buffer */
void GSM_RetrieveData(void * src, size_t len)
{
    if(gsm_rcv_data_cnt + len < GSM_MAX_DATA_SIZE) {
        memcpy(&gsm_rcv_datas[gsm_rcv_data_cnt], src, len);
        gsm_rcv_data_cnt += len;
    }
}

uint8_t GSM_Wakeup(void)
{
    uint8_t err = 1;
    uint16_t i = 0;

    while(i < 10) {
        GSM_SendAT("AT+CSCLK=0");
        err = GSM_WaitForMsg("OK", 2);
        if(err == 0) {
            return 0;
        }
        i++;
    }

    return 1;
}

uint8_t GSM_PowerOn(void)
{
    int16_t i = 0;
    uint8_t err = 0;

    DBG_LED2_OFF();
  
    /* SIM800C Power off ? */
    if(SIM800C_PWR_STATUS() == 0) {
        /* Press PWRKEY until STATUS is ON */   
        SIM800C_PWRKEY_ON();
        SysTick_Delay_ms(1500);
        SIM800C_PWRKEY_OFF();
        SysTick_Delay_ms(1800);
        while(SIM800C_PWR_STATUS() == 0) {
            SysTick_Delay_ms(200);
            i++;
            if(i > 10) {
                ROLLER_ERROR("\r\n GSM Power Failed !!! \r\n");
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
        err = GSM_WaitForMsg("SMS Ready", 60);
        if(err > 0)
        {
          set_fixed_baudrate();
          return err; 
        }

    }
    else  {
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

    return 0;

}

uint8_t GSM_CheckSignalStrength(void)
{
    uint8_t err = 0;
    uint8_t csq_retry_cnt = 3;

/* If the GSM signal is not so good, we can't get signal strength at once.
 * So I add retry here. retry count is adjustable.
 */

AT_CSQ_RETRY:

    /* Displays signal strength and channel bit error rate - +CSQ: 20,0*/
    GSM_SendAT("AT+CSQ"); 
    err = GSM_WaitForMsg("+CSQ:", 10);
    if(err > 0)
        return err; 

    err = ParserCSQ();
    if(0 == err){
        return 0;
    }
    else if(csq_retry_cnt--) goto AT_CSQ_RETRY;

    return err;    

}


uint8_t GSM_GPRSSendData(void)
{
    uint8_t err = 0;
    uint8_t i = 0;
    ErrorStatus errStatus;
    
    /* Network Registration Status  - CGREG: 0,3 OK*/
    GSM_SendAT("AT+CGREG?");
    err = GSM_WaitForMsg("+CGREG:", 2);
    if(err > 0)
        return err;

    /* Check if the MS is connected to the GPRS network - +CGATT: 0 OK*/
    GSM_SendAT("AT+CGATT?");
    err = GSM_WaitForMsg("+CGATT:", 2);
    if(err > 0)
        return err;

    /* Start Task and Set APN, USER NAME, PASSWORD */
    GSM_SendAT("AT+CSTT");    
    err=GSM_WaitForMsg("OK", 5);
    if(err>0)
        return err;

    /* Bring Up Wireless Connection with GPRS or CSD */
    GSM_SendAT("AT+CIICR");
    err=GSM_WaitForMsg("OK", 85);
    if(err > 0) {
        return err;
    }

    /* Get Local IP Address - 10.222.243.153*/
    GSM_SendAT("AT+CIFSR");
    err=GSM_WaitForMsg(".", 2);
    if(err>0)
        return err;
        
    /* Start up TCP or UDP connection */
    GSM_SendAT("AT+CIPSTART=\"TCP\",\"www.hwytree.com\",80"); 
    err = GSM_WaitForMsg("CONNECT OK",20);
    if(err > 0)
        return err;

    /* Send Data Through TCP or UDP Connection */
    GSM_SendAT("AT+CIPSEND"); 
    err = GSM_WaitForMsg(">",2);
    if(err > 0)
      return err; 

    sprintf((char*)gsm_snd_datas, 
      "GET %s?session=1&pn=%s %s\r\n%s\r\nHost: %s\r\n\r\n\0", 
      ROLLER_ADDR1,
      ROLLER_PHONE_NUM,
      ROLLER_HTTP_PROTOCOL,
      ROLLER_USER_AGENT,
      ROLLER_HOST
      );
    GSM_SendATData(gsm_snd_datas);
    err = GSM_WaitForMsg("SEND OK",5);
    if(err > 0)
      return err;   

    SysTick_Delay_ms(100);
   // UART4_RX_MSG_Proc();
   // INFO("%s\r\n",gsm_rcv_datas);

    for(i = 0; i < 20; i++) {
        err = GSM_MsgQuickCheck("CXALL");
        if(err == 0) {
            GSM_GPRSPushCarStatus();
            break;
        } 

        err = GSM_MsgQuickCheck("\"INSTRUCTION\":\"BPOPEN\"}");
        if(err == 0) {
            /* Beeper ON */
            SPI1_GetByte(0x17);
            INFO("\r\n Beeper Open OK!");
            break;
        }     

        err = GSM_MsgQuickCheck("\"INSTRUCTION\":\"BPCLOSE\"}");
        if(err == 0) {
            /* Beeper OFF */
            SPI1_GetByte(0x14);
            INFO("\r\n Beeper Close OK!");
            break;
        }

        err = GSM_MsgQuickCheck("\"INSTRUCTION\":\"CCGZM\"}");
        if(err == 0) {
            OBD_CleanUpDTC(&errStatus);
            INFO("\r\n DTC was cleaned up!");
            break;
        }

        err = GSM_MsgQuickCheck("\"INSTRUCTION\":\"BLOPEN\"}");
        if(err == 0) {
            BTFan_config();
            INFO("\r\n BT Power ON!");
            break;
        }
        err = GSM_MsgQuickCheck("\"INSTRUCTION\":\"BLCLOSE\"}");
        if(err == 0) {
            BT_PowerOff();
            INFO("\r\n BT Power OFF!");
            break;
        }
        err = GSM_MsgQuickCheck("\"INSTRUCTION\":\"EBLOPEN\"}");
        if(err == 0) {
            BTFan_Open();
            INFO("\r\n BT FAN OPEN!");
            break;
        }
        err = GSM_MsgQuickCheck("\"INSTRUCTION\":\"EBLCLOSE\"}");
        if(err == 0) {
            BTFan_Close();
            INFO("\r\n BT FAN CLOSE!");
            break;
        }        
        SysTick_Delay_ms(100);
        //INFO("%s\r\n",gsm_rcv_datas);
      //  UART4_RX_MSG_Proc();
    }

#if 0
    GSM_SendAT("AT+CIPCLOSE");
    err = GSM_WaitForMsg("CLOSE OK",10);
    if(err > 0)
      return err; 
#endif

    /* Deactivate GPRS PDP Context - AT+CIPSHUT SHUT OK*/
    GSM_SendAT("AT+CIPSHUT");    
    err = GSM_WaitForMsg("SHUT OK", 15);
    if(err > 0)
        return err;

    return err;
}

extern uint32_t ADC_SAMPLE_AVE;
extern uint32_t STM32_TEMP_AVE;


uint8_t GSM_GPRSPushCarStatus(void)
{
  ErrorStatus errStatus;
  uint8_t err = 0; 
  float acd_converted_o2_voltage;
  float adc_converted_temp_voltage;
 // float O2_persent;
  float car_battery_voltage = 0;
  uint8_t position_type = 0;

  double longitude = 0;
  double latitude = 0;
  char tmp[128];
  u8 kkw[2];
  u8 kdata[7];

  GSM_CheckCarBatteryVoltage();
  GSM_CheckBTSPosition();

  /* Send Data Through TCP or UDP Connection */
  GSM_SendAT("AT+CIPSEND"); 
  err = GSM_WaitForMsg(">",2);
  if(err > 0)
    return err; 
  adc_converted_temp_voltage  =(float) STM32_TEMP_AVE/4096*3.3; 
  acd_converted_o2_voltage =(float) ADC_SAMPLE_AVE/4096*3.3; 
#if 0  
  if(acd_converted_o2_voltage <= 0.71) {
    O2_persent = 0.209;
  }
  else if(acd_converted_o2_voltage < 1.06) {
    O2_persent = 0.209 - ((acd_converted_o2_voltage - 0.71) / (1.06-0.71))*(0.209 - 0.16);
    //O2_persent = 0.209 - (acd_converted_o2_voltage - 0.81) * 0.14;
  }
  else {
    O2_persent = 0.16 - ((acd_converted_o2_voltage - 1.06) / (2.21-1.06)) * 0.16;
    //O2_persent = 0.16 - (acd_converted_o2_voltage - 1.16) * 0.15238;
  }
#endif
  car_battery_voltage = (float) battery_voltage / 1000 * 6;

  if(GPS_data.lon.deg != 0)
  {
    position_type = 1;
    longitude = (double)GPS_data.lon.deg + ((double)GPS_data.lon.min + (double)GPS_data.lon.minp1 / 100 + (double)GPS_data.lon.minp2 / 10000) / 60;
    latitude = (double)GPS_data.lat.deg + ((double)GPS_data.lat.min + (double)GPS_data.lat.minp1 / 100 + (double)GPS_data.lat.minp2 / 10000) / 60;     
  }


    switch(OBD_GetProtocol())
    {
    case OBD_ISO14230_4ADDR:
        ISO14230_4ADDR_start(kkw);
        break;
        
    case OBD_ISO14230_4HL:
        ISO14230_4HL_start(kdata);
        break;

    case OBD_ISO9141_2ADDR:
        ISO9141_2ADDR_start(kkw);
        break;
        
    case OBD_ISO15765_4STD_500K:
    case OBD_ISO15765_4EXT_500K:
    case OBD_ISO15765_4STD_250K:
    case OBD_ISO15765_4EXT_250K:
    case OBD_PROTOCOL_UNKNOWN:
    default:
        break;
    }  
  
  sprintf((char*)gsm_json_datas,
    "{\"DATA\":[{\"pn\":\"%s\"},{\"alarmdata\":\"0\"},{\"commanddata\":\"10\",",
    ROLLER_PHONE_NUM);

  sprintf(tmp,
    "\"BUSTYPE\":\"%d\",",
    OBD_GetProtocol());
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);    
  sprintf(tmp,
    "\"OBDZS\":\"%s\",",
    OBD_ReadDS(OBD_DS_RPM, &errStatus));
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);

  sprintf(tmp,
    "\"OBDCS\":\"%s\",",
    OBD_ReadDS(OBD_DS_VSS, &errStatus));
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);

  sprintf(tmp,
    "\"OBDSW\":\"%s\",",
    OBD_ReadDS(OBD_DS_ECT, &errStatus));
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);  

  sprintf(tmp,
    "\"OBDKQLL\":\"%s\",",
    OBD_ReadDS(OBD_DS_MAF, &errStatus));
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);

  sprintf(tmp,
    "\"OBDQGJDYL\":\"%s\",",
    OBD_ReadDS(OBD_DS_MAP, &errStatus));
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);

  sprintf(tmp,
    "\"OBDJQMKD\":\"%s\",",
    OBD_ReadDS(OBD_DS_TP, &errStatus));
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);
  
  sprintf(tmp,
    "\"OBDYNDCGQZ\":\"%s\",",
    OBD_ReadDS(OBD_DS_O2B1S1, &errStatus));
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);

  sprintf(tmp,
    "\"OBDFHBFB\":\"%s\",",
    OBD_ReadDS(OBD_DS_PCT, &errStatus));
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);

  sprintf(tmp,
    "\"CNWD\":\"%.3f\",",
    adc_converted_temp_voltage);
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);

  sprintf(tmp,
    "\"CNYNN\":\"%.3f\",",
    acd_converted_o2_voltage);
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);

  sprintf(tmp,
    "\"BATV\":\"%.2f\",",
    car_battery_voltage);
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);

  sprintf(tmp,
    "\"DWLX\":\"%d\",\"JZDW\":\"%d,%d,%d,%d\",\"GPSDW\":\"%.06f,%.07f\",",
    position_type,    
    mcc,
    mnc,
    lac,
    cellid,
    longitude,
    latitude);
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);

  sprintf(tmp,
    "\"OBDECN\":\"%s\",",
    OBD_ReadDS(OBD_DS_DTC_CNT, &errStatus));
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);

  sprintf(tmp,
    "\"OBDEC\":\"%s\"},{\"bookdata\":\"0\"}]}\r\n",
    OBD_ReadDTC(&errStatus));
  strcpy((char*)(gsm_json_datas+strlen((char*)gsm_json_datas)),tmp);

  sprintf((char*)gsm_snd_datas, 
    "POST %s %s\r\n%s\r\n%s\r\nContent-Length: %d\r\n%s\r\n%s\r\nHost: %s:80\r\n%s\r\n\r\n%s",
    ROLLER_ADDR2,
    ROLLER_HTTP_PROTOCOL,
    ROLLER_ACCEPT,
    ROLLER_ACCEPT_LA,
    strlen((char*)gsm_json_datas),
    ROLLER_CTT_TYPE,
    ROLLER_CONNECTION,
    ROLLER_HOST,  
    ROLLER_USER_AGENT,
    gsm_json_datas
    );

  GSM_SendATData(gsm_snd_datas);
  err = GSM_WaitForMsg("SEND OK",2);
  if(err > 0)
    return err; 

#if 0
  /* Close connection */
  GSM_SendAT("AT+CIPCLOSE");   
  err = GSM_WaitForMsg("SEND OK",5);
  if(err > 0)
    return err;   
#endif
  return err;

}

/* timeout seconds */
uint8_t GSM_WaitForMsg(uint8_t *p, uint32_t timeout)
{
    char *p_find = NULL;
    uint32_t cnt = 0;
    uint8_t err = 1;

    SysTick_Delay_ms(100);
    timeout *= 10;
    while(cnt < timeout) {
        if(gsm_rcv_data_cnt > 0) {
            p_find = strstr((char*)gsm_rcv_datas, (char*)p);
            if(NULL != p_find) {
                INFO("%s\r\n",gsm_rcv_datas);
                err = 0;
                break;
            }

            p_find = strstr((char*)gsm_rcv_datas, "ERROR");
            if(NULL != p_find) {
                INFO("%s\r\n",gsm_rcv_datas);
                err = 2;
                break;
            }
        }
        SysTick_Delay_ms(100);
        cnt++;
    }

    /* timeout ? */
    if(1 == err) INFO("%s\r\n",gsm_rcv_datas);
    return err;
} 

uint8_t GSM_MsgQuickCheck(uint8_t *p)
{
    char *p_find = NULL;
    uint8_t err = 1;

    if(gsm_rcv_data_cnt > 0) {
        p_find = strstr((char*)gsm_rcv_datas, (char*)p);
        if(NULL != p_find) {
            err = 0;
        }
    }

    return err;
} 

uint8_t GSM_CheckCarBatteryVoltage(void)
{
    uint8_t err = 0;
    GSM_SendAT("AT+CADC?");
    err = GSM_WaitForMsg("+CADC:", 2);
    if(err != 0) {
        return err;
    }

    battery_voltage = ParserCADC();
    return err;
}

uint8_t GSM_CheckBTSPosition(void)
{
    uint8_t err = 0;
    GSM_SendAT("AT+CENG=1,0");
    err = GSM_WaitForMsg("OK", 2);
    if(err != 0) {
        return err;
    }

    GSM_SendAT("AT+CENG?");
    err = GSM_WaitForMsg("+CENG:", 2);
    if(err != 0) {
        return err;
    }

    ParserCENG();
    GSM_SendAT("AT+CENG=0,0");
    err = GSM_WaitForMsg("OK", 2);
    if(err != 0) {
        return err;
    }    
    return err;
}

uint8_t BT_PowerOff(void)
{
    uint8_t err = 0;
    
    GSM_SendAT("AT+BTPOWER=0");
    err = GSM_WaitForMsg("OK", 10);
    if(err > 0)
        return err;  
    return err;
}

uint8_t BTFan_config(void)
{
    uint8_t err = 0;
    uint8_t retry = 0;
    uint8_t bt_status = 0;
    char buff[32];

    DBG_LED2_OFF();
BT_POWERON:
    GSM_SendAT("AT+BTPOWER=1"); 
    err = GSM_WaitForMsg("OK", 10);
    if(err > 0)
    {
        GSM_SendAT("AT+BTPOWER=0");
        SysTick_Delay_ms(30000);
        goto BT_POWERON;

    }

    GSM_SendAT("AT+BTSTATUS?"); 
    err = GSM_WaitForMsg("OK", 10);
    if(err > 0)
        return err;     

    bt_status = ParserBTStatus();
    INFO("BT stauts=%d\r\n", bt_status);
    switch(bt_status)
    {
    case 1:
        GSM_SendAT("AT+BTUNPAIR=0"); 
        err = GSM_WaitForMsg("OK", 10);
        if(err > 0)
            return err; 

        break;
    case 2:    
        GSM_SendAT("AT+BTUNPAIR=0"); 
        err = GSM_WaitForMsg("OK", 10);
        if(err > 0)
            return err; 

        break;
    case 0:
    default:   
        break;
    }

    GSM_SendAT("AT+BTSTATUS?"); 
    err = GSM_WaitForMsg("OK", 10);
    if(err > 0)
        return err; 

    retry = 0;
BT_SCAN:
    retry++;
    GSM_SendAT("AT+BTSCAN=1,60"); 
    err = GSM_WaitForMsg("+BTSCAN: 1", 80);
    if(err > 0)
        return err;

    err = ParserBTFanDeviceID();
    if(err > 0)
    {
        if(retry > 2)
            return err; 
        else
            goto BT_SCAN;
    }

    sprintf(buff, "AT+BTPAIR=0,%u", bt_fan_device_id);
    GSM_SendAT((uint8_t *)buff); 
    err = GSM_WaitForMsg(BT_DEV_NAME, 10);
    if(err > 0)
        return err;

    GSM_SendAT("AT+BTPAIR=2,1234"); 
    err = GSM_WaitForMsg(BT_DEV_NAME, 10);
    if(err > 0)
        return err;

    SysTick_Delay_ms(1000);

    GSM_SendAT("AT+BTSTATUS?"); 
    err = GSM_WaitForMsg(BT_DEV_NAME, 10);
    if(err > 0)
        return err;    

    SysTick_Delay_ms(1000);
    retry = 0;
BT_GETPROFILE:
    retry++;
    GSM_SendAT("AT+BTGETPROF=1"); 
    err = GSM_WaitForMsg("OK", 60);
    if(err > 0)
        return err; 

    err = ParserBTFanProfileID();
    if(err > 0)
    {
        if(retry > 3)
            return err; 
        else
            goto BT_GETPROFILE;
    }

    sprintf(buff, "AT+BTCONNECT=%u,%u", bt_fan_device_id, bt_fan_profile_id);
    GSM_SendAT((uint8_t *)buff); 
    err = GSM_WaitForMsg("+BTCONNECT:", 60);
    if(err > 0)
        return err;  

    GSM_SendAT("AT+BTSTATUS?"); 
    err = GSM_WaitForMsg("OK", 10);
    if(err > 0)
        return err; 

    DBG_LED2_ON();
    return 0; 
}

uint8_t BTFan_Open(void)
{
    uint8_t err = 0;

    GSM_SendAT("AT+BTSPPSEND"); 
    err = GSM_WaitForMsg(">",2);
    if(err > 0)
      return err; 

    UART4_SendByte(0xCC);
    UART4_SendByte(0x00);
    UART4_SendByte(0x00);
    UART4_SendByte(0x00);
    UART4_SendByte(0xEE);
    UART4_SendByte(0x1A);
    GSM_DataReset();

    return err; 
}

uint8_t BTFan_Close(void)
{
    uint8_t err = 0;

    GSM_SendAT("AT+BTSPPSEND"); 
    err = GSM_WaitForMsg(">",2);
    if(err > 0)
      return err; 

    UART4_SendByte(0xCC);
    UART4_SendByte(0x01);
    UART4_SendByte(0x00);
    UART4_SendByte(0x00);
    UART4_SendByte(0xEE);
    UART4_SendByte(0x1A);
    GSM_DataReset();

    return err; 
}


static void set_fixed_baudrate(void)
{
    uint8_t i = 0;
    uint8_t err = 1;

    for(i = 0; i < 200; i++)
    {
        /* No echo */
        GSM_SendAT("AT");
        err = GSM_WaitForMsg("OK",2);  
        if(err == 0) break;
    }

    if(i == 200) SYS_Reset();


    GSM_SendAT("AT+IPR=115200");
    err = GSM_WaitForMsg("OK",2);  
    if(err > 0)
        SYS_Reset();
 

    GSM_SendAT("AT&W");
    GSM_WaitForMsg("OK",2);  
    SYS_Reset();
     
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
    for(i = 0; i < 5; i++) {
        p++;
        if(*p > 0x2F && *p < 0x3A) {
            rssi *= 10;
            rssi += *p - 0x30;
        }
        else if(*p == ',')
            break;
    }

    p = strchr((char *)gsm_rcv_datas, ','); 
    for(i = 0; i < 10; i++) {
        p++;
        if(*p > 0x2F && *p < 0x3A) {
            ber *= 10;
            ber += *p - 0x30;
        }
        else if(*p == 0x0D || *p == 0x0A)
            break; 
    }
 //   INFO("\r\n rssi=%d ber=%d\r\n ", rssi, ber);

    if(rssi > 1 && rssi < 32) {
        if(ber > 7) {
            err=1;  
        }
    }
    else {
        err=2;
    }

    return err;
}

/* 
AT+CADC?
+CADC: 1,2034

*/
static uint16_t ParserCADC(void)
{
    uint8_t i = 0;
    uint16_t flag = 0;
    uint16_t voltage = 0;
    char *p = NULL;  

    p = strchr((char *)gsm_rcv_datas, ':'); 
    for(i = 0; i < 5; i++) {
        p++;
        if(*p > 0x2F && *p < 0x3A) {
            flag *= 10;
            flag += *p - 0x30;
        }
        else if(*p == ',')
            break;
    }

    p = strchr((char *)gsm_rcv_datas, ','); 
    for(i = 0; i < 10; i++) {
        p++;
        if(*p > 0x2F && *p < 0x3A) {
            voltage *= 10;
            voltage += *p - 0x30;
        }
        else if(*p == 0x0D || *p == 0x0A)
            break; 
    }
//    INFO("\r\n flag=%d battery_voltage=%d\r\n ", flag, voltage);

    if(flag == 0) {
        voltage = 0;
    }

    return voltage;
}

/* 
AT+CENG?

+CENG:0,"0058,36,99,460(mcc),00(mnc),58(bsic),56cd(cellid22221),12,05,3361(lac),255"

+CENG:1,"0072(arfcn),10(rxl),43(bsic),278b(cellid),460(mcc),00(mnc),3339(lac)"

+CENG:2,"0062,06,20,50c1,460,00,3361"

+CENG:3,"0524,07,50,2b71,460,00,3339"

+CENG:4,"0070,10,52,27a7,460,00,3339"

+CENG:5,"0057,19,127,ffff,000,00,0"

+CENG:6,"0076,10,104,ffff,000,00,0"


*/
void ParserCENG(void)
{
    uint8_t i = 0;
    char *p = NULL;  
    uint16_t tmp = 0;

    mcc = 0;
    mnc = 0;
    cellid = 0;
    lac = 0;


    /* Find Current cell */
    p = strchr((char *)gsm_rcv_datas, '\"'); 
    for(i = 0; i < 5; i++) {
        p++;
        if(*p == ',')
            break;
    }
    for(i = 0; i < 5; i++) {
        p++;
        if(*p == ',')
            break;
    }
    for(i = 0; i < 5; i++) {
        p++;
        if(*p == ',')
            break;
    }

    for(i = 0; i < 5; i++) {
        p++;
        if(*p > 0x2F && *p < 0x3A) {
            mcc *= 10;
            mcc += *p - 0x30;
        }
        else if(*p == ',')
            break; 
    }  

    for(i = 0; i < 5; i++) {
        p++;
        if(*p > 0x2F && *p < 0x3A) {
            mnc *= 10;
            mnc += *p - 0x30;
        }
        else if(*p == ',')
            break; 
    }

    for(i = 0; i < 5; i++) {
        p++;
        if(*p == ',')
            break;
    }

    for(i = 0; i < 5; i++) {
        p++;
        if((*p > 0x2F && *p < 0x3A) || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) {
            switch(*p)
            {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                tmp = *p - 0x30;
                break;
            case 'a':
            case 'A':
                tmp = 10;
                break;
            case 'b':
            case 'B':
                tmp = 11;
                break;
            case 'c':
            case 'C':
                tmp = 12;
                break;
            case 'd':
            case 'D':
                tmp = 13;
                break;
            case 'e':
            case 'E':
                tmp = 14;
                break;
            case 'f':
            case 'F':
                tmp = 15;
                break;
            default:
                tmp = 0;
                break;
            }
            cellid *= 16;
            cellid += tmp;
        }
        else if(*p == ',')
            break; 
    }

    for(i = 0; i < 5; i++) {
        p++;
        if(*p == ',')
            break;
    }

    for(i = 0; i < 5; i++) {
        p++;
        if(*p == ',')
            break;
    }    

    for(i = 0; i < 5; i++) {
        p++;
        if((*p > 0x2F && *p < 0x3A) || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) {
            switch(*p)
            {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                tmp = *p - 0x30;
                break;
            case 'a':
            case 'A':
                tmp = 10;
                break;
            case 'b':
            case 'B':
                tmp = 11;
                break;
            case 'c':
            case 'C':
                tmp = 12;
                break;
            case 'd':
            case 'D':
                tmp = 13;
                break;
            case 'e':
            case 'E':
                tmp = 14;
                break;
            case 'f':
            case 'F':
                tmp = 15;
                break;
            default:
                tmp = 0;
                break;
            }
            lac *= 16;
            lac += tmp;        
        }
        else if(*p == ',')
            break; 
    }

    //INFO("mcc=%d,mnc=%d,cellid=%x,lac=%d\r\n",mcc,mnc,cellid,lac);

}

/*
AT+BTSCAN=1,10

OK

+BTSCAN: 0,1,"SPP-CA",00:ba:55:57:71:dd,-57

+BTSCAN: 1
*/

static uint8_t ParserBTFanDeviceID(void)
{
    uint8_t i = 0;
    uint8_t err = 0;
    char *p = NULL;  
    p = strstr((char *)gsm_rcv_datas, BT_DEV_NAME); 
    if(NULL == p)
        return 1;
    while(*p != ',') p--;
    p--;
    while(*p != ',') p--;
    for(i = 0; i < 5; i++) {
        p++;
        if(*p > 0x2F && *p < 0x3A) {
            bt_fan_device_id *= 10;
            bt_fan_device_id += *p - 0x30;
        }
        else if(*p == ',')
            break;
    }
    INFO("bt_fan_device_id=%d\r\n",bt_fan_device_id);

    return err;
}

/*
AT+BTGETPROF=1

+BTGETPROF: 4,"SPP"

OK
*/

static uint8_t ParserBTFanProfileID(void)
{
    uint8_t i = 0;
    uint8_t err = 0;
    char *p = NULL;  

    p = strchr((char *)gsm_rcv_datas, ':'); 
    for(i = 0; i < 5; i++) {
        p++;
        if(*p > 0x2F && *p < 0x3A) {
            bt_fan_profile_id *= 10;
            bt_fan_profile_id += *p - 0x30;
        }
        else if(*p == ',')
            break;
    }

    return err;
}

/*
+BTSTATUS: 5
P: 1,"SPP-CA",00:ba:55:57:71:d0
C:
OK

*/

static uint8_t ParserBTStatus(void)
{
    char *p = NULL;  

    p = strstr((char *)gsm_rcv_datas, "C:"); 
    if(NULL != p)
        return 2;

    p = strstr((char *)gsm_rcv_datas, "P:"); 
    if(NULL != p)
        return 1;

    return 0;
}


