#ifndef __SPI_H
#define __SPI_H
#include "stm32f10x.h"
#include <stdio.h>

void SPI1_Init(void);
uint8_t SPI1_GetByte(uint8_t cmd);

#endif  /*__SPI_H*/
