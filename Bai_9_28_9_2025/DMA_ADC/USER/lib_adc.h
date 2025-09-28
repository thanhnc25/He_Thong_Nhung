#ifndef _LIB_ADC_H_
#define _LIB_ADC_H_

#include "main.h"

void GPIO_ADC_Config(void);
void ADC_Config(void);
int Map(int x, int in_min, int in_max, int out_min, int out_max);
unsigned int ADCx_Read(void);

#endif