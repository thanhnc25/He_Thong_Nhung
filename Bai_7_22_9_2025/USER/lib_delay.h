#ifndef _LIB_DELAY_H_
#define _LIB_DELAY_H_

#include "lib_define.h"

void Delay_Setup(void);
void SysTick_Handler(void);
void Delay_ms(uint16_t time_delay);

#endif
