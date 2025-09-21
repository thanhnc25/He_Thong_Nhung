#ifndef _LIB_SPI_H_
#define _LIB_SPI_H_

#include "lib_define.h"

void SPI_Setup(SPI_TypeDef* SPIx, uint16_t spi_baurate);
void SPIx_SetSpeed(SPI_TypeDef* SPIx, uint16_t spi_baurate);
uint8_t SPIx_Transfer(SPI_TypeDef* SPIx, uint8_t data);

#endif
