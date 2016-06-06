#ifndef __SPI_H
#define __SPI_H
#include "stm32f10x.h"
#include <stdio.h>

void SPIx_Init(void);
uint8_t SPIx_ReadWriteByte(uint8_t TxData);
void Write_data(uint8_t RxData);
uint8_t SPI_SendByte(uint8_t byte);

#endif  /*__SPI_H*/
