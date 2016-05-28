#ifndef __GSM_H
#define __GSM_H
#include "stm32f10x.h"
#include <stdio.h>

void GSM_DataReset(void);

void GSM_RetrieveData(void * src, size_t len);
void GSM_SendAT(uint8_t *data);
uint8_t GSM_GPRSBuildTCPLink(void);


uint8_t GSM_PowerOn(void);
uint8_t GSM_GPRSConnect(void);
uint8_t GSM_Location(void);
uint8_t GSM_ConnectionHeartBeat(void);
uint8_t GSM_GPRSPushCarStatus(void);
uint8_t GSM_GPRSBeeperStatus(void);
uint8_t GSM_WaitForMsg(uint8_t *p,uint32_t timeout);
uint8_t GSM_MsgQuickCheck(uint8_t *p);


#endif  /*__GSM_H*/
