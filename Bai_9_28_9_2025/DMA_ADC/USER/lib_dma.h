#ifndef __LIB_DMA_H
#define __LIB_DMA_H

#include "stm32f10x.h"

void DMA_Config_Channel1(uint32_t periphAddr, uint32_t memAddr, uint16_t size);

typedef void (*DMA_Callback_t)(void);

void DMA_SetCallback_Channel1(DMA_Callback_t cb);

#endif
