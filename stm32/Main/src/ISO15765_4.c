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
#include <stdio.h>
#include "debug.h"
#include "systick.h"
#include "obd.h"
#include "formula.h"
#include "ISO15765_4.h"

uint8_t CAN_identifier_type;

/*******************************系统激活********************************/
CanTxMsg EntCmd15765  = {0x7DF,0x18DB33F1,CAN_ID_STD,CAN_RTR_DATA,8,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x00};
/*******************************读故障码********************************/
CanTxMsg DTCCmd15765  = {0x7DF,0x18DB33F1,CAN_ID_STD,CAN_RTR_DATA,8,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x00};
/*******************************清除障码********************************/
CanTxMsg CDTCCmd15765 = {0x7DF,0x18DB33F1,CAN_ID_STD,CAN_RTR_DATA,8,0x01,0x04,0x00,0x00,0x00,0x00,0x00,0x00};
/*******************************读数据流********************************/
CanTxMsg DSCmd15765   = {0x7DF,0x18DB33F1,CAN_ID_STD,CAN_RTR_DATA,8,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x00};
/************************************************************************/

ISO15765_4_TYPE ISO15765_4_ProtocolDetect(ErrorStatus *err)
{
    uint8_t *ram;
    
    ISO15765_4_Config(ISO15765_4STD_500K);
    ram = Send_CANFrame(&EntCmd15765,err);
    if (*err == SUCCESS && 'A' == ram[1])
    {	
        INFO("\r\n ISO15765_4STD_500K\r\n"); 
        return ISO15765_4STD_500K;
    }
    else
    {
        INFO("\r\n ISO15765_4STD_500K Failed\r\n"); 
    }

    ISO15765_4_Config(ISO15765_4STD_250K);
    Send_CANFrame(&EntCmd15765,err);
    if (*err == SUCCESS && 'A' == ram[1])
    {	
        INFO("\r\n ISO15765_4STD_250K\r\n");
        return ISO15765_4STD_250K;
    }
    else
    {
        INFO("\r\n ISO15765_4STD_250K Failed\r\n"); 
    }
    
    ISO15765_4_Config(ISO15765_4EXT_500K);
    Send_CANFrame(&EntCmd15765,err);
    if (*err == SUCCESS && 'A' == ram[1])
    {	
        INFO("\r\n ISO15765_4EXT_500K\r\n"); 
        return ISO15765_4EXT_500K;
    }
    else
    {
        INFO("\r\n ISO15765_4EXT_500K Failed\r\n"); 
    }

    ISO15765_4_Config(ISO15765_4EXT_250K);
    Send_CANFrame(&EntCmd15765,err);
    if (*err == SUCCESS && 'A' == ram[1])
    {	
        INFO("\r\n ISO15765_4EXT_250K\r\n");
        return ISO15765_4EXT_250K;
    } 
    else
    {
        INFO("\r\n ISO15765_4EXT_250K Failed\r\n"); 
    }

    INFO("\r\n CAN_OBFII_TEST_FAIL\r\n");
    *err = ERROR;
    return ISO15765_4_TYPE_UNKNOWN;
}

void ISO15765_4_Config(ISO15765_4_TYPE type)
{
    switch(type)
    {
    case ISO15765_4STD_500K:
    default:    
        CAN_Config(CAN_500K, CAN_ID_STD);
        break;

    case ISO15765_4EXT_500K:
        CAN_Config(CAN_500K, CAN_ID_EXT);
        break;

    case ISO15765_4STD_250K:
        CAN_Config(CAN_250K, CAN_ID_STD);
        break;
        
    case ISO15765_4EXT_250K:
        CAN_Config(CAN_250K, CAN_ID_EXT);
        break;        
    }
}

char* ISO15765_4_ReadDTC(ErrorStatus* err)
{
    return CAN_ReadDTC(&DTCCmd15765,err);
}

void ISO15765_4_CleanUpDTC(ErrorStatus* err)
{
    Send_CANFrame(&CDTCCmd15765,err);
}

__IO char         DSRAM[100];
/************************************************************************
  * @描述:  ISO15765 读实时数据流
  * @参数:  CanTxMsg* DSCmd:数据流命令 ErrorStatus* err 
  * @返回值: char* 数据流结果地址
  **********************************************************************/
char* ISO15765_4_ReadDS(ISO15765_4_DS cmd, ErrorStatus* err)
{
    uint8_t *ram;
    float result;
    char * string;

    DSCmd15765.Data[2] = DSControl[cmd].PIDByte;
    DSRAM[0]=0x00;
    ram = Send_CANFrame(&DSCmd15765,err);
    if (*err == SUCCESS)
    {
        if (DSControl[cmd].Type == Numeric)
        {
            result = DSControl[cmd].Equation0(ram + DSControl[cmd].FineByte);
            sprintf((char*)DSRAM,(char*)DSControl[cmd].Format, result);
        }
        else
        {
            string = DSControl[cmd].Equation1(ram + DSControl[cmd].FineByte);
            strcpy((char*)DSRAM, string);
        }
    }

    return (char*)DSRAM;
}

